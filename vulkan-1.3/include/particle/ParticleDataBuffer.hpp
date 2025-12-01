#pragma once
#include <vulkan/vulkan_core.h>
#ifndef _PARTICLE_DATA_BUFFER_HPP_
#define _PARTICLE_DATA_BUFFER_HPP_
#include <GlobalDef.hpp>
#include <array>
#include <memory>
#include <random>
#include <type_traits>

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

template <typename ParticleType>
struct ParticleSysDataBuffer<
    ParticleType, std::enable_if_t<has_particle_fields_v<ParticleType>, void>> {

  enum class ParticleDimension { Particle2D, Particle3D };

  ParticleSysDataBuffer(VkDevice device, VmaAllocator allocator,
                        ParticleDimension dim = ParticleDimension::Particle3D)
      : device_(device), allocator_(allocator), staging(allocator), dim_(dim) {
    mt.seed(static_cast<unsigned>(std::random_device{}()));
    rndDist = std::uniform_real_distribution<float>(0.f, 1.f);
  }

  virtual ~ParticleSysDataBuffer() { destroy(); }

  void destroy() {
    if (isinit_) {
      staging.destroy();
      for (auto &buf : buffers) {
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

  bool isValid() const { return isinit_; }

  void create(const std::size_t particle_count) {
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
    for (auto &buf : buffers) {
      buf.reset();
      buf = std::make_shared<AllocatedBuffer>(allocator_);
      buf->create(bufferSize,
                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                      VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,

                  VMA_MEMORY_USAGE_GPU_ONLY,
                  "ParticleSysDataBuffer::PingPongBuffer");
    }

    isinit_ = true;
  }

  void submit(VkCommandBuffer cmd) {

    // fill random data
    if (dim_ == ParticleDimension::Particle2D) {
      fill2D(staging.map(), particleCount);
    } else {
      fill3D(staging.map(), particleCount);
    }

    staging.unmap();

    VkBufferCopy bufferCopy{0, 0, getBufferSize()};
    vkCmdCopyBuffer(cmd, staging.buffer, buffers[0]->buffer, 1, &bufferCopy);

    pendingUpload_ = true;
  }

  void flush(VkFence fence) {
    if (!pendingUpload_)
      return;

    vkWaitForFences(device_, 1, &fence, true,
                    std::numeric_limits<uint64_t>::max());

    // staging.destroy();
    pendingUpload_ = false;
  }

  std::size_t getParticleCount() const noexcept { return particleCount; }

  std::size_t getBufferSize() const noexcept {
    return particleCount * particleSize;
  }

  void swap() noexcept {
    index ^= 1; // index = (index + 1) % 2
  }

  VkBuffer get_in_buffer() const noexcept { return buffers[index]->buffer; }

  VkBuffer get_out_buffer() const noexcept {
    return buffers[index ^ 1]->buffer;
  }

  /*one is for particle in, another is for particle out;
   * buffer[0] will be the first one and it's going to swap in every frame
   */
  bool index = 0;
  std::size_t particleCount{0};
  std::size_t particleSize = sizeof(ParticleType);
  std::array<std::shared_ptr<AllocatedBuffer>, 2> buffers;

protected:
  std::random_device rd;
  std::mt19937 mt;
  std::uniform_real_distribution<float> rndDist;

  // This is the staging buffer to init the first buffer slot
  AllocatedBuffer staging;

  virtual void fill2D(void *buffer, const std::size_t particle_count) {
    ParticleType *src = reinterpret_cast<ParticleType *>(buffer);
    memset(src, 0, particle_count * particleSize);

    for (std::size_t i = 0; i < particle_count; ++i) {
      auto &particle = src[i];

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
  }

  virtual void fill3D(void *buffer, const std::size_t particle_count) {
    ParticleType *src = reinterpret_cast<ParticleType *>(buffer);
    memset(src, 0, particle_count * particleSize);

    for (std::size_t i = 0; i < particle_count; ++i) {
      auto &particle = src[i];

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
  }

protected:
  bool isinit_ = false;
  VkDevice device_{};
  VmaAllocator allocator_{};
  ParticleDimension dim_;
  bool pendingUpload_ = false;
};
} // namespace engine

#endif //_PARTICLE_DATA_BUFFER_HPP_
