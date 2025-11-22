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

CommonFrameContext::CommonFrameContext(FrameData *eng)
    : parent_(eng), _frameDescriptor(eng->engine_->device_) {}

CommonFrameContext::~CommonFrameContext() { destroy(); }

void CommonFrameContext::destroy(bool needWaitIdle) {
  if (isinit_) {
    if (needWaitIdle) {
      vkDeviceWaitIdle(parent_->engine_->device_);
    }

    destroy_fence();
    destroy_command();
    clean_last_frame();
    destroy_allocator();
    isinit_ = false;
  }
}

void CommonFrameContext::init(const VkFenceCreateInfo &fenceCreateInfo,
                              const VkCommandPoolCreateInfo &commandPoolInfo,
                              const uint32_t setCount,
                              const std::vector<PoolSizeRatio> &poolSizeRatio) {

  if (isinit_)
    return;
  init_fence(fenceCreateInfo);
  init_command(commandPoolInfo);
  init_allocator(setCount, poolSizeRatio);
  isinit_ = true;
}

void CommonFrameContext::init_fence(const VkFenceCreateInfo &fenceCreateInfo) {
  vkCreateFence(parent_->engine_->device_, &fenceCreateInfo, nullptr,
                &_finishedFence);
}

void CommonFrameContext::destroy_fence() {
  vkDestroyFence(parent_->engine_->device_, _finishedFence, nullptr);
}

void CommonFrameContext::init_command(
    const VkCommandPoolCreateInfo &commandPoolInfo) {
  vkCreateCommandPool(parent_->engine_->device_, &commandPoolInfo, nullptr,
                      &_commandPool);

  VkCommandBufferAllocateInfo cmdAllocInfo =
      tools::command_buffer_allocate_info(_commandPool, 1);

  vkAllocateCommandBuffers(parent_->engine_->device_, &cmdAllocInfo,
                           &_commandBuffer);
}

void CommonFrameContext::destroy_command() {
  vkDestroyCommandPool(parent_->engine_->device_, _commandPool, nullptr);
}

void CommonFrameContext::init_allocator(
    const uint32_t setCount, const std::vector<PoolSizeRatio> &poolSizeRatio) {
  _frameDescriptor.init(setCount, poolSizeRatio);
}

void CommonFrameContext::destroy_allocator() {
  _frameDescriptor.destroy_pools();
}

void CommonFrameContext::reset_allocator_pools() {
  _frameDescriptor.reset_pools();
}

void CommonFrameContext::destroy_by_deferred(std::function<void()> &&function) {
  _deletionQueue.push_function(std::move(function));
}

void CommonFrameContext::clean_last_frame() { _deletionQueue.flush(); }

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
          const VkFenceCreateInfo& fenceCreateInfo,
                     const VkSemaphoreCreateInfo &binSemaphoreCreateInfo,
                     const VkSemaphoreCreateInfo &timelineSemaphoreCreateInfo) {
  if (isinit_)
    return;
  init_images(extent);
  init_sync(fenceCreateInfo, binSemaphoreCreateInfo);
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

void FrameData::init_sync(const VkFenceCreateInfo& fenceCreateInfo, const VkSemaphoreCreateInfo &semaphoreCreateInfo) {
          vkCreateFence(engine_->device_, &fenceCreateInfo, nullptr, &finalSyncFence_);
          vkCreateSemaphore(engine_->device_, &semaphoreCreateInfo, nullptr, &swapChainWait_);
          vkCreateSemaphore(engine_->device_, &semaphoreCreateInfo, nullptr, &graphicToPresent_);
}

void FrameData::init_timeline(
    const VkSemaphoreCreateInfo &semaphoreCreateInfo) {
  vkCreateSemaphore(engine_->device_, &semaphoreCreateInfo, nullptr,
                    &timelineSemaphore_);
}

CommonFrameContext *FrameData::get_context(ContextPass pass) {
  auto it = ctx.find(pass);
  return (it != ctx.end()) ? it->second.get() : nullptr;
}

VkDescriptorSet FrameData::allocate(ContextPass pass,
                                    VkDescriptorSetLayout layout, void *pNext) {
  auto *context = get_context(pass);
  return context ? context->allocate(layout, pNext) : VK_NULL_HANDLE;
}

void FrameData::reset_allocator_pools(ContextPass pass) {
  if (auto *context = get_context(pass)) {
    context->reset_allocator_pools();
  }
}

void FrameData::destroy_by_deferred(ContextPass pass,
                                    std::function<void()> &&func) {
  if (auto *context = get_context(pass)) {
    context->destroy_by_deferred(std::move(func));
  }
}

void FrameData::clean_last_frame(ContextPass pass) {
  if (auto *context = get_context(pass)) {
    context->clean_last_frame();
  }
}

void FrameData::destroy_timeline() {
  vkDestroySemaphore(engine_->device_, timelineSemaphore_, nullptr);
}

void FrameData::destroy_sync() {
          vkDestroyFence(engine_->device_, finalSyncFence_, nullptr);
          vkDestroySemaphore(engine_->device_, graphicToPresent_, nullptr);
  vkDestroySemaphore(engine_->device_, swapChainWait_, nullptr);
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
