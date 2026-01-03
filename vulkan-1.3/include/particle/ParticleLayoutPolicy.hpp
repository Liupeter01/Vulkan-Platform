#pragma once
#ifndef _PARTICLE_LAYOUT_POLICY_HPP_
#define _PARTICLE_LAYOUT_POLICY_HPP_
#include <array>
#include <string>
#include <vulkan/vulkan.h>
#include <AllocatedBuffer.hpp>
#include <particle/ParticleDefTraits.hpp>

namespace engine {
          namespace particle {
                    namespace policy {
                              namespace details {
                                        struct BufferArray2 {
                                                  BufferArray2(VkDevice dev, VmaAllocator alloc, std::string name = "BufferArray2");
                                                  virtual ~BufferArray2();

                                                  void create();
                                                  void recreate(const std::size_t  allocSize,
                                                            VkBufferUsageFlags bufferUsage,
                                                            VmaMemoryUsage memoryUsage);

                                                  void clear();

                                                  template<typename T>
                                                  void perpareTransferData(std::vector<T>&& data) {
                                                            static_assert(sizeof(T) > 0, "Type T must be a complete type");

                                                            if (!configure_)
                                                                      throw std::runtime_error("Please configure() first");

                                                            if (!gpuAllocated_) {
                                                                      __createGpuBuffer();
                                                            }

                                                            bufs[0]->perpareTransferData(data.data(), sizeof(T) * data.size());

                                                            cpuReady_ = true;
                                                  }

                                                  void setUploadCompleteTimeline(uint64_t value);
                                                  void purgeReleaseStaging(uint64_t observedValue);
                                                  void updateUploadingStatus(uint64_t observedValue);
                                                  void recordUpload(VkCommandBuffer cmd);

                                                  void destroy() noexcept;

                                                  VkBuffer in(bool idx)  const noexcept;
                                                  VkBuffer out(bool idx) const noexcept;

                                                  std::size_t getBufferSize() const { return allocSize_; }

                                                  void configure(const std::size_t allocSize,
                                                            VkBufferUsageFlags bufferUsage =
                                                            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                            VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY);

                                        private:
                                                  void __createGpuBuffer();
                                                  void __destroyGpuBuffer();

                                        private:
                                                  bool configure_ = false;
                                                  bool gpuAllocated_ = false;
                                                  bool cpuReady_ = false;
                                                  bool pendingUpload_ = false;

                                                  VkDevice device_;
                                                  VmaAllocator allocator_;

                                                  std::size_t  allocSize_{ 0 };
                                                  VkBufferUsageFlags bufferUsage_ =
                                                            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

                                                  VmaMemoryUsage memoryUsage_ = VMA_MEMORY_USAGE_GPU_ONLY;

                                                  std::string name_ = "BufferArray2";

                                                  std::array<std::shared_ptr<::engine::v2::AllocatedBuffer2>, 2> bufs{};
                                        };
                              }

                              template <typename ParticleT, typename = void>
                              struct AOSLayout;

                              template <typename ParticleT>
                              struct AOSLayout<ParticleT, std::enable_if_t<has_particle_fields_v<ParticleT>, void>> {

                                        using ParticleType = ParticleT;
                                        using Traits = ParticleLayoutTraits<ParticleT>;

                                        struct LayoutView {
                                                  LayoutView(VkBuffer buf, std::size_t size)
                                                            : packed(buf), bufferSize(size){}
                                                  VkBuffer packed{};

                                                  std::size_t bufferSize{};

                                                  static constexpr std::size_t stride = Traits::aos_stride;
                                                  static constexpr std::size_t   position_offset = Traits::position_offset;
                                                  static constexpr std::size_t   velocity_offset = Traits::velocity_offset;
                                                  static constexpr  std::size_t   color_offset = Traits::color_offset;
                                        };

                                        using view = LayoutView;

                                        AOSLayout(VkDevice device, VmaAllocator allocator, std::string name = "AOSLayout")
                                                  :device_(device), 
                                                  allocator_(allocator),
                                                  name_(name),
                                                  packed_(device, allocator)
                                        { }

                                        virtual ~AOSLayout() { destroy(); }

                                        void configure(const std::size_t particleCount,
                                                  VkBufferUsageFlags bufferUsage =
                                                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                  VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                  VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                  VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY) {

                                                  configure_ = false;

                                                  packed_.configure(
                                                            Traits::aos_stride * particleCount,
                                                            bufferUsage,
                                                            memoryUsage);

                                                  configure_ = true;
                                        }

                                        void create() {
                                                  if (gpuAllocated_) return;

                                                  packed_.create();

                                                  gpuAllocated_ = true;
                                        }

                                        void prepareTransferData(std::vector<ParticleType>&& data){

                                                  if (!configure_)
                                                            throw std::runtime_error("Please configure() first");

                                                  if (!gpuAllocated_) {
                                                            create();
                                                  }

                                                  packed_.perpareTransferData(std::move(data));

                                                  cpuReady_ = true;
                                        }

                                        void setUploadCompleteTimeline(uint64_t value){
                                                  packed_.setUploadCompleteTimeline(value);
                                        }

                                        void purgeReleaseStaging(uint64_t observedValue){
                                                  if (cpuReady_) {
                                                            packed_.purgeReleaseStaging(observedValue);
                                                            cpuReady_ = false;
                                                  }
                                        }

                                        void updateUploadingStatus(uint64_t observedValue){
                                                  if (pendingUpload_) {
                                                            packed_.updateUploadingStatus(observedValue);
                                                            pendingUpload_ = false;
                                                  }
                                        }

                                        void recordUpload(VkCommandBuffer cmd) {
                                                  if (!cpuReady_)
                                                            throw std::runtime_error("[BufferArray2]: CPU staging missing");

                                                  packed_.recordUpload(cmd);

                                                  pendingUpload_ = true;
                                        }

                                        void destroy()  { 
                                                  if (gpuAllocated_) {
                                                            packed_.destroy();

                                                            gpuAllocated_ = false;
                                                  }
                                        }

                                        LayoutView in(bool idx) const noexcept {
                                                  return LayoutView{ packed_.in(idx) , packed_.getBufferSize()};
                                        }

                                        LayoutView out(bool idx) const noexcept {
                                                  return  LayoutView{ packed_.out(idx) , packed_.getBufferSize() };
                                        }

                              private:
                                        bool configure_ = false;
                                        bool gpuAllocated_ = false;
                                        bool cpuReady_ = false;
                                        bool pendingUpload_ = false;

                                        std::string name_ = "AOSLayout";
                                        VkDevice device_;
                                        VmaAllocator allocator_;
                                        details::BufferArray2 packed_;
                              };

                              template <typename ParticleT, typename = void>
                              struct SOALayout;


                              template <typename ParticleT>
                              struct SOALayout<ParticleT, std::enable_if_t<has_particle_fields_v<ParticleT>, void>> {

                                        using ParticleType = ParticleT;
                                        using Traits = ParticleLayoutTraits<ParticleT>;

                                        struct LayoutView {
                                                  LayoutView(VkBuffer pos, VkBuffer v, VkBuffer c,
                                                            std::size_t pBufferSize, std::size_t vBufferSize, std::size_t cBufferSize)
                                                            :position(pos),velocity(v), color(c),
                                                            positionBufferSize(pBufferSize), velocityBufferSize(vBufferSize), colorBufferSize(cBufferSize)
                                                  { }

                                                  VkBuffer position;
                                                  VkBuffer velocity;
                                                  VkBuffer color;

                                                  std::size_t positionBufferSize{};
                                                  std::size_t velocityBufferSize{};
                                                  std::size_t colorBufferSize{};

                                                  static constexpr std::size_t   position_size = Traits::position_size;
                                                  static constexpr std::size_t   velocity_size = Traits::velocity_size;
                                                  static constexpr   std::size_t   color_size = Traits::color_size;
                                        };

                                        using view = LayoutView;

                                        SOALayout(VkDevice device, VmaAllocator allocator, std::string name = "SOALayout")
                                                  :device_(device),
                                                  allocator_(allocator),
                                                  name_(name),
                                                position_(device, allocator),
                                                  velocity_(device, allocator),
                                                  color_(device, allocator)
                                        { }

                                        virtual ~SOALayout() {
                                                  destroy();
                                        }

                                        void configure(const std::size_t particleCount,
                                                  VkBufferUsageFlags bufferUsage =
                                                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                  VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                  VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                  VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY) {

                                                  position_.configure(
                                                            Traits::position_size * particleCount,
                                                            bufferUsage,
                                                            memoryUsage);

                                                  velocity_.configure(
                                                            Traits::velocity_size * particleCount,
                                                            bufferUsage,
                                                            memoryUsage);

                                                  color_.configure(
                                                            Traits::color_size * particleCount,
                                                            bufferUsage,
                                                            memoryUsage);

                                                  configure_ = true;
                                        }

                                        void create() {
                                                  if (gpuAllocated_) return;
                                                  position_.create();
                                                  velocity_.create();
                                                  color_.create();
                                                  gpuAllocated_ = true;
                                        }

                                        void destroy() {
                                                  if (gpuAllocated_) {
                                                            position_.destroy();
                                                            velocity_.destroy();
                                                            color_.destroy();
                                                            gpuAllocated_ = false;
                                                  }
                                        }

                                        static void splitSOA(
                                                  const ParticleType* __restrict src,
                                                  size_t count,
                                                  glm::vec4* __restrict pos,
                                                  glm::vec4* __restrict vel,
                                                  glm::vec4* __restrict col
                                        ) {
                                                  for (size_t i = 0; i < count; ++i) {
                                                            pos[i] = src[i].position;
                                                            vel[i] = src[i].velocity;
                                                            col[i] = src[i].color;
                                                  }
                                        }

                                        void prepareTransferData(std::vector<ParticleType>&& data) {

                                                  static_assert(has_particle_fields_v<ParticleType>,
                                                            "ParticleType must have position, velocity and color fields");

                                                  if (!configure_)
                                                            throw std::runtime_error("Please configure() first");

                                                  if (!gpuAllocated_) {
                                                            create();
                                                  }

                                                  if(data.empty())
                                                            throw std::runtime_error("Dim Data Empty!");

                                                  using PositionType =  std::decay_t<decltype(data[0].position)>;
                                                  using VelocityType = std::decay_t<decltype(data[0].velocity)>;
                                                  using ColorType = std::decay_t<decltype(data[0].color)>;

                                                  const auto size = data.size();
                                                  std::vector<PositionType> position;
                                                  std::vector< VelocityType> velocity;
                                                  std::vector< ColorType> color;
                                                  position.resize(size);
                                                  velocity.resize(size);
                                                  color.resize(size);
                                                  splitSOA(data.data(), size,  position.data(), velocity.data(), color.data());

                                                  position_.perpareTransferData<PositionType>(std::move(position));
                                                  velocity_.perpareTransferData< VelocityType>(std::move(velocity));
                                                  color_.perpareTransferData<ColorType>(std::move(color));

                                                  cpuReady_ = true;
                                        }

                                        void setUploadCompleteTimeline(uint64_t value) {
                                                  position_.setUploadCompleteTimeline(value);
                                                  velocity_.setUploadCompleteTimeline(value);
                                                  color_.setUploadCompleteTimeline(value);
                                        }

                                        void purgeReleaseStaging(uint64_t observedValue) {
                                                  if (cpuReady_) {
                                                            position_.purgeReleaseStaging(observedValue);
                                                            velocity_.purgeReleaseStaging(observedValue);
                                                            color_.purgeReleaseStaging(observedValue);
                                                            cpuReady_ = false;
                                                  }
                                        }

                                        void updateUploadingStatus(uint64_t observedValue) {
                                                  if (pendingUpload_) {
                                                            position_.updateUploadingStatus(observedValue);
                                                            velocity_.updateUploadingStatus(observedValue);
                                                            color_.updateUploadingStatus(observedValue);
                                                            pendingUpload_ = false;
                                                  }
                                        }

                                        void recordUpload(VkCommandBuffer cmd) {
                                                  if (!cpuReady_)
                                                            throw std::runtime_error("[BufferArray2]: CPU staging missing");
                                                  position_.recordUpload(cmd);
                                                  velocity_.recordUpload(cmd);
                                                  color_.recordUpload(cmd);
                                                  pendingUpload_ = true;
                                        }

                                        LayoutView in(bool idx) const noexcept {
                                                  return {
                                                      position_.in(idx),
                                                      velocity_.in(idx),
                                                      color_.in(idx),
                                                      position_.getBufferSize(),
                                                      velocity_.getBufferSize(),
                                                       color_.getBufferSize()
                                                  };
                                        }

                                        LayoutView out(bool idx) const noexcept {
                                                  return {
                                                      position_.out(idx),
                                                      velocity_.out(idx),
                                                      color_.out(idx),
                                                                                                            position_.getBufferSize(),
                                                      velocity_.getBufferSize(),
                                                       color_.getBufferSize()
                                                  };
                                        }

                              private:
                                        bool gpuAllocated_ = false;
                                        bool configure_ = false;
                                        bool cpuReady_ = false;
                                        bool pendingUpload_ = false;

                                        std::string name_ = "SOALayout";
                                        VkDevice device_ = VK_NULL_HANDLE;
                                        VmaAllocator allocator_;

                                        details::BufferArray2 position_;
                                        details::BufferArray2 velocity_;
                                        details::BufferArray2 color_;
                              };
                    }
          }
}

#endif //_PARTICLE_LAYOUT_POLICY_HPP_