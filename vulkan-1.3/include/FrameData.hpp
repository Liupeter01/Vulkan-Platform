#pragma once
#ifndef _FRAME_DATA_HPP_
#define _FRAME_DATA_HPP_
#include <Descriptors.hpp>
#include <deque>
#include <functional>
#include <memory>

namespace engine {

class VulkanEngine;
struct AllocatedImage;

struct DeletionQueue {
  void push_function(std::function<void()> &&function);
  void flush();

private:
  std::deque<std::function<void()>> deletors;
};

struct FrameData {
  FrameData(VulkanEngine *eng);
  virtual ~FrameData();

  void destroy_sync();
  void destroy_command(bool needWaitIdle = true);
  void destroy_allocator();
  void destroy_images();

  void init_images(VkExtent3D extent);
  void init_allocator(const uint32_t setCount,
                      const std::vector<PoolSizeRatio> &poolSizeRatio);
  void init_graphic_command(const VkCommandPoolCreateInfo &commandPoolInfo);
  void init_compute_command(const VkCommandPoolCreateInfo& commandPoolInfo);
  void init_sync(const VkFenceCreateInfo &fenceCreateInfo,
                 const VkSemaphoreCreateInfo &semaphoreCreateInfo);

  void reset_allocator_pools();
  void destroy_by_deferred(std::function<void()> &&function);
  void clean_last_frame();
  VkDescriptorSet allocate(VkDescriptorSetLayout layout,
                           void *pNext = VK_NULL_HANDLE);

  void reset_images(VkExtent3D newExtent);

  VkExtent2D getExtent2D() const;

  VkFence _renderFinishedFence;
  VkFence _computeFinishedFence;

  //Highest Prioity! Before all Compute & Graphic
  VkSemaphore _swapChainWait;
  VkSemaphore _transferWait;       //before all the compute & graphic!
  VkSemaphore _computeWait;      //before graphic!

  VkCommandPool _graphicCommandPool;
  VkCommandPool _computeCommandPool;

  VkCommandBuffer _graphicCommandBuffer;
  VkCommandBuffer _computeCommandBuffer;

  DescriptorPoolAllocator _frameDescriptor;
  DeletionQueue _deletionQueue;

  std::unique_ptr<AllocatedImage> drawImage_ = nullptr;
  std::unique_ptr<AllocatedImage> depthImage_ = nullptr;

private:
  bool isGraphicCommandInit = false;
  bool isComputeCommandInit = false;
  bool isSyncInit = false;
  VulkanEngine *engine_;
  VkExtent3D oldExtent_{};
};
} // namespace engine

#endif //_FRAME_DATA_HPP_
