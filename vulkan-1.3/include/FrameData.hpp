#pragma once
#ifndef _FRAME_DATA_HPP_
#define _FRAME_DATA_HPP_
#include <Descriptors.hpp>
#include <deque>
#include <functional>

namespace engine {

struct DeletionQueue {
  void push_function(std::function<void()> &&function);
  void flush();

private:
  std::deque<std::function<void()>> deletors;
};

// Double Buffer
constexpr unsigned int FRAMES_IN_FLIGHT = 2;

struct FrameData {
  FrameData(VkDevice device);
  virtual ~FrameData();

  void destroy_sync();
  void destroy_command(bool needWaitIdle = true);
  void destroy_allocator();

  void init_allocator(const uint32_t setCount,
                      const std::vector<PoolSizeRatio> &poolSizeRatio);
  void init_command(const VkCommandPoolCreateInfo &commandPoolInfo);
  void init_sync(const VkFenceCreateInfo &fenceCreateInfo,
                 const VkSemaphoreCreateInfo &semaphoreCreateInfo);

  void reset_allocator_pools();
  void destroy_by_deferred(std::function<void()> &&function);
  void clean_last_frame();
  VkDescriptorSet allocate(VkDescriptorSetLayout layout,
                           void *pNext = VK_NULL_HANDLE);

  VkFence _renderFinishedFence;
  VkSemaphore _swapChainWait, _renderPresentKHRSignal;
  VkCommandPool _commandPool;
  VkCommandBuffer _mainCommandBuffer;
  DescriptorPoolAllocator _frameDescriptor;
  DeletionQueue _deletionQueue;

private:
  bool isCommandInit = false;
  bool isSyncInit = false;
  VkDevice _device;
};
} // namespace engine

#endif //_FRAME_DATA_HPP_
