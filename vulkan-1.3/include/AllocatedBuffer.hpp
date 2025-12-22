#pragma once
#ifndef _ALLOCATED_BUFFER_HPP_
#define _ALLOCATED_BUFFER_HPP_
#include <GlobalDef.hpp>
#include <string>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace engine {
namespace v2 {

class AllocatedBuffer2 : public ResourcesStateManager {
public:
  AllocatedBuffer2(VmaAllocator allocator,
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

  // Prepare Cpu staging(Transfer src) data
  // Note:  recordUpload function WILL NOT ACCEPT DATA MODIFICATION
  void updateCpuStaging(const void *data, const std::size_t length);

  void purgeReleaseStaging(uint64_t observedValue);

  VkBuffer buffer();

protected:
  void __createGpuBuffer();
  void __destroyGpuBuffer();

protected:
  bool configured_{false};
  size_t allocSize_{0};

  // staging(CPU)
  bool cpuReady_{false};
  AllocatedBuffer staging_;

private:
  std::string name_ = "AllocatedBuffer2";
  VmaAllocator allocator_;

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
};
} // namespace v2
} // namespace engine

#endif // !_ALLOCATED_BUFFER_HPP_
