#include <FrameData.hpp>
#include <GlobalDef.hpp>
#include <Tools.hpp>
#include <VulkanEngine.hpp>
#include <spdlog/spdlog.h>

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

FrameData::FrameData(VulkanEngine *eng)
    : engine_(eng), _frameDescriptor(eng->device_), isCommandInit(false),
      isSyncInit(false) {

  if (!eng) {
    spdlog::error("[FrameData CTOR]: Invalid VulkanEngine!");
    std::abort();
  }
}

FrameData::~FrameData() {

  _deletionQueue.flush();
  destroy_command();
  destroy_sync();
  // destroy_images();
  destroy_allocator();
}

void FrameData::init_command(const VkCommandPoolCreateInfo &commandPoolInfo) {
  if (isCommandInit)
    return;

  vkCreateCommandPool(engine_->device_, &commandPoolInfo, nullptr,
                      &_commandPool);

  VkCommandBufferAllocateInfo cmdAllocInfo =
      tools::command_buffer_allocate_info(_commandPool, 1);

  vkAllocateCommandBuffers(engine_->device_, &cmdAllocInfo,
                           &_mainCommandBuffer);

  isCommandInit = true;
}

void FrameData::init_sync(const VkFenceCreateInfo &fenceCreateInfo,
                          const VkSemaphoreCreateInfo &semaphoreCreateInfo) {

  if (isSyncInit)
    return;

  vkCreateFence(engine_->device_, &fenceCreateInfo, nullptr,
                &_renderFinishedFence);
  vkCreateSemaphore(engine_->device_, &semaphoreCreateInfo, nullptr,
                    &_swapChainWait);
  vkCreateSemaphore(engine_->device_, &semaphoreCreateInfo, nullptr,
                    &_renderPresentKHRSignal);

  isSyncInit = true;
}

void FrameData::reset_allocator_pools() { _frameDescriptor.reset_pools(); }

VkDescriptorSet CommonFrameContext::allocate(VkDescriptorSetLayout layout,
                                             void *pNext) {
  return _frameDescriptor.allocate(layout, pNext);
}

GraphicFrameContext::GraphicFrameContext(FrameData *eng)
    : CommonFrameContext(eng) {}

GraphicFrameContext::~GraphicFrameContext() {}

ComputeFrameContext::ComputeFrameContext(FrameData *eng)
    : CommonFrameContext(eng) {}

ComputeFrameContext::~ComputeFrameContext() {}

FrameData::FrameData(VulkanEngine *eng) : engine_(eng) {

  if (!eng) {
    spdlog::error("[FrameData CTOR]: Invalid VulkanEngine!");
    std::abort();
  }
}

FrameData::~FrameData() { destroy(); }

void FrameData::init(VkExtent3D extent,
          const VkSemaphoreCreateInfo& binSemaphoreCreateInfo,
          const VkSemaphoreCreateInfo& timelineSemaphoreCreateInfo){
  if (isinit_)
    return;
  init_images(extent);
  init_sync(binSemaphoreCreateInfo);
  init_timeline(timelineSemaphoreCreateInfo);
  isinit_ = true;
}

void FrameData::destroy() {
  if (isinit_) {
            destroy_timeline();
    destroy_sync();
    destroy_images();
    ctx.clear();
    isinit_ = false;
  }
}

void FrameData::clean_last_frame() { _deletionQueue.flush(); }

  vkCreateSemaphore(engine_->device_, &semaphoreCreateInfo, nullptr,
                    &_swapChainWait);
}

void FrameData::init_timeline(const VkSemaphoreCreateInfo& semaphoreCreateInfo) {
          vkCreateSemaphore(engine_->device_, &semaphoreCreateInfo, nullptr,
                    &timelineSemaphore_);
}

void FrameData::destroy_allocator() { _frameDescriptor.destroy_pools(); }

VkDescriptorSet FrameData::allocate(VkDescriptorSetLayout layout, void *pNext) {
  return _frameDescriptor.allocate(layout, pNext);
}

void FrameData::destroy_command(bool needWaitIdle) {
  if (isCommandInit) {
    // make sure the gpu has stopped doing its things
    if (needWaitIdle) {
      vkDeviceWaitIdle(engine_->device_);
    }

    vkDestroyCommandPool(engine_->device_, _commandPool, nullptr);
    _commandPool = VK_NULL_HANDLE;

    isCommandInit = false;
  }
}

void FrameData::destroy_timeline() {
          vkDestroySemaphore(engine_->device_,timelineSemaphore_, nullptr);
}

void FrameData::destroy_sync() {
  vkDestroySemaphore(engine_->device_, _swapChainWait, nullptr);
}

void FrameData::init_images(VkExtent3D extent) {

  oldExtent_ = extent;

  drawImage_.reset();
  depthImage_.reset();

  drawImage_ =
      std::make_unique<AllocatedImage>(engine_->device_, engine_->allocator_);
  depthImage_ =
      std::make_unique<AllocatedImage>(engine_->device_, engine_->allocator_);

  drawImage_->create_image(
      extent, VK_FORMAT_R16G16B16A16_SFLOAT,
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
          VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

  depthImage_->create_image(extent, VK_FORMAT_D32_SFLOAT,
                            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void FrameData::destroy_images() {
  if (!drawImage_ || !depthImage_)
    throw std::runtime_error("Draw/Depth Images are null!");

  drawImage_->destroy();
  depthImage_->destroy();

  drawImage_.reset();
  depthImage_.reset();
}

void FrameData::reset_images(VkExtent3D newExtent) {

  // if (newExtent.depth * newExtent.height * newExtent.width <=
  //     oldExtent_.depth * oldExtent_.height * oldExtent_.width) {

  //  return;
  //}

  spdlog::info(
      "[FrameData]: Windows Resize width = {}, height = {}, depth = {}",
      newExtent.width, newExtent.height, newExtent.depth);

  destroy_images();
  init_images(newExtent);
}

VkExtent2D FrameData::getExtent2D() const {
  return {oldExtent_.width, oldExtent_.height};
}

} // namespace engine
