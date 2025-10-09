#pragma once
#ifndef _GLOBALDEF_HPP_
#define _GLOBALDEF_HPP_
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS // no degresss
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace engine {

// Double Buffer
constexpr unsigned int FRAMES_IN_FLIGHT = 2;

struct FrameData {
  VkFence _renderFinishedFence;
  VkSemaphore _swapChainWait, _renderPresentKHRSignal;
  VkCommandPool _commandPool;
  VkCommandBuffer _mainCommandBuffer;
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
