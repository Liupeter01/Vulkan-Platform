#include <Util.hpp>
#include <VkBootstrap.h>
#include <VulkanEngine.hpp>
#include <chrono>
#include <numeric>

namespace engine {

VulkanEngine::VulkanEngine(Window &win, bool enableValidationLayer)
    : isInit(false), window_(win), frameNumber_(0), frames_(FRAMES_IN_FLIGHT),
      enableValidationLayers_(enableValidationLayer) {

  init_vulkan();
  init_swapchain();
  init_sync();
  init_commands();
}

VulkanEngine::~VulkanEngine() {
  destroy_commands();
  destroy_sync();
  destroy_swapchain();
  destroy_vulkan();
}

void VulkanEngine::run() {

  auto currTime = std::chrono::high_resolution_clock::now();

  while (!window_.shouldClose()) {
    glfwPollEvents();

    auto nowTime = std::chrono::high_resolution_clock::now();
    auto timeFrame = std::chrono::duration<float, std::chrono::seconds::period>(
                         nowTime - currTime)
                         .count();
    currTime = nowTime;

    draw();
  }

  vkDeviceWaitIdle(device_);
}

void VulkanEngine::draw() {
  auto &currentFrame = get_current_frame();

  // wait until the gpu has finished rendering the last frame.
  vkWaitForFences(device_, 1, &currentFrame._renderFinishedFence, true,
                  std::numeric_limits<float>::max());
  vkResetFences(device_, 1, &currentFrame._renderFinishedFence);

  // request image from the swapchain
  uint32_t swapchainImageIndex{};
  vkAcquireNextImageKHR(device_, swapchain_, std::numeric_limits<float>::max(),
                        currentFrame._swapChainWait, nullptr,
                        &swapchainImageIndex);

  // now that we are sure that the commands finished executing, we can safely
  VkCommandBuffer cmd = currentFrame._mainCommandBuffer;

  // reset the command buffer to begin recording again.
  vkResetCommandBuffer(cmd, 0);

  // use command buffer exactly once
  VkCommandBufferBeginInfo cmdBeginInfo = tools::command_buffer_begin_info(
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  vkBeginCommandBuffer(cmd, &cmdBeginInfo);

  // SwapChain Image
  VkImage &image = swapchainImages_[swapchainImageIndex];
  util::transition_image(cmd, image, VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_GENERAL);

  // make a clear-color from frame number. This will flash with a 120 frame
  // period.
  VkClearColorValue clearValue;
  float flash = std::abs(std::sin(frameNumber_ / 120.f));
  clearValue = {{0.0f, 0.0f, flash, 1.0f}};

  VkImageSubresourceRange clearRange =
      tools::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

  // clear image
  vkCmdClearColorImage(cmd, image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1,
                       &clearRange);

  // make the swapchain image into presentable mode
  util::transition_image(cmd, image, VK_IMAGE_LAYOUT_GENERAL,
                         VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  vkEndCommandBuffer(cmd);

  VkCommandBufferSubmitInfo cmdinfo = tools::command_buffer_submit_info(cmd);

  VkSemaphoreSubmitInfo swapChainImageWait = tools::semaphore_submit_info(
      VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
      get_current_frame()._swapChainWait);

  VkSemaphoreSubmitInfo presentKHRSignal =
      tools::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                                   get_current_frame()._renderPresentKHRSignal);

  VkSubmitInfo2 info = {};
  info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;

  info.waitSemaphoreInfoCount = info.signalSemaphoreInfoCount = 1;

  info.pWaitSemaphoreInfos = &swapChainImageWait;
  info.pSignalSemaphoreInfos = &presentKHRSignal;

  info.commandBufferInfoCount = 1;
  info.pCommandBufferInfos = &cmdinfo;

  vkQueueSubmit2(graphicsQueue_, 1, &info,
                 get_current_frame()._renderFinishedFence);

  // we want to wait on the _renderSemaphore for that,
  // as its necessary that drawing commands have finished before the image is
  // displayed to the user
  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapchain_;

  presentInfo.pWaitSemaphores = &get_current_frame()._renderPresentKHRSignal;
  presentInfo.waitSemaphoreCount = 1;

  presentInfo.pImageIndices = &swapchainImageIndex;

  vkQueuePresentKHR(graphicsQueue_, &presentInfo);

  switch_to_next_frame();
}

std::vector<const char *> VulkanEngine::getRequiredExtensions() {
  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char *> extensions(glfwExtensions,
                                       glfwExtensions + glfwExtensionCount);

  if (enableValidationLayers_) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

#ifdef __APPLE__
  extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
  extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#if defined(VK_EXT_METAL_SURFACE_EXTENSION_NAME)
  extensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
#else
  extensions.push_back("VK_MVK_macos_surface");
#endif

  createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

  return extensions;
}

void VulkanEngine::init_vulkan() {

  vkb::InstanceBuilder builder;
  auto ins_ret = builder.set_app_name("Vulkan_Application")
                     .request_validation_layers(enableValidationLayers_)
                     .use_default_debug_messenger()
                     .require_api_version(1, 3, 0)
                     .enable_extensions(getRequiredExtensions())
                     .build();

  vkb::Instance vkb_inst = ins_ret.value();

  instance_ = vkb_inst.instance;
  debugMessenger_ = vkb_inst.debug_messenger;

  // Create VkSurfaceKHR
  window_.createWindowSurface(instance_, &surface_);

  // Choose Device
  vkb::PhysicalDeviceSelector selector{vkb_inst};

  // vulkan 1.2 features
  VkPhysicalDeviceVulkan12Features features12;
  features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  features12.bufferDeviceAddress = true;
  features12.descriptorIndexing = true;

  // vulkan 1.3 features
  VkPhysicalDeviceVulkan13Features features;
  features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
  features.dynamicRendering = true;
  features.synchronization2 = true;

  vkb::PhysicalDevice _physicalDevice =
      selector.set_minimum_version(1, 3)
          .set_required_features_13(features)
          .set_required_features_12(features12)
          .set_surface(surface_)
          .select()
          .value();

  vkb::DeviceBuilder deviceBuilder{_physicalDevice};
  vkb::Device vkbDevice = deviceBuilder.build().value();

  physicalDevice_ = _physicalDevice.physical_device;
  device_ = vkbDevice.device;

  graphicsQueue_ = vkbDevice.get_queue(vkb::QueueType::graphics).value();
  graphicsQueueFamily_ =
      vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

  isInit = true;
}

void VulkanEngine::init_swapchain() {
  create_swapchain(window_.getExtent().width, window_.getExtent().height);
}

void VulkanEngine::init_commands() {
  VkCommandPoolCreateInfo commandPoolInfo = tools::command_pool_create_info(
      graphicsQueueFamily_, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  for (auto &frame : frames_) {
    vkCreateCommandPool(device_, &commandPoolInfo, nullptr,
                        &frame._commandPool);

    VkCommandBufferAllocateInfo cmdAllocInfo =
        tools::command_buffer_allocate_info(frame._commandPool, 1);

    vkAllocateCommandBuffers(device_, &cmdAllocInfo, &frame._mainCommandBuffer);
  }
}

void VulkanEngine::init_sync() {
  // DeadLock Prevention!!!
  VkFenceCreateInfo fenceCreateInfo =
      tools::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
  VkSemaphoreCreateInfo semaphoreCreateInfo = tools::semaphore_create_info();

  for (auto &frame : frames_) {
    vkCreateFence(device_, &fenceCreateInfo, nullptr,
                  &frame._renderFinishedFence);
    vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr,
                      &frame._swapChainWait);
    vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr,
                      &frame._renderPresentKHRSignal);
  }
}

void VulkanEngine::create_swapchain(uint32_t width, uint32_t height) {
  vkb::SwapchainBuilder swapchainBuilder{physicalDevice_, device_, surface_};
  VkSurfaceFormatKHR formatKHR{};
  formatKHR.format = swapchainImageFormat_;
  formatKHR.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

  vkb::Swapchain vkbSwapchain =
      swapchainBuilder
          //.use_default_format_selection()
          .set_desired_format(formatKHR)
          .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // V-Sync
          .set_desired_extent(width, height)
          .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
          .build()
          .value();

  swapchainExtent_ = vkbSwapchain.extent;
  swapchain_ = vkbSwapchain.swapchain;
  swapchainImages_ = vkbSwapchain.get_images().value();
  swapchainImageViews_ = vkbSwapchain.get_image_views().value();
}

void VulkanEngine::destroy_vulkan() {

  if (!isInit)
    return;
  vkDestroySurfaceKHR(instance_, surface_, nullptr);
  vkDestroyDevice(device_, nullptr);

  vkb::destroy_debug_utils_messenger(instance_, debugMessenger_);
  vkDestroyInstance(instance_, nullptr);
}

void VulkanEngine::destroy_swapchain() {
  vkDestroySwapchainKHR(device_, swapchain_, nullptr);

  // destroy swapchain resources
  for (int i = 0; i < swapchainImageViews_.size(); i++) {

    vkDestroyImageView(device_, swapchainImageViews_[i], nullptr);
  }
}

void VulkanEngine::destroy_sync() {
  for (auto &frame : frames_) {
    vkDestroyFence(device_, frame._renderFinishedFence, nullptr);
    vkDestroySemaphore(device_, frame._swapChainWait, nullptr);
    vkDestroySemaphore(device_, frame._renderPresentKHRSignal, nullptr);
  }
}

void VulkanEngine::destroy_commands() {
  if (isInit) {
    // make sure the gpu has stopped doing its things
    vkDeviceWaitIdle(device_);

    for (auto &frame : frames_) {
      vkDestroyCommandPool(device_, frame._commandPool, nullptr);
    }
  }
}

FrameData &VulkanEngine::get_current_frame() {
  return frames_[frameNumber_ % FRAMES_IN_FLIGHT];
}
void VulkanEngine::switch_to_next_frame() { ++frameNumber_; }
} // namespace engine
