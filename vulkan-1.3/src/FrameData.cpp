#include <FrameData.hpp>
#include <Tools.hpp>

namespace engine {

void DeletionQueue::push_function(std::function<void()> &&function) {
  deletors.push_back(function);
}

void DeletionQueue::flush() {
  // reverse iterate the deletion queue to execute all the functions
  for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
    (*it)(); // call functors
  }

  deletors.clear();
}

FrameData::FrameData(VkDevice device)
    : _device(device), _frameDescriptor(device), isCommandInit(false),
      isSyncInit(false) {}

FrameData::~FrameData() {

          _deletionQueue.flush();
  destroy_command();
  destroy_sync();
  destroy_allocator();
}

void FrameData::init_command(const VkCommandPoolCreateInfo &commandPoolInfo) {
  if (isCommandInit)
    return;

  vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_commandPool);

  VkCommandBufferAllocateInfo cmdAllocInfo =
      tools::command_buffer_allocate_info(_commandPool, 1);

  vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_mainCommandBuffer);

  isCommandInit = true;
}

void FrameData::init_sync(const VkFenceCreateInfo &fenceCreateInfo,
                          const VkSemaphoreCreateInfo &semaphoreCreateInfo) {

  if (isSyncInit)
    return;

  vkCreateFence(_device, &fenceCreateInfo, nullptr, &_renderFinishedFence);
  vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_swapChainWait);
  vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr,
                    &_renderPresentKHRSignal);

  isSyncInit = true;
}

void FrameData::reset_allocator_pools() { _frameDescriptor.reset_pools(); }

void FrameData::destroy_by_deferred(std::function<void()> &&function) {
  _deletionQueue.push_function(std::move(function));
}

void FrameData::clean_last_frame() { _deletionQueue.flush(); }

void FrameData::init_allocator(
    const uint32_t setCount, const std::vector<PoolSizeRatio> &poolSizeRatio) {
  _frameDescriptor.init(setCount, poolSizeRatio);
}

void FrameData::destroy_allocator() { _frameDescriptor.destroy_pools(); }

VkDescriptorSet FrameData::allocate(VkDescriptorSetLayout layout, void *pNext) {
  return _frameDescriptor.allocate(layout, pNext);
}

void FrameData::destroy_command(bool needWaitIdle) {
  if (isCommandInit) {
    // make sure the gpu has stopped doing its things
    if (needWaitIdle) {
      vkDeviceWaitIdle(_device);
    }

    vkDestroyCommandPool(_device, _commandPool, nullptr);
    _commandPool = VK_NULL_HANDLE;

    isCommandInit = false;
  }
}

void FrameData::destroy_sync() {
  if (isSyncInit) {
    vkDestroyFence(_device, _renderFinishedFence, nullptr);
    vkDestroySemaphore(_device, _swapChainWait, nullptr);
    vkDestroySemaphore(_device, _renderPresentKHRSignal, nullptr);
    isSyncInit = false;
  }
}
} // namespace engine
