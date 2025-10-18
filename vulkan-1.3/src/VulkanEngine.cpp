#include <Tools.hpp>
#include <Util.hpp>
#include <VkBootstrap.h>
#include <VulkanEngine.hpp>
#include <chrono>
#include <exception>
#include <numeric>
#include <stdexcept>

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
  init_frames(setCount_, frame_sizes);
  init_immediate_sync();
  init_immediate_commands();
  init_vma_allocator();
  init_custom_image();

  // Load Compute Shader
  computeEffect.reset();
  computeEffect =
      std::make_shared<compute::ComputePipelinePacked>(device_, allocator_);

  // Load Graphic Shader
  graphicEffect.reset();
  graphicEffect =
      std::make_shared<graphic::GraphicPipelinePacked>(device_, allocator_);

  if (!graphicEffect || !computeEffect)
    throw std::runtime_error("Init Graphic/Compute Pipeline Packed Error!");

  if (auto mesh = MeshAsset::loadGltfMeshes(
          device_, allocator_, CONFIG_HOME "assets/gltf/basicmesh.glb");
      mesh) {
    graphicEffect->load_asset(std::move(mesh.value()));
  }

  computeEffect->init();
  graphicEffect->init();

  imm_command_submit(graphicEffect->getMeshFunctor());
  imm_command_submit(graphicEffect->getColorFunctor());

  graphicEffect->flushUpload(immFence_);

  init_imgui();
}

void VulkanEngine::destroy() {

  destroy_imgui();

  // Do it before vkdevice being removed!
  computeEffect->destroy();
  computeEffect.reset();
  graphicEffect->destroy();
  graphicEffect.reset();

  // HAS TO BE DONE BEFORE REMOVE VMA!!!
  destroy_frames();

  destroy_custom_image();
  destroy_vma_allocator();
  destroy_immediate_commands();
  destroy_immediate_sync();
  destroy_swapchain();
  destroy_vulkan();
}

void VulkanEngine::imm_command_submit(
    std::function<void(VkCommandBuffer)> &&function) {

  vkResetFences(device_, 1, &immFence_);
  vkResetCommandBuffer(immCommandBuffer_, 0);

  VkCommandBufferBeginInfo cmdBeginInfo = tools::command_buffer_begin_info(
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  vkBeginCommandBuffer(immCommandBuffer_, &cmdBeginInfo);
  function(immCommandBuffer_);
  vkEndCommandBuffer(immCommandBuffer_);

  VkCommandBufferSubmitInfo cmdinfo =
      tools::command_buffer_submit_info(immCommandBuffer_);
  VkSubmitInfo2 submit = tools::submit_info(&cmdinfo, nullptr, nullptr);

  // submit command buffer to the queue and execute it.
  //  _renderFence will now block until the graphic commands finish execution
  vkQueueSubmit2(graphicsQueue_, 1, &submit, immFence_);

  vkWaitForFences(device_, 1, &immFence_, true,
                  std::numeric_limits<uint64_t>::max());
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

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (ImGui::Begin("background")) {

      if (!computeEffect)
        throw std::runtime_error("computeEffect not init!");

      ComputePipelinePacked &selected = *computeEffect;

      ImGui::Text("Selected effect: ", selected.name.c_str());

      ImGui::InputFloat4("topLeft", (float *)&selected.getData().topLeft,
                         "%.3f", ImGuiInputTextFlags_ElideLeft);
      ImGui::InputFloat4("topRight", (float *)&selected.getData().topRight,
                         "%.3f", ImGuiInputTextFlags_ElideLeft);
      ImGui::InputFloat4("bottomLeft", (float *)&selected.getData().bottomLeft,
                         "%.3f", ImGuiInputTextFlags_ElideLeft);
      ImGui::InputFloat4("bottomRight",
                         (float *)&selected.getData().bottomRight);
      ImGui::SliderFloat("Render Scale", &renderScale, 0.3f, 1.f, "%.3f",
                         ImGuiInputTextFlags_ElideLeft);
    }
    ImGui::End();

    // make imgui calculate internal draw structures
    ImGui::Render();

    draw();

    if (resize_requested) {
      resize_swapchain();
    }
  }

  vkDeviceWaitIdle(device_);
}

void VulkanEngine::resize_swapchain() {

  vkDeviceWaitIdle(device_);
  destroy_swapchain();
  create_swapchain(window_.getExtent().width, window_.getExtent().height);
  resize_requested = false;
}

void VulkanEngine::draw_background(VkCommandBuffer cmd, VkImage image) {
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

void VulkanEngine::draw_imgui(VkCommandBuffer cmd, VkExtent2D drawExtent,
                              VkImageView imageView) {
  auto colorAttachmentInfo = tools::color_attachment_info(imageView);
  auto renderInfo =
      tools::rendering_info(swapchainExtent_, &colorAttachmentInfo);

  vkCmdBeginRendering(cmd, &renderInfo);

  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

  vkCmdEndRendering(cmd);
}

void VulkanEngine::draw() {
  auto &currentFrame = get_current_frame();

  // wait until the gpu has finished rendering the last frame.
  vkWaitForFences(device_, 1, &currentFrame._renderFinishedFence, true,
                  std::numeric_limits<uint64_t>::max());
  vkResetFences(device_, 1, &currentFrame._renderFinishedFence);

  currentFrame.clean_last_frame();      // execute flush
  currentFrame.reset_allocator_pools(); // reset pools

  // request image from the swapchain
  uint32_t swapchainImageIndex{};
  VkResult e = vkAcquireNextImageKHR(
      device_, swapchain_, std::numeric_limits<uint64_t>::max(),
      currentFrame._swapChainWait, nullptr, &swapchainImageIndex);
  if (e == VK_ERROR_OUT_OF_DATE_KHR) {
    resize_requested = true;
    return;
  }

  // now that we are sure that the commands finished executing, we can safely
  VkCommandBuffer cmd = currentFrame._mainCommandBuffer;

  // reset the command buffer to begin recording again.
  vkResetCommandBuffer(cmd, 0);

  // use command buffer exactly once
  VkCommandBufferBeginInfo cmdBeginInfo = tools::command_buffer_begin_info(
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  drawExtent_.height = static_cast<uint32_t>(
      std::min(swapchainExtent_.height, drawImage_->imageExtent.height) *
      renderScale);
  drawExtent_.width = static_cast<uint32_t>(
      std::min(swapchainExtent_.width, drawImage_->imageExtent.width) *
      renderScale);

  vkBeginCommandBuffer(cmd, &cmdBeginInfo);

  VkImage &draw_image = drawImage_->image;   // Draw Image
  VkImage &depth_image = depthImage_->image; // Depth Image
  VkImage &swapchain_image =
      swapchainImages_[swapchainImageIndex]; // SwapChain Image

  VkImageView &image_view = swapchainImageViews_[swapchainImageIndex];

  // transition our main draw image into general layout so we can write into it
  // we will overwrite it all so we dont care about what was the older layout
  util::transition_image(cmd, draw_image, VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_GENERAL);

  // Draw Background
  draw_background(cmd, draw_image);

  // compute
  computeEffect->draw(drawExtent_, *drawImage_, *depthImage_, currentFrame);

  util::transition_image(cmd, draw_image, VK_IMAGE_LAYOUT_GENERAL,
                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  util::transition_image(cmd, depth_image, VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

  graphicEffect->draw(drawExtent_, *drawImage_, *depthImage_, currentFrame);

  // transition the draw image and the swapchain image into their correct
  // transfer layouts
  util::transition_image(cmd, draw_image,
                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  util::transition_image(cmd, swapchain_image, VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  // execute a copy from the draw image into the swapchain
  util::copy_image_to_image(cmd, draw_image, swapchain_image, drawExtent_,
                            swapchainExtent_);

  // set swapchain image layout to Attachment Optimal so we can draw it
  util::transition_image(cmd, swapchain_image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  draw_imgui(cmd, drawExtent_, image_view);

  util::transition_image(cmd, swapchain_image,
                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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

  e = vkQueuePresentKHR(graphicsQueue_, &presentInfo);
  if (e == VK_ERROR_OUT_OF_DATE_KHR) {
    resize_requested = true;
    return;
  }

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

void VulkanEngine::init_immediate_commands() {
  VkCommandPoolCreateInfo commandPoolInfo = tools::command_pool_create_info(
      graphicsQueueFamily_, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  vkCreateCommandPool(device_, &commandPoolInfo, nullptr, &immCommandPool_);

  VkCommandBufferAllocateInfo cmdAllocInfo =
      tools::command_buffer_allocate_info(immCommandPool_, 1);

  vkAllocateCommandBuffers(device_, &cmdAllocInfo, &immCommandBuffer_);
}

void VulkanEngine::init_immediate_sync() {
  // DeadLock Prevention!!!
  VkFenceCreateInfo fenceCreateInfo =
      tools::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);

  vkCreateFence(device_, &fenceCreateInfo, nullptr, &immFence_);
}

void VulkanEngine::init_vma_allocator() {
  // initialize the memory allocator
  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice = physicalDevice_;
  allocatorInfo.device = device_;
  allocatorInfo.instance = instance_;
  allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

#if ENABLE_VALIDATION_LAYERS
  allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;
#endif

  vmaCreateAllocator(&allocatorInfo, &allocator_);
}

void VulkanEngine::init_custom_image() {

  VkExtent3D extent = {window_.getExtent().width, window_.getExtent().height,
                       1};

  drawImage_.reset();
  drawImage_ = std::make_unique<AllocatedImage>(device_, allocator_);

  depthImage_.reset();
  depthImage_ = std::make_unique<AllocatedImage>(device_, allocator_);

  drawImage_->create_image(
      extent, VK_FORMAT_R16G16B16A16_SFLOAT,
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
          VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

  depthImage_->create_image(extent, VK_FORMAT_D32_SFLOAT,
                            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void VulkanEngine::init_imgui() {
  // this initializes the core structures of imgui
  ImGui::CreateContext();

  // this initializes imgui for SDL
  ImGui_ImplGlfw_InitForVulkan(window_.getGLFWWindow(), true);

  VkDescriptorPoolSize pool_sizes[] = {
      {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 1000;
  pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
  pool_info.pPoolSizes = pool_sizes;

  vkCreateDescriptorPool(device_, &pool_info, nullptr, &imguiPool_);

  ImGui_ImplVulkan_InitInfo init_info{};
  init_info.Instance = instance_;
  init_info.PhysicalDevice = physicalDevice_;
  init_info.Device = device_;
  init_info.Queue = graphicsQueue_;
  init_info.DescriptorPool = imguiPool_;
  init_info.MinImageCount = 3;
  init_info.ImageCount = 3;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.UseDynamicRendering = true;

  init_info.PipelineRenderingCreateInfo = {};
  init_info.PipelineRenderingCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
  init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats =
      &swapchainImageFormat_;

  ImGui_ImplVulkan_Init(&init_info);
}

void VulkanEngine::destroy_imgui() {
  ImGui_ImplVulkan_Shutdown();
  vkDestroyDescriptorPool(device_, imguiPool_, nullptr);
}

void VulkanEngine::destroy_custom_image() {
  if (!drawImage_ || !depthImage_)
    throw std::runtime_error("Draw/Depth Images are null!");

  drawImage_->destroy();
  depthImage_->destroy();

  drawImage_.reset();
  depthImage_.reset();
}

void VulkanEngine::destroy_vma_allocator() {

#if ENABLE_VALIDATION_LAYERS
  char *stats = nullptr;
  vmaBuildStatsString(allocator_, &stats, true);
  printf("%s\n", stats);
  vmaFreeStatsString(allocator_, stats);
#endif

  vmaDestroyAllocator(allocator_);
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

void VulkanEngine::init_frames(
    const uint32_t setCount, const std::vector<PoolSizeRatio> &poolSizeRatio) {
  // DeadLock Prevention!!!
  VkFenceCreateInfo fenceCreateInfo =
      tools::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
  VkSemaphoreCreateInfo semaphoreCreateInfo = tools::semaphore_create_info();

  VkCommandPoolCreateInfo commandPoolInfo = tools::command_pool_create_info(
      graphicsQueueFamily_, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  std::generate(frames_.begin(), frames_.end(),
                [this, commandPoolInfo, semaphoreCreateInfo, fenceCreateInfo,
                 setCount, poolSizeRatio]() {
                  auto ret = std::make_unique<FrameData>(device_);
                  ret->init_sync(fenceCreateInfo, semaphoreCreateInfo);
                  ret->init_command(commandPoolInfo);
                  ret->init_allocator(setCount, poolSizeRatio);
                  return ret;
                });
}

void VulkanEngine::destroy_frames() {
  vkDeviceWaitIdle(device_);
  for (auto &frame : frames_) {
    if (frame) {
      frame->destroy_command(false);
      frame->destroy_sync();
      frame->destroy_allocator();
    }
  }
  frames_.clear();
}

void VulkanEngine::destroy_vulkan() {

  if (isInit) {
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
    vkDestroyDevice(device_, nullptr);

    vkb::destroy_debug_utils_messenger(instance_, debugMessenger_);
    vkDestroyInstance(instance_, nullptr);
    isInit = false;
  }
}

void VulkanEngine::destroy_swapchain() {
  vkDestroySwapchainKHR(device_, swapchain_, nullptr);

  // destroy swapchain resources
  for (int i = 0; i < swapchainImageViews_.size(); i++) {

    vkDestroyImageView(device_, swapchainImageViews_[i], nullptr);
  }
}

void VulkanEngine::destroy_immediate_sync() {
  vkDestroyFence(device_, immFence_, nullptr);
}

void VulkanEngine::destroy_immediate_commands() {
  vkDestroyCommandPool(device_, immCommandPool_, nullptr);
}

FrameData &VulkanEngine::get_current_frame() {
  return *frames_[frameNumber_ % FRAMES_IN_FLIGHT];
}
void VulkanEngine::switch_to_next_frame() { ++frameNumber_; }
} // namespace engine
