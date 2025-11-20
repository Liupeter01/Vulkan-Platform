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
  void init_command(const VkCommandPoolCreateInfo &commandPoolInfo);
  void init_sync(const VkFenceCreateInfo &fenceCreateInfo,
                 const VkSemaphoreCreateInfo &semaphoreCreateInfo);

  void reset_allocator_pools();
  void destroy_by_deferred(std::function<void()> &&function);
  void clean_last_frame();
  VkDescriptorSet allocate(VkDescriptorSetLayout layout,
                           void *pNext = VK_NULL_HANDLE);

  FrameData *parent_;

protected:
  void init_fence(const VkFenceCreateInfo &fenceCreateInfo);
  void init_command(const VkCommandPoolCreateInfo &commandPoolInfo);
  void init_allocator(const uint32_t setCount,
                      const std::vector<PoolSizeRatio> &poolSizeRatio);
  void destroy_fence();
  void destroy_command();
  void destroy_allocator();

  bool isinit_ = false;
};

struct GraphicFrameContext : CommonFrameContext {
  GraphicFrameContext(FrameData *eng);
  virtual ~GraphicFrameContext();
};

struct ComputeFrameContext : CommonFrameContext {
  ComputeFrameContext(FrameData *eng);
  virtual ~ComputeFrameContext();
};

struct FrameData {

  friend struct CommonFrameContext;
  enum class ContextPass { COMPUTE, GRAPHIC, TRANSFER };

  FrameData(VulkanEngine *eng);
  virtual ~FrameData();

  void init(VkExtent3D extent,
            const VkSemaphoreCreateInfo &binSemaphoreCreateInfo,
            const VkSemaphoreCreateInfo& timelineSemaphoreCreateInfo);
  void destroy();

  void reset_images(VkExtent3D newExtent);

  VkExtent2D getExtent2D() const;

  VkFence _renderFinishedFence;
  VkSemaphore _swapChainWait, _renderPresentKHRSignal;
  VkCommandPool _commandPool;
  VkCommandBuffer _mainCommandBuffer;
  DescriptorPoolAllocator _frameDescriptor;
  DeletionQueue _deletionQueue;

  void clean_last_frame(ContextPass pass);

  // ======== Synchronization Handles ========
  VkSemaphore _swapChainWait; // Highest priority: Wait before all

  // ======== Timeline Synchronization Handles ========
  uint64_t timelineValue_{};
  
  uint64_t computeWaitValue_{};
  uint64_t computeSignalValue_{};

  uint64_t graphicsWaitValue_{};
  uint64_t graphicsSignalValue_{};

  uint64_t transferWaitValue_{};
  uint64_t transferSignalValue_{};

  uint64_t presentWaitValue_{};

  VkSemaphore timelineSemaphore_;

  // ======== Sub-contexts ========
  std::unordered_map<ContextPass,
                     std::unique_ptr<CommonFrameContext> // MUST NOT BE shared!
                     >
      ctx;

  // ======== Frame Images ========
  std::unique_ptr<AllocatedImage> drawImage_ = nullptr;
  std::unique_ptr<AllocatedImage> depthImage_ = nullptr;

protected:
  void init_images(VkExtent3D extent);
  void init_sync(const VkSemaphoreCreateInfo &semaphoreCreateInfo);
  void init_timeline(const VkSemaphoreCreateInfo& semaphoreCreateInfo);

  void destroy_timeline();
  void destroy_sync();
  void destroy_images();

private:
  bool isCommandInit = false;
  bool isSyncInit = false;
  VulkanEngine *engine_;
  VkExtent3D oldExtent_{};
};
} // namespace engine

#endif //_FRAME_DATA_HPP_
