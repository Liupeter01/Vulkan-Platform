#include <Render.hpp>

engine::Render::Render(Window &window, MyEngineDevice &device)
    : window_(window), device_(device), currentFrameIdx_(0) {
  recreateSwapChain();
  createCommandBuffers();
}

engine::Render::~Render() { freeCommandBuffers(); }

void engine::Render::freeCommandBuffers() {
  vkFreeCommandBuffers(device_.device(), device_.getCommandPool(),
                       static_cast<uint32_t>(commandBuffers_.size()),
                       commandBuffers_.data());
  commandBuffers_.clear();
}

void engine::Render::createCommandBuffers() {
  commandBuffers_.resize(MyEngineSwapChain::MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers_.size());
  allocInfo.commandPool = device_.getCommandPool();

  if (vkAllocateCommandBuffers(device_.device(), &allocInfo,
                               commandBuffers_.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}

void engine::Render::recreateSwapChain() {
  auto extent = window_.getExtent();
  while (!extent.width || !extent.height) {
    extent = window_.getExtent();
    glfwWaitEvents();
  }
  vkDeviceWaitIdle(device_.device());

  if (swapChain_ == nullptr) {
    swapChain_.reset();
    swapChain_ = std::make_unique<MyEngineSwapChain>(device_, extent);
  } else {
    std::shared_ptr<MyEngineSwapChain> oldSwapChain = std::move(swapChain_);

    swapChain_ =
        std::make_unique<MyEngineSwapChain>(device_, extent, oldSwapChain);

    if (!oldSwapChain->compareSwapFormats(*swapChain_.get())) {
      throw std::runtime_error("Image/Depth format has changed!");
    }
  }
}

VkCommandBuffer engine::Render::beginFrame() {
  auto result = swapChain_->acquireNextImage(&currentImageIdx_);

  // WINDOW RESIZED
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return nullptr;
  }

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  isFrameStarted_ = true;

  auto commandBuffer = getCurrCommandBuffer();

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }
  return commandBuffer;
}

void engine::Render::endFrame() {
  assert(isFrameStarted_ &&
         "can not get framebuffer when frame is not started!");
  auto commandBuffer = getCurrCommandBuffer();
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }

  auto result =
      swapChain_->submitCommandBuffers(&commandBuffer, &currentImageIdx_);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      window_.wasWindowResized()) {
    window_.resetWindowResizedFlag();
    recreateSwapChain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }

  isFrameStarted_ = false;
  currentFrameIdx_ =
      (currentFrameIdx_ + 1) % MyEngineSwapChain::MAX_FRAMES_IN_FLIGHT;
}

void engine::Render::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = swapChain_->getRenderPass();
  renderPassInfo.framebuffer = swapChain_->getFrameBuffer(currentImageIdx_);

  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swapChain_->getSwapChainExtent();

  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
  clearValues[1].depthStencil = {1.0f, 0};

  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport{};
  viewport.x = viewport.y = 0.f;
  viewport.minDepth = 0.f;
  viewport.maxDepth = 1.f;
  viewport.width = static_cast<float>(swapChain_->getSwapChainExtent().width);
  viewport.height = static_cast<float>(swapChain_->getSwapChainExtent().height);

  VkRect2D scissor{{0, 0}, swapChain_->getSwapChainExtent()};
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void engine::Render::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
  assert(isFrameStarted_ &&
         "can not get framebuffer when frame is not started!");

  vkCmdEndRenderPass(commandBuffer);
}
