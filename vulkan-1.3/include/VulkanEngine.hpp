#pragma once
#ifndef _VULKAN_ENGINE_HPP_
#define _VULKAN_ENGINE_HPP_
#include <Descriptors.hpp>
#include <GlobalDef.hpp>
#include <Window.hpp>
#include <iostream>
#include <vector>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

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
  void init();
  void destroy();

  void draw_background(VkCommandBuffer &cmd, VkImage &image);
  void draw_compute(VkCommandBuffer &cmd);

  FrameData &get_current_frame();
  void switch_to_next_frame();

private:
  void init_vulkan();
  void init_swapchain();
  void init_commands();
  void init_sync();
  void init_vma_allocator();
  void init_custom_image();
  void init_descriptors();
  void init_compute_pipeline();

  void destroy_compute_pipeline();
  void destroy_descriptors();
  void destroy_custom_image();
  void destroy_vma_allocator();
  void destroy_sync();
  void destroy_commands();
  void destroy_swapchain();
  void destroy_vulkan();
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

  struct AllocatedImage {
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
  };

  AllocatedImage drawImage_;
  VkExtent2D drawExtent_;

  VmaAllocator allocator_;

  // Initializing the layout and descriptors
  DescriptorAllocator descriptorAllocator_;
  VkDescriptorSet drawCompDescriptor_;
  VkDescriptorSetLayout drawCompDescriptorLayout_;

  // Compute Pipeline
  VkPipeline gradientComputePipeline_;
  VkPipelineLayout gradientComputePipelineLayout_;

  // Graphic Pipeline
};
} // namespace engine
#endif //_VULKAN_ENGINE_HPP_
