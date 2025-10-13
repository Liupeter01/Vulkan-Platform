#pragma once
#ifndef _GLOBALDEF_HPP_
#define _GLOBALDEF_HPP_
#include <Descriptors.hpp>
#include <FrameData.hpp>
#include <vk_mem_alloc.h>

#define GLM_FORCE_RADIANS // no degresss
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace engine {

struct AllocatedImage {
  VkImage image;
  VkImageView imageView;
  VmaAllocation allocation;
  VkExtent3D imageExtent;
  VkFormat imageFormat;

  AllocatedImage(VkDevice device, VmaAllocator allocator);
  virtual ~AllocatedImage();
  void create_as_depth(VkExtent3D extent);
  void create_as_draw(VkExtent3D extent);
  void destroy();

private:
  bool isinit_ = false;
  VkDevice device_;
  VmaAllocator allocator_;
};

struct AllocatedBuffer {
  AllocatedBuffer(VmaAllocator allocator);
  virtual ~AllocatedBuffer();

  VkBuffer buffer = VK_NULL_HANDLE;
  VmaAllocation allocation{};
  VmaAllocationInfo info{};

  void create(size_t allocSize, VkBufferUsageFlags usage,
              VmaMemoryUsage memoryUsage);
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
} // namespace engine

#endif //_GLOBALDEF_HPP_
