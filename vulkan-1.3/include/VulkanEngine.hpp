#pragma once
#include <vulkan/vulkan_core.h>
#ifndef _VULKAN_ENGINE_HPP_
#define _VULKAN_ENGINE_HPP_
#include <GlobalDef.hpp>
#include <Window.hpp>
#include <iostream>
#include <vector>

namespace engine {
class VulkanEngine {
public:
  VulkanEngine(Window &win, bool enableValidationLayer = true);
  virtual ~VulkanEngine();
  VulkanEngine(VulkanEngine &&) = delete;
  VulkanEngine &operator=(VulkanEngine &&) = delete;

public:
  void run();
  [[nodiscard]] std::vector<const char *> getRequiredExtensions();

protected:
  // draw loop
  void draw();

  void init_vulkan();
  void init_swapchain();
  void init_commands();
  void init_sync();

  void destroy_sync();
  void destroy_commands();
  void destroy_swapchain();
  void destroy_vulkan();

  FrameData &get_current_frame();
  void switch_to_next_frame();

private:
  void create_swapchain(uint32_t width, uint32_t height);

private:
  bool isInit = false;

#if ENABLE_VALIDATION_LAYERS
  bool enableValidationLayers_ = true;
#else
  bool enableValidationLayers_ = false;
#endif

  Window &window_;

  VkInstance instance_; // Vulkan library handle
  VkDebugUtilsMessengerEXT debugMessenger_ =
      VK_NULL_HANDLE; // Vulkan debug output handle
  VkPhysicalDevice physicalDevice_ =
      VK_NULL_HANDLE; // GPU chosen as the default device

  VkDevice device_;      // Vulkan device for commands
  VkSurfaceKHR surface_; // Vulkan window surface

  // SwapChain Part
  VkSwapchainKHR swapchain_;
  VkFormat swapchainImageFormat_ = VK_FORMAT_B8G8R8A8_UNORM;

  std::vector<VkImage> swapchainImages_;         //
  std::vector<VkImageView> swapchainImageViews_; //
  VkExtent2D swapchainExtent_;

  // CommandBuffer Part
  unsigned int frameNumber_ = 0;
  std::vector<FrameData> frames_;

  VkQueue graphicsQueue_;
  uint32_t graphicsQueueFamily_;
};
} // namespace engine
#endif //_VULKAN_ENGINE_HPP_
