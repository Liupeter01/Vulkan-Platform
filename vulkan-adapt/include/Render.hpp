#pragma once
#ifndef _RENDER_HPP_
#define _RENDER_HPP_
#include <Device.hpp>
#include <Model.hpp>
#include <SwapChain.hpp>
#include <Window.hpp>

namespace engine {
struct Render {
  Render(Render &&) = delete;
  Render &operator=(Render &&) = delete;

  Render(Window &window, MyEngineDevice &device);
  virtual ~Render();

  VkCommandBuffer beginFrame();
  void endFrame();
  void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
  void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

  bool isFrameInProgress() const { return isFrameStarted_; }
  VkCommandBuffer getCurrCommandBuffer() const {
    assert(isFrameStarted_ &&
           "can not get framebuffer when frame is not started!");
    return commandBuffers_[currentFrameIdx_];
  }
  VkRenderPass getSwapChainRenderPass() const {
    return swapChain_->getRenderPass();
  }
  float getAspectRatio() const { return swapChain_->extentAspectRatio(); }
  int getFrameIndex() const { return currentFrameIdx_; }

protected:
  void recreateSwapChain();
  void createCommandBuffers();
  void freeCommandBuffers();

private:
  bool isFrameStarted_;
  uint32_t currentImageIdx_;
  int currentFrameIdx_;

  Window &window_;
  MyEngineDevice &device_;
  std::unique_ptr<MyEngineSwapChain> swapChain_;
  std::vector<VkCommandBuffer> commandBuffers_;
};
} // namespace engine

#endif //_RENDER_HPP_
