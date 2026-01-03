#pragma once
#ifndef _PARTICLE_DATA_BUFFER_HPP_
#define _PARTICLE_DATA_BUFFER_HPP_
#include <AllocatedBuffer.hpp>
#include <GlobalDef.hpp>
#include <array>
#include <atomic>
#include <memory>
#include <random>
#include <type_traits>
#include <vulkan/vulkan.h>
#include <particle/ParticleDefTraits.hpp>
#include <particle/ParticleLayoutPolicy.hpp>

namespace engine {

class VulkanEngine;

enum class ParticleDimension { Particle2D, Particle3D };

namespace v1 {
          template <typename ParticleType, typename = void>
          struct ParticleSysDataBuffer : std::false_type {
                    static_assert(sizeof(ParticleType) == 0,
                              "ParticleSysDataBuffer requires ParticleType with position, "
                              "velocity, and color fields!");
          };

          // ParticleSysDataBuffer implements ping-pong GPU storage for particle
// simulation.
// - Two GPU buffers are allocated: buffers_[0] and buffers_[1].
// - get_in_buffer() reads from buffers_[index], get_out_buffer() writes to
// buffers_[index ^ 1].
// - swap() flips index each simulation step.
//
// Upload model:
// - perpareTransferData(): creates buffers if needed and prepares CPU staging
// for BOTH buffers,
//   so both ping-pong states start initialized (avoids reading uninitialized
//   GPU memory).
// - singleTimeUpload(cmd): records GPU upload commands once (idempotent).
// - updateUploadingStatus(observedValue): advances internal resource state
// based on timeline.
// - purgeReleaseStaging(observedValue): releases CPU staging only after GPU
// upload is complete.

          template <typename _Ty>
          struct ParticleSysDataBuffer<
                    _Ty, std::enable_if_t<has_particle_fields_v<_Ty>, void>> {

                    using ParticleType = _Ty;

                    ParticleSysDataBuffer(VkDevice device, VmaAllocator allocator)
                              : device_(device), allocator_(allocator) {
                              mt.seed(static_cast<unsigned>(std::random_device{}()));
                              rndDist = std::uniform_real_distribution<float>(0.f, 1.f);
                    }

                    virtual ~ParticleSysDataBuffer() { destroy(); }

                    void destroy() { __destroyGpuBuffer(); }

                    void configure(const std::size_t particle_count,
                              ParticleDimension dim = ParticleDimension::Particle3D,
                              VkBufferUsageFlags bufferUsage =
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                              VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY) {
                              if (gpuAllocated_) {
                                        throw std::runtime_error(
                                                  "configure() after GPU allocated; call destroy() first");
                              }

                              dim_ = dim;

                              particleCount = particle_count;
                              particleBufferSize = particleCount * particleSize;

                              bufferUsage_ = bufferUsage;
                              memoryUsage_ = memoryUsage;

                              configure_ = true;
                    }

                    void singleTimeUpload(VkCommandBuffer cmd) {
                              std::call_once(upload_once_, [&] {
                                        if (!cpuReady_)
                                                  throw std::runtime_error("CPU staging missing");
                                        for (auto& buf : buffers_)
                                                  buf->recordUpload(cmd);
                                        pendingUpload_ = true;
                                        });
                    }

                    void perpareTransferData() {
                              if (!configure_)
                                        throw std::runtime_error("Please configure() first");

                              if (!gpuAllocated_) {
                                        __createGpuBuffer();
                              }

                              std::vector<uint8_t> data =
                                        ((dim_ == ParticleDimension::Particle2D) ? fill2D() : fill3D());

                              for (auto& buf : buffers_) {
                                        buf->perpareTransferData(data.data(), data.size());
                              }
                              cpuReady_ = true;
                    }

                    void updateUploadingStatus(uint64_t observedValue) {
                              if (pendingUpload_) {
                                        for (auto& buf : buffers_) {
                                                  buf->updateUploadingStatus(observedValue);
                                        }
                                        pendingUpload_ = false;
                              }
                    }

                    void purgeReleaseStaging(uint64_t observedValue) {
                              if (cpuReady_) {
                                        for (auto& buf : buffers_) {
                                                  buf->purgeReleaseStaging(observedValue);
                                        }
                                        cpuReady_ = false;
                              }
                    }

                    void setUploadCompleteTimeline(uint64_t value) {
                              for (auto& buf : buffers_) {
                                        buf->setUploadCompleteTimeline(value);
                              }
                    }

                    std::size_t getParticleCount() const noexcept { return particleCount; }

                    std::size_t getBufferSize() const noexcept { return particleBufferSize; }

                    void swap() noexcept {
                              index ^= 1; // index = (index + 1) % 2
                    }

                    VkBuffer get_in_buffer() const noexcept { return buffers_[index]->buffer(); }

                    VkBuffer get_out_buffer() const noexcept {
                              return buffers_[index ^ 1]->buffer();
                    }

                    /*one is for particle in, another is for particle out;
                     * buffer[0] will be the first one and it's going to swap in every frame
                     */
                    bool index = 0;
                    std::size_t particleCount{ 0 };
                    std::size_t particleSize = sizeof(ParticleType);
                    std::size_t particleBufferSize{ 0 };

                    std::array<std::shared_ptr<::engine::v2::AllocatedBuffer2>, 2> buffers_;

          protected:
                    virtual std::vector<uint8_t> fill2D() {
                              std::vector<uint8_t> ret;
                              ret.resize(particleBufferSize);
                              ParticleType* src = reinterpret_cast<ParticleType*>(ret.data());

                              for (std::size_t i = 0; i < particleCount; ++i) {
                                        auto& particle = src[i];

                                        float r = std::sqrt(rndDist(mt));
                                        float theta = 2.0f * glm::pi<float>() * rndDist(mt);

                                        float x = r * cos(theta);
                                        float y = r * sin(theta);

                                        particle.position = glm::vec4(x, y, 0, 1);
                                        glm::vec3 pos = glm::vec3(particle.position);
                                        glm::vec3 vel = glm::normalize(glm::cross(pos, glm::vec3(0, 0, 1)));

                                        particle.velocity = glm::vec4(vel * 0.002f, 0.0f);
                                        particle.color = glm::vec4(rndDist(mt), rndDist(mt), rndDist(mt), 1.0f);
                              }
                              return ret;
                    }

                    virtual std::vector<uint8_t> fill3D() {
                              std::vector<uint8_t> ret;
                              ret.resize(particleBufferSize);
                              ParticleType* src = reinterpret_cast<ParticleType*>(ret.data());
                              for (std::size_t i = 0; i < particleCount; ++i) {
                                        auto& particle = src[i];

                                        float u = rndDist(mt);
                                        float v = rndDist(mt);

                                        float theta = 2.0f * glm::pi<float>() * u; // 0~2pi
                                        float phi = acos(2.0f * v - 1);            //  0~π

                                        float sx = sin(phi) * cos(theta);
                                        float sy = sin(phi) * sin(theta);
                                        float sz = cos(phi);

                                        float r = cbrt(rndDist(mt));

                                        glm::vec3 direction = glm::vec3(sx, sy, sz);
                                        particle.position = glm::vec4(direction * r, 1.0f);
                                        particle.velocity = glm::vec4(direction * 0.0005f, 0.f);
                                        particle.color = glm::vec4(rndDist(mt), rndDist(mt), rndDist(mt), 1.0f);
                              }
                              return ret;
                    }

          private:
                    void __createGpuBuffer() {
                              if (gpuAllocated_)
                                        return;

                              for (auto& buf : buffers_) {
                                        buf.reset();
                                        buf = std::make_shared<::engine::v2::AllocatedBuffer2>(
                                                  device_, allocator_, "ParticleSysDataBuffer::PingPongBuffer");

                                        buf->configure(particleBufferSize, bufferUsage_, memoryUsage_);
                              }

                              gpuAllocated_ = true;
                    }

                    void __destroyGpuBuffer() {
                              if (gpuAllocated_) {
                                        for (auto& buf : buffers_) {
                                                  buf->destroy();
                                                  buf.reset();
                                        }
                                        gpuAllocated_ = false;
                              }
                    }

          protected:
                    ParticleDimension dim_;

                    std::random_device rd;
                    std::mt19937 mt;
                    std::uniform_real_distribution<float> rndDist;

                    VkBufferUsageFlags bufferUsage_ = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
                    VmaMemoryUsage memoryUsage_ = VMA_MEMORY_USAGE_GPU_ONLY;

          private:
                    bool configure_ = false;
                    bool gpuAllocated_ = false;
                    bool cpuReady_ = false;
                    bool pendingUpload_ = false;

                    std::once_flag upload_once_;

                    VkDevice device_{};
                    VmaAllocator allocator_{};
          };
}

namespace v2 {
          template <typename ParticleType,
                    template<typename> class LayoutT = ::engine::particle::policy::SOALayout,
                    typename = void>
          class ParticleSysDataBuffer2  {
          public:
                    static_assert(!std::is_same_v<ParticleType, ParticleType> == 0,
                              "ParticleSysDataBuffer requires ParticleType with position, "
                              "velocity, and color fields!");
          };

          template <typename ParticleType,
                              template<typename> class LayoutT
          >
          class ParticleSysDataBuffer2<
                    ParticleType, 
                    LayoutT,
                    std::enable_if_t<is_valid_particle_layout_v<ParticleType, LayoutT>, void>
          >{
          
          public:
                    using Layout = LayoutT<ParticleType>;
                    using View = typename Layout::view;

                    ParticleSysDataBuffer2(VkDevice device, VmaAllocator allocator, std::string name = "ParticleSysDataBuffer2")
                              :device_(device)
                              , allocator_(allocator)
                              , name_(name)
                    { 
                              mt.seed(static_cast<unsigned>(std::random_device{}()));
                              rndDist = std::uniform_real_distribution<float>(0.f, 1.f);
                    }

                    virtual ~ParticleSysDataBuffer2() { destroy(); }

          public:

                    void configure(const std::size_t particle_count,
                              ParticleDimension dim = ParticleDimension::Particle3D,
                              VkBufferUsageFlags bufferUsage =
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                              VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY) {

                              if (gpuAllocated_) {
                                        throw std::runtime_error(
                                                  "configure() after GPU allocated; call destroy() first");
                              }

                              dim_ = dim;

                              particleCount_ = particle_count;

                              bufferUsage_ = bufferUsage;
                              memoryUsage_ = memoryUsage;

                              configure_ = true;
                    }

                    void destroy() {
                              __destroyGpuBuffer();
                    }

                    void swap() noexcept {
                              index ^= 1; // index = (index + 1) % 2
                    }

                    View in() const noexcept {
                              assert(layout_ && "ParticleSysDataBuffer2: layout not created");
                              return layout_->in(index);
                    }

                    View out() const noexcept {
                              assert(layout_ && "ParticleSysDataBuffer2: layout not created");
                              return layout_->out(index);
                    }

                    std::size_t getParticleCount() const noexcept { 
                              return particleCount_; 
                    }

                    void singleTimeUpload(VkCommandBuffer cmd) {
                              if (!cpuReady_)
                                        throw std::runtime_error("CPU staging missing");

                              std::call_once(upload_once_, [&] {

                                        if (layout_) {
                                                  layout_->recordUpload(cmd);
                                        }
                                        pendingUpload_ = true;
                                        });
                    }

                    void prepareTransferData() {
                              if (!configure_)
                                        throw std::runtime_error("Please configure() first");

                              if (!gpuAllocated_) {
                                        __createGpuBuffer();
                              }

                              auto data =
                                        ((dim_ == ParticleDimension::Particle2D) ? fill2D() : fill3D());

                              if (layout_) {
                                        layout_->prepareTransferData(std::move(data));
                              }
                              cpuReady_ = true;
                    }

                    void updateUploadingStatus(uint64_t observedValue) {
                              if (pendingUpload_) {
                                        if (layout_) {
                                                  layout_->updateUploadingStatus(observedValue);
                                        }
                                        pendingUpload_ = false;
                              }
                    }

                    void purgeReleaseStaging(uint64_t observedValue) {
                              if (cpuReady_) {
                                        if (layout_) {
                                                  layout_->purgeReleaseStaging(observedValue);
                                        }
                                        cpuReady_ = false;
                              }
                    }

                    void setUploadCompleteTimeline(uint64_t value) {
                              if (layout_) {
                                        layout_->setUploadCompleteTimeline(value);
                              }
                    }

                    virtual std::vector<ParticleType> fill2D() {
                              std::vector<ParticleType> ret;
                              ret.resize(particleCount_ );
                              auto* src = ret.data();

                              for (std::size_t i = 0; i < particleCount_; ++i) {
                                        auto& particle = src[i];

                                        float r = std::sqrt(rndDist(mt));
                                        float theta = 2.0f * glm::pi<float>() * rndDist(mt);

                                        float x = r * cos(theta);
                                        float y = r * sin(theta);

                                        particle.position = glm::vec4(x, y, 0, 1);
                                        glm::vec3 pos = glm::vec3(particle.position);
                                        glm::vec3 vel = glm::normalize(glm::cross(pos, glm::vec3(0, 0, 1)));

                                        particle.velocity = glm::vec4(vel * 0.002f, 0.0f);
                                        particle.color = glm::vec4(rndDist(mt), rndDist(mt), rndDist(mt), 1.0f);
                              }
                              return ret;
                    }

                    virtual std::vector<ParticleType> fill3D() {
                              std::vector<ParticleType> ret;
                              ret.resize(particleCount_);
                              auto* src = ret.data();
                              for (std::size_t i = 0; i < particleCount_; ++i) {
                                        auto& particle = src[i];

                                        float u = rndDist(mt);
                                        float v = rndDist(mt);

                                        float theta = 2.0f * glm::pi<float>() * u; // 0~2pi
                                        float phi = acos(2.0f * v - 1);            //  0~π

                                        float sx = sin(phi) * cos(theta);
                                        float sy = sin(phi) * sin(theta);
                                        float sz = cos(phi);

                                        float r = cbrt(rndDist(mt));

                                        glm::vec3 direction = glm::vec3(sx, sy, sz);
                                        particle.position = glm::vec4(direction * r, 1.0f);
                                        particle.velocity = glm::vec4(direction * 0.0005f, 0.f);
                                        particle.color = glm::vec4(rndDist(mt), rndDist(mt), rndDist(mt), 1.0f);
                              }
                              return ret;
                    }

          private:
                    void __createGpuBuffer() {
                              if (gpuAllocated_)
                                        return;

                              if (!configure_)
                                        throw std::runtime_error("Please configure() first");

                              layout_.reset();
                              layout_ = std::make_unique< Layout>(device_, allocator_, name_ + std::string("::Layout"));
                              layout_->configure(particleCount_, bufferUsage_, memoryUsage_);
                              layout_->create();

                              gpuAllocated_ = true;
                    }

                    void __destroyGpuBuffer() {
                              if (gpuAllocated_) {
                                        if (layout_) {
                                                  layout_->destroy();
                                                  layout_.reset();
                                                  layout_ = nullptr;
                                        }
                                        gpuAllocated_ = false;
                              }
                    }


          protected:
                    /*one is for particle in, another is for particle out;
                     * buffer[0] will be the first one and it's going to swap in every frame
                     */
                    bool index = 0;
                    std::size_t particleCount_{ 0 };
                    ParticleDimension dim_;

                    VkBufferUsageFlags bufferUsage_ = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
                    VmaMemoryUsage memoryUsage_ = VMA_MEMORY_USAGE_GPU_ONLY;

          private:
                    bool configure_ = false;
                    bool gpuAllocated_ = false;
                    bool cpuReady_ = false;
                    bool pendingUpload_ = false;

                    std::once_flag upload_once_;
                    std::string name_ = "ParticleSysDataBuffer2";

                    VkDevice device_;
                    VmaAllocator allocator_;

                    std::random_device rd;
                    std::mt19937 mt;
                    std::uniform_real_distribution<float> rndDist;

                    std::unique_ptr<Layout> layout_;
          };

}

} // namespace engine

#endif //_PARTICLE_DATA_BUFFER_HPP_
