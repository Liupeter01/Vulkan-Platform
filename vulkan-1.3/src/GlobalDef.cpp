#include <GlobalDef.hpp>
#include <Tools.hpp>

namespace engine {

AllocatedImage::AllocatedImage(VkDevice device, VmaAllocator allocator)
    : device_(device), allocator_(allocator), isinit_(false) {}

AllocatedImage::~AllocatedImage() { destroy(); }

void AllocatedImage::create_as_depth(VkExtent3D extent) {
  if (isinit_)
    return;
  imageFormat = VK_FORMAT_D32_SFLOAT;
  imageExtent = extent;

  VkImageUsageFlags depthImageUsages{};
  depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

  VkImageCreateInfo dimg_info =
      tools::image_create_info(imageFormat, depthImageUsages, extent);

  VmaAllocationCreateInfo rimg_allocinfo = {};
  rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  rimg_allocinfo.requiredFlags =
      VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  // allocate and create the image
  vmaCreateImage(allocator_, &dimg_info, &rimg_allocinfo, &image, &allocation,
                 nullptr);

  // build a image-view for the draw image to use for rendering
  VkImageViewCreateInfo rview_info = tools::imageview_create_info(
      imageFormat, image, VK_IMAGE_ASPECT_DEPTH_BIT);

  vkCreateImageView(device_, &rview_info, nullptr, &imageView);
  isinit_ = true;
}

void AllocatedImage::create_as_draw(VkExtent3D extent) {
  if (isinit_)
    return;

  // hardcoding the draw format to 32 bit float
  imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
  imageExtent = extent;

  VkImageUsageFlags drawImageUsages =
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  VkImageCreateInfo rimg_info =
      tools::image_create_info(imageFormat, drawImageUsages, extent);

  VmaAllocationCreateInfo rimg_allocinfo = {};
  rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  rimg_allocinfo.requiredFlags =
      VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  // allocate and create the image
  vmaCreateImage(allocator_, &rimg_info, &rimg_allocinfo, &image, &allocation,
                 nullptr);

  // build a image-view for the draw image to use for rendering
  VkImageViewCreateInfo rview_info = tools::imageview_create_info(
      imageFormat, image, VK_IMAGE_ASPECT_COLOR_BIT);

  vkCreateImageView(device_, &rview_info, nullptr, &imageView);
  isinit_ = true;
}

void AllocatedImage::destroy() {
  if (isinit_) {
    vkDestroyImageView(device_, imageView, nullptr);
    vmaDestroyImage(allocator_, image, allocation);
    isinit_ = false;
  }
}

AllocatedBuffer::AllocatedBuffer(VmaAllocator allocator)
    : allocator_(allocator), isinit(false) {}
AllocatedBuffer::~AllocatedBuffer() { destroy(); }

void AllocatedBuffer::create(size_t allocSize, VkBufferUsageFlags usage,
                             VmaMemoryUsage memoryUsage) {

  if (isinit)
    return;

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = allocSize;
  bufferInfo.usage = usage;

  VmaAllocationCreateInfo vmaallocInfo = {};
  vmaallocInfo.usage = memoryUsage;
  vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

  // allocate the buffer
  vmaCreateBuffer(allocator_, &bufferInfo, &vmaallocInfo, &buffer, &allocation,
                  &info);

  isinit = true;
}

void AllocatedBuffer::clear() {
  if (info.pMappedData && info.size > 0) {
    std::memset(info.pMappedData, 0, info.size);
  }
}
void AllocatedBuffer::reset(size_t newSize, VkBufferUsageFlags usage,
                            VmaMemoryUsage memoryUsage) {

  destroy();
  create(newSize, usage, memoryUsage);
}

void *AllocatedBuffer::map() {
  void *src;
  vmaMapMemory(allocator_, allocation, &src);
  return src;
}
void AllocatedBuffer::unmap() { vmaUnmapMemory(allocator_, allocation); }

void AllocatedBuffer::destroy() {
  if (isinit) {
    vmaDestroyBuffer(allocator_, buffer, allocation);
    isinit = false;
  }
}
} // namespace engine
