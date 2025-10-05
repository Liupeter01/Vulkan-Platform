#include <Util.hpp>
#include <VkBootstrap.h>
#include <VulkanEngine.hpp>
#include <chrono>
#include <numeric>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

namespace engine {

VulkanEngine::VulkanEngine(Window &win, bool enableValidationLayer)
    : isInit(false), window_(win), frameNumber_(0), frames_(FRAMES_IN_FLIGHT),
      enableValidationLayers_(enableValidationLayer) {

  init();
}

VulkanEngine::~VulkanEngine() { destroy(); }

void VulkanEngine::init() {
  init_vulkan();
  init_swapchain();
  init_sync();
  init_commands();
  init_vma_allocator();
  init_custom_image();
  init_descriptors();
  init_compute_pipeline();
}

void VulkanEngine::destroy() {
  destroy_compute_pipeline();
  destroy_descriptors();
  destroy_custom_image();
  destroy_vma_allocator();
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

void VulkanEngine::draw_background(VkCommandBuffer &cmd, VkImage &image) {
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
}

void VulkanEngine::draw_compute(VkCommandBuffer &cmd) {

  vkCmdBindPipeline(cmd, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE,
                    gradientComputePipeline_);

  // bind the descriptor set containing the draw image for the compute pipeline
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                          gradientComputePipelineLayout_, 0, 1,
                          &drawCompDescriptor_, 0, nullptr);

  // execute the compute pipeline dispatch. We are using 16x16 workgroup size so
  // we need to divide by it
  vkCmdDispatch(
      cmd, static_cast<uint32_t>(std::ceil(drawExtent_.width / 16.0f)),
      static_cast<uint32_t>(std::ceil(drawExtent_.height / 16.0f)), 1);
}

void VulkanEngine::draw() {
  auto &currentFrame = get_current_frame();

  // wait until the gpu has finished rendering the last frame.
  vkWaitForFences(device_, 1, &currentFrame._renderFinishedFence, true,
                  std::numeric_limits<uint64_t>::max());
  vkResetFences(device_, 1, &currentFrame._renderFinishedFence);

  // request image from the swapchain
  uint32_t swapchainImageIndex{};
  vkAcquireNextImageKHR(
      device_, swapchain_, std::numeric_limits<uint64_t>::max(),
      currentFrame._swapChainWait, nullptr, &swapchainImageIndex);

  // now that we are sure that the commands finished executing, we can safely
  VkCommandBuffer cmd = currentFrame._mainCommandBuffer;

  // reset the command buffer to begin recording again.
  vkResetCommandBuffer(cmd, 0);

  // use command buffer exactly once
  VkCommandBufferBeginInfo cmdBeginInfo = tools::command_buffer_begin_info(
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  drawExtent_.width = drawImage_.imageExtent.width;
  drawExtent_.height = drawImage_.imageExtent.height;

  vkBeginCommandBuffer(cmd, &cmdBeginInfo);

  VkImage &src_image = drawImage_.image; // Draw Image
  VkImage &target_image =
      swapchainImages_[swapchainImageIndex]; // SwapChain Image

  // transition our main draw image into general layout so we can write into it
  // we will overwrite it all so we dont care about what was the older layout
  util::transition_image(cmd, src_image, VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_GENERAL);

  // Draw Background
  // draw_background(cmd, src_image);

  // compute
  draw_compute(cmd);

  // transition the draw image and the swapchain image into their correct
  // transfer layouts
  util::transition_image(cmd, src_image, VK_IMAGE_LAYOUT_GENERAL,
                         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  util::transition_image(cmd, target_image, VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  // execute a copy from the draw image into the swapchain
  util::copy_image_to_image(cmd, src_image, target_image, drawExtent_,
                            swapchainExtent_);

  // set swapchain image layout to Present so we can show it on the screen
  util::transition_image(cmd, target_image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
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
  const char **glfwExtensions =
      glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char *> extensions;
  if (glfwExtensions) {
    extensions.insert(extensions.end(), glfwExtensions,
                      glfwExtensions + glfwExtensionCount);
  }
#if ENABLE_VALIDATION_LAYERS
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

#ifdef __APPLE__
  extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

  if (std::find(extensions.begin(), extensions.end(),
                VK_KHR_SURFACE_EXTENSION_NAME) == extensions.end()) {
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
  }

#if defined(VK_EXT_METAL_SURFACE_EXTENSION_NAME)
  extensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
#else
  extensions.push_back("VK_MVK_macos_surface");
#endif

#endif

#if ENABLE_VALIDATION_LAYERS
  for (auto ext : extensions) {
    std::cout << "Requesting instance extension: " << ext << std::endl;
  }
#endif

  return extensions;
}

void VulkanEngine::init_vulkan() {

  vkb::InstanceBuilder builder;

  auto ins_ret = builder
                     .set_app_name("Vulkan_Application")
#if ENABLE_VALIDATION_LAYERS
                     .request_validation_layers(true)
                     .use_default_debug_messenger()
#endif
                     .require_api_version(1, 3, 0)
                     .enable_extensions(getRequiredExtensions())
                     .build();

  if (!ins_ret) {
    auto err = ins_ret.error();
    std::cerr << "Failed to create Vulkan instance: " << err.message() << " ("
              << err.message() << ")" << std::endl;
    std::abort();
  }

  if (!ins_ret.has_value()) {
    std::cerr << "Failed to create Vulkan instance: "
              << ins_ret.error().message() << " (" << ins_ret.error().message()
              << ")" << std::endl;
    std::abort();
  }

  vkb::Instance vkb_inst = ins_ret.value();

  instance_ = vkb_inst.instance;
  debugMessenger_ = vkb_inst.debug_messenger;

  // Create VkSurfaceKHR
  window_.createWindowSurface(instance_, &surface_);

  // Choose Device
  vkb::PhysicalDeviceSelector selector{vkb_inst};

  // vulkan 1.2 features
  VkPhysicalDeviceVulkan12Features features12{};
  features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  features12.bufferDeviceAddress = true;
  features12.descriptorIndexing = true;

  // vulkan 1.3 features
  VkPhysicalDeviceVulkan13Features features{};
  features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
  features.dynamicRendering = true;
  features.synchronization2 = true;

  auto select_ret = selector.set_minimum_version(1, 3)
                        .set_required_features_13(features)
                        .set_required_features_12(features12)
                        .set_surface(surface_)
                        .select();

  vkb::PhysicalDevice _physicalDevice = select_ret.value();

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

void VulkanEngine::init_vma_allocator() {
  // initialize the memory allocator
  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice = physicalDevice_;
  allocatorInfo.device = device_;
  allocatorInfo.instance = instance_;
  allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
  vmaCreateAllocator(&allocatorInfo, &allocator_);
}

void VulkanEngine::init_custom_image() {

  VkExtent3D drawImageExtent = {window_.getExtent().width,
                                window_.getExtent().height, 1};

  // hardcoding the draw format to 32 bit float
  drawImage_.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
  drawImage_.imageExtent = drawImageExtent;

  VkImageUsageFlags drawImageUsages =
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  VkImageCreateInfo rimg_info = tools::image_create_info(
      drawImage_.imageFormat, drawImageUsages, drawImageExtent);

  VmaAllocationCreateInfo rimg_allocinfo = {};
  rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  rimg_allocinfo.requiredFlags =
      VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  // allocate and create the image
  vmaCreateImage(allocator_, &rimg_info, &rimg_allocinfo, &drawImage_.image,
                 &drawImage_.allocation, nullptr);

  // build a image-view for the draw image to use for rendering
  VkImageViewCreateInfo rview_info = tools::imageview_create_info(
      drawImage_.imageFormat, drawImage_.image, VK_IMAGE_ASPECT_COLOR_BIT);

  vkCreateImageView(device_, &rview_info, nullptr, &drawImage_.imageView);
}

void VulkanEngine::init_descriptors() {
  std::vector<DescriptorAllocator::PoolSizeRatio> sizes{
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}};

  // init
  descriptorAllocator_ = DescriptorAllocator{device_, 10, sizes};

  {
    drawCompDescriptorLayout_ =
        DescriptorLayoutBuilder{}
            .add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
            .build(device_, VK_SHADER_STAGE_COMPUTE_BIT); // add bindings
  }

  // allocate a descriptor set for our draw image
  drawCompDescriptor_ =
      descriptorAllocator_.allocate(drawCompDescriptorLayout_);

  VkDescriptorImageInfo imgInfo{};
  imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  imgInfo.imageView = drawImage_.imageView;

  VkWriteDescriptorSet drawImageWrite = {};
  drawImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

  drawImageWrite.dstBinding = 0;
  drawImageWrite.dstSet = drawCompDescriptor_;
  drawImageWrite.descriptorCount = 1;
  drawImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  drawImageWrite.pImageInfo = &imgInfo;

  vkUpdateDescriptorSets(device_, 1, &drawImageWrite, 0, nullptr);
}

void VulkanEngine::init_compute_pipeline() {

  VkPipelineLayoutCreateInfo computeLayout{};
  computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  computeLayout.pNext = nullptr;
  computeLayout.pSetLayouts = &drawCompDescriptorLayout_;
  computeLayout.setLayoutCount = 1;

  vkCreatePipelineLayout(device_, &computeLayout, nullptr,
                         &gradientComputePipelineLayout_);

  // load shader
  VkShaderModule computeDrawShader;
  util::load_shader(CONFIG_HOME "shaders/gradient.comp.spv", device_,
                    &computeDrawShader);

  VkPipelineShaderStageCreateInfo stageinfo{};
  stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stageinfo.pNext = nullptr;
  stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  stageinfo.module = computeDrawShader;
  stageinfo.pName = "main";

  VkComputePipelineCreateInfo computePipelineCreateInfo{};
  computePipelineCreateInfo.sType =
      VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  computePipelineCreateInfo.layout = gradientComputePipelineLayout_;
  computePipelineCreateInfo.stage = stageinfo;

  vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1,
                           &computePipelineCreateInfo, nullptr,
                           &gradientComputePipeline_);

  vkDestroyShaderModule(device_, computeDrawShader, nullptr);
}

void VulkanEngine::destroy_compute_pipeline() {
  vkDestroyPipelineLayout(device_, gradientComputePipelineLayout_, nullptr);
  vkDestroyPipeline(device_, gradientComputePipeline_, nullptr);
}

void VulkanEngine::destroy_descriptors() {
  descriptorAllocator_.destroy_pool();
  vkDestroyDescriptorSetLayout(device_, drawCompDescriptorLayout_, nullptr);
}

void VulkanEngine::destroy_custom_image() {
  vkDestroyImageView(device_, drawImage_.imageView, nullptr);
  vmaDestroyImage(allocator_, drawImage_.image, drawImage_.allocation);
}

void VulkanEngine::destroy_vma_allocator() { vmaDestroyAllocator(allocator_); }

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
