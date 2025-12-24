#pragma once
#ifndef _ALLOCATED_TEXTURE_HPP_
#define _ALLOCATED_TEXTURE_HPP_
#include <AllocatedBuffer.hpp>
#include <GlobalDef.hpp>
#include <string>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace engine {

namespace v1 {

struct AllocatedTexture {
  AllocatedTexture(VkDevice device, VmaAllocator allocator);
  virtual ~AllocatedTexture();
  void createBuffer(void *data, VkExtent3D size, VkFormat format,
                    VkImageUsageFlags usage, bool mipmapped = false);

  VkImage &getImage() const;
  VkImageView &getImageView() const;
  void uploadBufferToImage(VkCommandBuffer cmd);
  void flushUpload(VkFence fence);
  void invalid();
  bool isValid() const;
  void destroy();

private:
  bool isinit = false;
  bool pendingUpload_ = false;
  bool mipmapped_ = false;
  VkExtent3D extent_;
  mutable AllocatedImage dstImage_;
  ::engine::v1::AllocatedBuffer srcBuffer_;
  VkDevice device_;
  VmaAllocator allocator_;
};

} // namespace v1

namespace v2 {

class AllocatedTexture2 : public ResourcesStateManager {
public:
  AllocatedTexture2(VkDevice device, VmaAllocator allocator,
                    const std::string &name = "AllocatedTexture2");
  virtual ~AllocatedTexture2();

  // configure Image Size and their usage!
  void configure(VkExtent3D extent, VkFormat format,
                 VkImageUsageFlags imageUsage, bool mipmapped = false,
                 VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY);

  VkImage image();
  VkImageView imageView();

  void perpareTransferData(const void *data, const std::size_t length);

  void updateUploadingStatus(uint64_t observedValue);
  void purgeReleaseStaging(uint64_t observedValue);

  // virtual functions
  void recordUpload(VkCommandBuffer cmd) override;
  bool tryUninstall(uint64_t observedValue) override; // GPU =>Uninstall
  void forceUninstall() override;
  void destroy() override;

  bool createGpuImage();

protected:
  void __createGpuImage();
  void __destroyGpuImage();

  // Prepare Cpu Buffer & staging(Transfer src) data
  // Note:  recordUpload function WILL NOT ACCEPT DATA MODIFICATION
  void updateCpuStaging();
  void updateCpuBuffer(const void *data, const std::size_t length);

protected:
  std::string name_ = "AllocatedTexture2";

  // staging(CPU)
  bool cpuStaging_{false};
  ::engine::v1::AllocatedBuffer staging_;

private:
  bool configured_{false};
  bool gpuAllocated_{false};
  bool mipMapped_{false};
  bool pendingUpload_{false};

  VkDevice device_;
  VmaAllocator allocator_;

  VkImageUsageFlags imageUsage_;
  VmaMemoryUsage memoryUsage_;
  VkImageAspectFlags aspectFlag_;

  VkImage image_;
  VkImageView imageView_;
  VmaAllocation allocation_;
  VkExtent3D imageExtent_;
  VkFormat imageFormat_;

  // CPU-side texture data is owned by std::vector.
  // Vulkan buffers are used strictly for GPU transfer.
  std::size_t textureSize_{};
  std::vector<uint8_t> textureBuffer_;
};
} // namespace v2
} // namespace engine

#endif //_ALLOCATED_TEXTURE_HPP_
