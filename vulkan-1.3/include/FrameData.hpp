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

struct FrameData;
struct CommonFrameContext {
          CommonFrameContext(FrameData* eng);
          virtual ~CommonFrameContext();

          void init(const VkFenceCreateInfo& fenceCreateInfo,
                    const VkCommandPoolCreateInfo& commandPoolInfo,
                    const uint32_t setCount,
                    const std::vector<PoolSizeRatio>& poolSizeRatio);
          void destroy(bool needWaitIdle = true);

          VkFence _finishedFence;
          VkCommandPool _commandPool;
          VkCommandBuffer _commandBuffer;
          DescriptorPoolAllocator _frameDescriptor;
          DeletionQueue _deletionQueue;

          void reset_allocator_pools();
          void destroy_by_deferred(std::function<void()>&& function);
          void clean_last_frame();
          VkDescriptorSet allocate(VkDescriptorSetLayout layout,
                    void* pNext = VK_NULL_HANDLE);

          FrameData* parent_;

protected:
          void init_fence(const VkFenceCreateInfo& fenceCreateInfo);
          void init_command(const VkCommandPoolCreateInfo& commandPoolInfo);
          void init_allocator(const uint32_t setCount,
                    const std::vector<PoolSizeRatio>& poolSizeRatio);
          void destroy_fence();
          void destroy_command();
          void destroy_allocator();

          bool isinit_ = false;
};

struct GraphicFrameContext: CommonFrameContext {
          GraphicFrameContext(FrameData* eng);
          virtual ~GraphicFrameContext();
};

struct ComputeFrameContext : CommonFrameContext {
          ComputeFrameContext(FrameData* eng);
          virtual ~ComputeFrameContext();
};

struct FrameData {
  
          friend struct CommonFrameContext;
          enum class ContextPass {
                    COMPUTE, GRAPHIC, TRANSFER
          };

  FrameData(VulkanEngine *eng);
  virtual ~FrameData();

  void init(VkExtent3D extent, const VkSemaphoreCreateInfo& semaphoreCreateInfo);
  void destroy();

  void reset_images(VkExtent3D newExtent);

  VkExtent2D getExtent2D() const;

  CommonFrameContext* get_context(ContextPass pass);

  VkDescriptorSet allocate(ContextPass pass, VkDescriptorSetLayout layout, void* pNext = VK_NULL_HANDLE);

  void reset_allocator_pools(ContextPass pass);
  void destroy_by_deferred(ContextPass pass, std::function<void()>&& func);

  void clean_last_frame(ContextPass pass);

  // ======== Synchronization Handles ========
  VkSemaphore _swapChainWait;   // Highest priority: Wait before all
  VkSemaphore _transferWait;    // Wait before compute/graphic
  VkSemaphore _computeWait;     // Wait before graphic

  // ======== Sub-contexts ========
  std::unordered_map<
            ContextPass,
            std::unique_ptr<CommonFrameContext>   //MUST NOT BE shared!
  > ctx;

  // ======== Frame Images ========
  std::unique_ptr<AllocatedImage> drawImage_ = nullptr;
  std::unique_ptr<AllocatedImage> depthImage_ = nullptr;

protected:

          void init_images(VkExtent3D extent);
          void init_sync(const VkSemaphoreCreateInfo& semaphoreCreateInfo);

          void destroy_sync();
          void destroy_images();

private:
          bool isinit_ = false;
  VulkanEngine *engine_;
  VkExtent3D oldExtent_{};
};
} // namespace engine

#endif //_FRAME_DATA_HPP_
