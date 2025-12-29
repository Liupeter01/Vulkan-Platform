#pragma once
#ifndef _PARTICLE_DATA_BUFFER_HPP_
#define _PARTICLE_DATA_BUFFER_HPP_
#include <GlobalDef.hpp>
#include <array>
#include <memory>
#include <random>
#include <type_traits>
#include <vulkan/vulkan.h>
#include <AllocatedBuffer.hpp>
#include <atomic>

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

// ParticleSysDataBuffer implements ping-pong GPU storage for particle simulation.
// - Two GPU buffers are allocated: buffers_[0] and buffers_[1].
// - get_in_buffer() reads from buffers_[index], get_out_buffer() writes to buffers_[index ^ 1].
// - swap() flips index each simulation step.
//
// Upload model:
// - perpareTransferData(): creates buffers if needed and prepares CPU staging for BOTH buffers,
//   so both ping-pong states start initialized (avoids reading uninitialized GPU memory).
// - singleTimeUpload(cmd): records GPU upload commands once (idempotent).
// - updateUploadingStatus(observedValue): advances internal resource state based on timeline.
// - purgeReleaseStaging(observedValue): releases CPU staging only after GPU upload is complete.

template <typename _Ty>
struct ParticleSysDataBuffer<
          _Ty, std::enable_if_t<has_particle_fields_v<_Ty>, void>> {

          using ParticleType = _Ty;

  enum class ParticleDimension { Particle2D, Particle3D };

  ParticleSysDataBuffer(VkDevice device, VmaAllocator allocator);
  virtual ~ParticleSysDataBuffer();

  void destroy();

  void configure(const std::size_t particle_count,
                    ParticleDimension dim = ParticleDimension::Particle3D,
                      VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                                                                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                                                  VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                                                  VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                      VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY
                     );

  void singleTimeUpload(VkCommandBuffer cmd);
  void perpareTransferData();
  void updateUploadingStatus(uint64_t observedValue);
  void purgeReleaseStaging(uint64_t observedValue);
  void setUploadCompleteTimeline(uint64_t value);

  std::size_t getParticleCount() const noexcept;
  std::size_t getBufferSize() const noexcept;
  void swap() noexcept;
  VkBuffer get_in_buffer() const noexcept;
  VkBuffer get_out_buffer() const noexcept;

  /*one is for particle in, another is for particle out;
   * buffer[0] will be the first one and it's going to swap in every frame
   */
  bool index = 0;
  std::size_t particleCount{0};
  std::size_t particleSize = sizeof(ParticleType);
  std::size_t particleBufferSize{ 0 };

  std::array<std::shared_ptr< ::engine::v2::AllocatedBuffer2>, 2> buffers_;

  //struct {
  //          std::array<std::shared_ptr< ::engine::v2::AllocatedBuffer2>, 2> position_;
  //          std::array<std::shared_ptr< ::engine::v2::AllocatedBuffer2>, 2> velocity_;
  //          std::array<std::shared_ptr< ::engine::v2::AllocatedBuffer2>, 2> color_;
  //} ParticleStorge;

protected:
          virtual std::vector<uint8_t> fill2D();
          virtual std::vector<uint8_t> fill3D();

private:
          void __createGpuBuffer();
          void __destroyGpuBuffer();

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
