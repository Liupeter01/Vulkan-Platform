#pragma once
#ifndef _PARTICLE_DATA_BUFFER_HPP_
#define _PARTICLE_DATA_BUFFER_HPP_
#include <array>
#include <random>
#include <memory>
#include <type_traits>
#include <GlobalDef.hpp>

namespace engine {

          template<typename  ParticleType>
          struct remove_cvref{
                    using type = std::remove_cv_t<std::remove_reference_t<ParticleType>>;
          };

          template<typename T>
          using remove_cvref_t = typename remove_cvref<T>::type;

          template<typename  ParticleType>
          using __Particle_Trait = std::void_t<
                    decltype(std::declval<remove_cvref_t<ParticleType>>().position),
                    decltype(std::declval<remove_cvref_t<ParticleType>>().velocity),
                    decltype(std::declval<remove_cvref_t<ParticleType>>().color)>;

          template<typename T, typename = void>
          struct has_particle_fields : std::false_type {};  

          template<typename T>
          struct has_particle_fields<T, __Particle_Trait<T>> : std::true_type {}; 

          template<typename T>
          constexpr bool has_particle_fields_v = has_particle_fields<T>::value;

          template<typename ParticleType, typename = void>
          struct ParticleSysDataBuffer : std::false_type {
                    static_assert(sizeof(ParticleType) == 0,
                              "ParticleSysDataBuffer requires ParticleType with position, velocity, and color fields!");
          };

          template<typename ParticleType>
          struct ParticleSysDataBuffer<ParticleType , std::enable_if_t<has_particle_fields_v<ParticleType>, void>>{
                    ParticleSysDataBuffer(VulkanEngine* eng)
                              : engine_(eng), staging(eng->allocator_)
                    {
                              mt.seed(static_cast<unsigned>(std::random_device{}()));
                              rndDist = std::uniform_real_distribution<float>(0.f, 1.f);
                    }

                    virtual ~ParticleSysDataBuffer() { 
                              destroy(); 
                    }

                    void destroy() {
                              if (isinit_) {
                                        staging.destroy();
                                        for (auto& buf : buffers) {
                                                  buf->destroy();
                                                  buf.reset();
                                        }
                                        isinit_ = false;
                              }
                    }

                    void invalid() {
                              destroy();
                              pendingUpload_ = false;
                    }

                    bool isValid() const { 
                              return isinit_; 
                    }

                    void ParticleSysDataBuffer::create(const std::size_t particle_count) {
                              if (isinit_)
                                        return;

                              staging.destroy();

                              particleCount = particle_count;
                              const std::size_t bufferSize = particleSize * particle_count;

                              /*create staging buffer for upload*/
                              staging.create(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                        VMA_MEMORY_USAGE_CPU_ONLY,
                                        "ParticleSysDataBuffer::StagingBuffer");

                              /*Ping-Pong Swap Buffer*/
                              for (auto& buf : buffers) {
                                        buf.reset();
                                        buf = std::make_shared<AllocatedBuffer>(engine_->allocator_);
                                        buf->create(
                                                  bufferSize,
                                                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                  VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                  VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,

                                                  VMA_MEMORY_USAGE_GPU_ONLY, "ParticleSysDataBuffer::PingPongBuffer");
                              }

                              isinit_ = true;
                    }

                    void submit(VkCommandBuffer cmd) {
                              // fill random data
                              fill(staging.map(), particleCount);
                              staging.unmap();

                              VkBufferCopy bufferCopy{ 0, 0,getBufferSize() };
                              vkCmdCopyBuffer(cmd, staging.buffer, buffers[0]->buffer, 1, &bufferCopy);

                              pendingUpload_ = true;
                    }

                    void flush(VkFence fence) {
                              if (!pendingUpload_)
                                        return;

                              vkWaitForFences(engine_->device_, 1, &fence, true,
                                        std::numeric_limits<uint64_t>::max());

                              // staging.destroy();
                              pendingUpload_ = false;
                    }

                    std::size_t getParticleCount() const noexcept {
                              return particleCount;
                    }

                    std::size_t getBufferSize() const noexcept {
                              return particleCount * particleSize;
                    }

                    void swap() noexcept {
                              index ^= 1; // index = (index + 1) % 2
                    }

                    VkBuffer get_in_buffer() const noexcept {
                              return buffers[index]->buffer;
                    }

                    VkBuffer get_out_buffer() const noexcept {
                              return buffers[index ^ 1]->buffer;
                    }

                    /*one is for particle in, another is for particle out;
                     * buffer[0] will be the first one and it's going to swap in every frame
                     */
                    bool index = 0;
                    std::size_t particleCount{ 0 };
                    std::size_t particleSize = sizeof(ParticleType);
                    std::array<std::shared_ptr<AllocatedBuffer>, 2> buffers;

          protected:
                    std::random_device rd;
                    std::mt19937 mt;
                    std::uniform_real_distribution<float> rndDist;

                    // This is the staging buffer to init the first buffer slot
                    AllocatedBuffer staging;

                    void fill(void* buffer,
                              const std::size_t particle_count) {
                              ParticleType* src = reinterpret_cast<ParticleType*>(buffer);
                              memset(src, 0, particle_count * particleSize);

                              for (std::size_t i = 0; i < particle_count; ++i) {
                                        auto& particle = src[i];

                                       const float r = std::sqrtf(rndDist(mt));
                                        const float theta = 2.f * rndDist(mt) * glm::pi<float>();
                                        const float x = r * cosf(theta);
                                        const float y = r * sinf(theta);
                                        particle.position = glm::vec4(x, y, 0, 0);

                                        glm::vec2 dir = glm::vec2(x, y);
                                        if (glm::length(dir) < 1e-6f) {
                                                  float angle = 2.f * rndDist(mt) * glm::pi<float>();
                                                  dir = glm::vec2(cosf(angle), sinf(angle));
                                        }

                                        particle.velocity = glm::vec4(normalize(dir) * 0.00025f, 0.f, 0.f);
                                        particle.color = glm::vec4(rndDist(mt), rndDist(mt), rndDist(mt), 1.0f);
                              }
                    }

          private:
                    bool isinit_ = false;
                    VulkanEngine* engine_;
                    bool pendingUpload_ = false;
          };
}

#endif //_PARTICLE_DATA_BUFFER_HPP_