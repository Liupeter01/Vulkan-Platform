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

namespace engine {

class VulkanEngine;

template <typename ParticleType> struct remove_cvref {
  using type = std::remove_cv_t<std::remove_reference_t<ParticleType>>;
};

template <typename T> using remove_cvref_t = typename remove_cvref<T>::type;

template <typename ParticleType>
using __Particle_Trait =
    std::void_t<decltype(std::declval<remove_cvref_t<ParticleType>>().position),
                decltype(std::declval<remove_cvref_t<ParticleType>>().velocity),
                decltype(std::declval<remove_cvref_t<ParticleType>>().color)>;

template <typename T, typename = void>
struct has_particle_fields : std::false_type {};

template <typename T>
struct has_particle_fields<T, __Particle_Trait<T>> : std::true_type {};

template <typename T>
constexpr bool has_particle_fields_v = has_particle_fields<T>::value;

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

  enum class ParticleDimension { Particle2D, Particle3D };

  ParticleSysDataBuffer(VkDevice device, VmaAllocator allocator)
            : device_(device), allocator_(allocator) {
            mt.seed(static_cast<unsigned>(std::random_device{}()));
            rndDist = std::uniform_real_distribution<float>(0.f, 1.f);
  }

  virtual ~ParticleSysDataBuffer() {
            destroy();
  }

  void destroy() {
            __destroyGpuBuffer();
  }

  void configure(const std::size_t particle_count,
            ParticleDimension dim = ParticleDimension::Particle3D,
            VkBufferUsageFlags bufferUsage =
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY) 
  {
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

  std::size_t getParticleCount() const noexcept {
            return particleCount;
  }

  std::size_t getBufferSize() const noexcept {
            return particleBufferSize;
  }

  void swap() noexcept {
            index ^= 1; // index = (index + 1) % 2
  }

  VkBuffer get_in_buffer() const noexcept {
            return buffers_[index]->buffer();
  }

  VkBuffer get_out_buffer() const noexcept {
            return buffers_[index ^ 1]->buffer();
  }

  /*one is for particle in, another is for particle out;
   * buffer[0] will be the first one and it's going to swap in every frame
   */
  bool index = 0;
  std::size_t particleCount{0};
  std::size_t particleSize = sizeof(ParticleType);
  std::size_t particleBufferSize{0};

  std::array<std::shared_ptr<::engine::v2::AllocatedBuffer2>, 2> buffers_;

protected:
  virtual std::vector<uint8_t> fill2D(){
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
                      float phi = acos(2.0f * v - 1);            //  0~¦Ð

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
  void __createGpuBuffer(){
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
  bool uploadOnceFlag_ = false;
  bool cpuReady_ = false;
  bool pendingUpload_ = false;

  std::once_flag upload_once_;

  VkDevice device_{};
  VmaAllocator allocator_{};
};
} // namespace engine

#endif //_PARTICLE_DATA_BUFFER_HPP_
