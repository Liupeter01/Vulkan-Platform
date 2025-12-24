#pragma once
#ifndef _ALLOCATED_BUFFER_HPP_
#define _ALLOCATED_BUFFER_HPP_
#include <GlobalDef.hpp>
#include <string>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace engine {

namespace v1 {

struct AllocatedBuffer {
  AllocatedBuffer(VmaAllocator allocator);
  virtual ~AllocatedBuffer();

  VkBuffer buffer = VK_NULL_HANDLE;
  VmaAllocation allocation{};
  VmaAllocationInfo info{};

  void create(size_t allocSize, VkBufferUsageFlags usage,
              VmaMemoryUsage memoryUsage,
              const std::string &name = "AllocatedBuffer");
  void destroy();

  void *map();
  void unmap();
  void clear();
  void reset(size_t newSize, VkBufferUsageFlags usage,
             VmaMemoryUsage memoryUsage);

private:
  bool isinit = false;
  VmaAllocator allocator_;
};

} // namespace v1

namespace v2 {

class AllocatedBuffer2 : public ResourcesStateManager {
public:
  AllocatedBuffer2(VkDevice device, VmaAllocator allocator,
                   const std::string &name = "AllocateBuffer2");

  virtual ~AllocatedBuffer2();

  // virtual functions
  void recordUpload(VkCommandBuffer cmd) override;
  bool tryUninstall(uint64_t observedValue) override; // GPU =>Uninstall
  void forceUninstall() override;
  void destroy() override;

  // configure Gpu (staging) Buffer Size and their usage!
  void configure(const size_t allocSize,
                 VkBufferUsageFlags bufferUsage =
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                 VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY);

  void perpareTransferData(const void *data, const std::size_t length);

  void updateUploadingStatus(uint64_t observedValue);
  void purgeReleaseStaging(uint64_t observedValue);

  VkBuffer buffer();

  VkDeviceAddress getBufferDeviceAddress();

protected:
  void __createGpuBuffer();
  void __destroyGpuBuffer();

  // Prepare Cpu Buffer & staging(Transfer src) data
  // Note:  recordUpload function WILL NOT ACCEPT DATA MODIFICATION
  void updateCpuStaging();
  void updateCpuBuffer(const void *data, const std::size_t length);

protected:
  bool configured_{false};
  size_t allocSize_{0};

  // staging(CPU)
  bool cpuReady_{false};
  ::engine::v1::AllocatedBuffer staging_;

private:
  std::string name_ = "AllocatedBuffer2";
  VmaAllocator allocator_;
  VkDevice device_ = VK_NULL_HANDLE;

  VkBufferUsageFlags bufferUsage_ = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

  VmaMemoryUsage memoryUsage_ = VMA_MEMORY_USAGE_GPU_ONLY;

  // GPU buffer
  VkBuffer buffer_{VK_NULL_HANDLE};
  VmaAllocation allocation_{};
  VmaAllocationInfo allocInfo_{};
  bool gpuAllocated_{false};

  bool pendingUpload_{false};

  // CPU-side texture data is owned by std::vector.
  // Vulkan buffers are used strictly for GPU transfer.
  std::vector<uint8_t> cpuBuffer_;
};
} // namespace v2
} // namespace engine

#endif // !_ALLOCATED_BUFFER_HPP_
