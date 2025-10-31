#pragma once
#ifndef _GLOBALDEF_HPP_
#define _GLOBALDEF_HPP_
#include <Descriptors.hpp>
#include <FrameData.hpp>
#include <string>
#include <vk_mem_alloc.h>

#define GLM_FORCE_RADIANS // no degresss
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace engine {
struct AllocatedBuffer;

struct EngineStats {
          float frametime{};
          int triangle_count{};
          int drawcall_count{};
          float scene_update_time{};
          float mesh_draw_time{};
};

struct AllocatedImage {
  VkImage image;
  VkImageView imageView;
  VmaAllocation allocation;
  VkExtent3D imageExtent;
  VkFormat imageFormat;

  AllocatedImage(VkDevice device, VmaAllocator allocator);
  virtual ~AllocatedImage();

  void create_image(VkExtent3D extent, VkFormat format, VkImageUsageFlags usage,
                    bool mipmapped = false,
                    const std::string &name = "AllocatedImage");

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
  VkExtent3D extent_;
  mutable AllocatedImage dstImage_;
  AllocatedBuffer srcBuffer_;
  VkDevice device_;
  VmaAllocator allocator_;
};

} // namespace engine

#endif //_GLOBALDEF_HPP_
