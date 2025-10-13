#pragma once
#ifndef _VULKAN_ENGINE_HPP_
#define _VULKAN_ENGINE_HPP_
#include <Descriptors.hpp>
#include <GlobalDef.hpp>
#include <Window.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <pipeline/ComputePipeline.hpp>
#include <pipeline/GraphicPipeline.hpp>
#include <string>
#include <vector>

// IMGUI Support
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

namespace engine {
class VulkanEngine {
public:
  using CommandSubmitFunc = std::function<void(VkCommandBuffer)>;
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
  void init();
  void destroy();

  void imm_command_submit(CommandSubmitFunc &&function);

  FrameData &get_current_frame();
  void switch_to_next_frame();

private:
  void init_vulkan();
  void init_swapchain();
  void create_swapchain(uint32_t width, uint32_t height);
  void init_frames(const uint32_t setCount, const std::vector<PoolSizeRatio>& poolSizeRatio);
  void init_immediate_commands();
  void init_immediate_sync();
  void init_vma_allocator();
  void init_custom_image();
  void init_imgui();

  void destroy_imgui();
  void destroy_custom_image();
  void destroy_vma_allocator();
  void destroy_frames();
  void destroy_immediate_sync();
  void destroy_immediate_commands();
  void destroy_swapchain();
  void destroy_vulkan();

private:
  void resize_swapchain();
  void draw_background(VkCommandBuffer cmd, VkImage image);
  void draw_imgui(VkCommandBuffer cmd, VkExtent2D drawExtent,
                  VkImageView imageView = VK_NULL_HANDLE);

private:
  bool isInit = false;
  bool resize_requested = false;

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

  // Support IMGUI
  VkDescriptorPool imguiPool_ = VK_NULL_HANDLE;

  // CommandBuffer Part
  unsigned int frameNumber_ = 0;

  const uint32_t setCount_ = 1000;
  const std::vector<PoolSizeRatio> frame_sizes = {
    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
  };

  std::vector<std::unique_ptr<FrameData>> frames_;
  VkQueue graphicsQueue_;
  uint32_t graphicsQueueFamily_;

  VkExtent2D drawExtent_;
  std::unique_ptr<AllocatedImage> drawImage_ = nullptr;
  std::unique_ptr<AllocatedImage> depthImage_ = nullptr;
  float renderScale = 1.f;

  VmaAllocator allocator_;

  // immediate submit structures
  VkFence immFence_;
  VkCommandBuffer immCommandBuffer_;
  VkCommandPool immCommandPool_;

  // Compute Pipeline
  std::shared_ptr<PipelineBasic> computeEffect = nullptr; // Compute Pipeline
  std::shared_ptr<PipelineBasic> graphicEffect = nullptr; // Graphic Pipeline
};
} // namespace engine
#endif //_VULKAN_ENGINE_HPP_
