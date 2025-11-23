#include <Tools.hpp>
#include <Util.hpp>
#include <VulkanEngine.hpp>
#include <builder/BarrierBuilder.hpp>
#include <builder/NodeManagerBuiler.hpp>
#include <builder/SceneNodeBuilder.hpp>
#include <chrono>
#include <exception>
#include <interactive/Keyboard_Controller.hpp>
#include <map>
#include <numeric>
#include <spdlog/spdlog.h>
#include <stdexcept>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

namespace engine {

VulkanEngine::VulkanEngine(Window &win, bool enableValidationLayer)
    : isInit(false), window_(win), frameNumber_(0),
      enableValidationLayers_(enableValidationLayer) {

  init();
}

VulkanEngine::~VulkanEngine() { destroy(); }

void VulkanEngine::init() {
  init_vulkan();
  init_swapchain();
  init_immediate_sync();
  init_immediate_commands();
  init_vma_allocator();
  init_frames(setCount_, frame_sizes);
  init_imgui();

  init_default_color();
  imm_command_submit(
      [this](VkCommandBuffer cmd) { submit_default_color(cmd); });
  flush_default_color(immFence_);

  init_default_sampler();
  init_scene_layout();
  init_scene();
  init_camera();

  if (auto nodemgr = NodeManagerBuilder{this}
                         .set_debug_color()
                         .set_filepath(CONFIG_HOME "assets/gltf/basicmesh.glb")
                         .set_options()
                         .set_root("/root")
                         .build();
      nodemgr) {

    (*nodemgr)->name = "default";
    sceneMgr->addScene((*nodemgr));
    sceneMgr->submit();
  }

  // node::SceneNodeConf conf;
  // conf.globalSceneLayout = sceneDescriptorSetLayout_;
  //  if (auto mesh = SceneNodeBuilder{this}
  //                      .set_config(conf)
  //                      .set_filepath(CONFIG_HOME "assets/gltf/basicmesh.glb")
  //                      .set_options()
  //                      .build();
  //      mesh) {
  //
  //    (*mesh)->name = "default";
  //    sceneMgr->addScene((*mesh));
  //    sceneMgr->submit();
  //  }
}

void VulkanEngine::destroy() {

  destroy_camera();
  destroy_scene();
  destroy_scene_layout();
  destroy_default_color();
  destroy_default_sampler();

  destroy_imgui();

  vkDeviceWaitIdle(device_);
  // HAS TO BE DONE BEFORE REMOVE VMA!!!
  destroy_frames();

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

  auto &data = sceneMgr->getComputeData();

  KeyBoardController keyboard_controller;
  auto frameTimeStart = std::chrono::high_resolution_clock::now();
  auto frameTimeDuration =
      std::chrono::duration<float, std::chrono::microseconds::period>(
          std::chrono::high_resolution_clock::now() - frameTimeStart)
          .count();

  camera_->setViewTarget(glm::vec3{0.f, 0.f, -1.f}, glm::vec3(0.f, 0.f, 0.f));
  camera_->setPerspectiveProjection(glm::radians(45.f),
                                    static_cast<float>(swapchainExtent_.width) /
                                        swapchainExtent_.height,
                                    0.1f, 100.f);

  // camera_
  while (!window_.shouldClose()) {

    glfwPollEvents();

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Engine Control Panel");
    show_states(stats);
    sceneMgr->on_gui();
    ImGui::End();

    stats.drawcall_count = stats.triangle_count = 0;

    // make imgui calculate internal draw structures
    ImGui::Render();

    keyboard_controller.movePlaneYXZ(window_.getGLFWWindow(), stats.frametime,
                                     camera_);
    camera_->setYXZ(camera_->localTransform.translation,
                    camera_->localTransform.rotation);

    // draw meshes!
    draw();

    auto drawEnd = std::chrono::high_resolution_clock::now();
    auto drawTimeFrame =
        std::chrono::duration<float, std::chrono::microseconds::period>(
            drawEnd - frameTimeStart)
            .count();

    auto frameTimeEnd = std::chrono::high_resolution_clock::now();
    frameTimeDuration =
        std::chrono::duration<float, std::chrono::microseconds::period>(
            frameTimeEnd - frameTimeStart)
            .count();

    stats.mesh_draw_time = drawTimeFrame / 1000.f;
    stats.frametime = frameTimeDuration / 1000.f;

    sceneMgr->getParticleData().deltaTime = stats.frametime;

    frameTimeStart = frameTimeEnd;

    if (resize_requested) {

      vkDeviceWaitIdle(device_);

      int width, height;
      glfwGetFramebufferSize(window_.getGLFWWindow(), &width, &height);
      while (!width || !height) {
        glfwGetFramebufferSize(window_.getGLFWWindow(), &width, &height);
        glfwWaitEvents();
      }

      resize_swapchain();
      resize_frames();
    }

    switch_to_next_frame();
  }

  vkDeviceWaitIdle(device_);
}

VkDescriptorSetLayout VulkanEngine::create_ubo_layout() {
  return DescriptorLayoutBuilder{device_}
      .add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
      .build(VK_SHADER_STAGE_VERTEX_BIT |
             VK_SHADER_STAGE_FRAGMENT_BIT); // add bindings
}

void VulkanEngine::resize_swapchain() {
  destroy_swapchain();
  create_swapchain(window_.getExtent().width, window_.getExtent().height);
  resize_requested = false;
}

void VulkanEngine::resize_frames() {

  VkExtent3D extent = {window_.getExtent().width, window_.getExtent().height,
                       1};

  for (auto &frame : frames_) {
    if (frame) {
      frame->reset_images(extent);
    }
  }
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

void VulkanEngine::show_states(const EngineStats &stats) {
  static float fps[120] = {};
  static int index = 0;

  static float sampleAccum = 0.0f;
  static int sampleCount = 0;

  // ---- Clamp frametime for safety ----
  if (!std::isfinite(stats.frametime) ||
      stats.frametime <= std::numeric_limits<float>::epsilon()) {
    spdlog::warn("[VulkanEngine Statistic Info]: INVALID frametime = {}",
                 stats.frametime);
    return;
  }

  float currentFPS = 1000.f / stats.frametime;

  sampleAccum += currentFPS;
  sampleCount++;

  bool ready = false;
  float averagedFPS = 0.0f;

  if (sampleCount >= 4) {
    averagedFPS = sampleAccum / sampleCount;
    sampleAccum = 0.0f;
    sampleCount = 0;
    ready = true;
  }

  // EMA
  static float filteredFPS = 72.f; // START WITH REASONABLE DEFAULT!!
  const float alpha = 0.25f;       // EMA weight

  if (ready) {
    filteredFPS = filteredFPS * (1.0f - alpha) + averagedFPS * alpha;
    fps[index] = filteredFPS;
    index = (index + 1) % IM_ARRAYSIZE(fps);
  }

  if (ImGui::CollapsingHeader("Engine Stats", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImU32 lineColor;
    if (filteredFPS >= 60)
      lineColor = IM_COL32(0, 255, 0, 255);
    else if (filteredFPS >= 30 && filteredFPS < 60)
      lineColor = IM_COL32(255, 255, 0, 255);
    else
      lineColor = IM_COL32(255, 0, 0, 255);

    ImGui::Text("Frame time:  %.3f ms", stats.frametime);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0.30f)); // dark
    ImGui::PushStyleColor(ImGuiCol_PlotLines,
                          ImGui::ColorConvertU32ToFloat4(lineColor));
    ImGui::PushStyleColor(ImGuiCol_PlotLinesHovered,
                          ImGui::ColorConvertU32ToFloat4(lineColor));
    ImGui::PlotLines(
        "FPS", fps, IM_ARRAYSIZE(fps),
        index,                                          // offset
        fmt::format("{:.1f} FPS", filteredFPS).c_str(), // overlay text
        0.0f,                                           // ymin
        30.0f,                                          // ymax
        ImVec2(250, 30)                                 // width  height
    );
    ImGui::Text("Draw time:   %.3f ms", stats.mesh_draw_time);
    ImGui::Text("Update time: %.3f ms", stats.scene_update_time);
    ImGui::Text("Triangles:   %i", stats.triangle_count);
    ImGui::Text("Draw calls:  %i", stats.drawcall_count);

    ImGui::PopStyleColor(3);
    ImGui::Separator();
  }
}

bool VulkanEngine::isDeviceSuitable(const vkb::PhysicalDevice &device) {
  auto properties = device.properties;
  auto features = device.features;

  if (properties.deviceType ==
          VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
      features.geometryShader) {
    return true;
  }
  return false;
}

void VulkanEngine::draw() {

  // wait until the gpu has finished rendering the last frame.
  frame_cache = nullptr;
  frame_cache = &get_current_frame();

  if (!frame_cache)[[unlikely]]
            throw std::runtime_error("[VulkanEngine]: Invalid frame cache!");

  if (!frame_cache->fence_ready()) {
            // wait until the gpu has finished rendering the last frame.
            vkWaitForFences(device_, 1, &frame_cache->finalSyncFence_, true,
                      std::numeric_limits<uint64_t>::max());
  }
  vkResetFences(device_, 1, &frame_cache->finalSyncFence_);

  sceneMgr->update_scene();

  frame_cache->computeWaitValue_ = frame_cache->timelineValue_;
  frame_cache->computeSignalValue_ = ++frame_cache->timelineValue_;

  compute();

  frame_cache->graphicsWaitValue_ = frame_cache->computeSignalValue_;
  frame_cache->graphicsSignalValue_ = ++frame_cache->timelineValue_;

  // request image from the swapchain
  uint32_t swapchainImageIndex{};

  graphic(swapchainImageIndex);
  presentKHR(swapchainImageIndex);
}

void VulkanEngine::compute() {

  auto *context = frame_cache->get_context(FrameData::ContextPass::COMPUTE);
  if (!context) {
    spdlog::error("[VulkanEngine Error]: Invalid Compute Pass!");
    return;
  }

  context->clean_last_frame();
  context->reset_allocator_pools();

  // now that we are sure that the commands finished executing, we can safely
  VkCommandBuffer cmd = context->_commandBuffer;

  // reset the command buffer to begin recording again.
  vkResetCommandBuffer(cmd, 0);

  // use command buffer exactly once
  VkCommandBufferBeginInfo cmdBeginInfo = tools::command_buffer_begin_info(
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  vkBeginCommandBuffer(cmd, &cmdBeginInfo);

  sceneMgr->compute(cmd, frame_cache->ctx[FrameData::ContextPass::COMPUTE]);

  vkEndCommandBuffer(cmd);

  VkCommandBufferSubmitInfo cmdinfo = tools::command_buffer_submit_info(cmd);

  VkSemaphoreSubmitInfo computeWait = tools::semaphore_submit_info(
      VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, frame_cache->timelineSemaphore_,
      computeQueueFamily_, frame_cache->computeWaitValue_);

  VkSemaphoreSubmitInfo computeSignal = tools::semaphore_submit_info(
      VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, frame_cache->timelineSemaphore_,
      computeQueueFamily_, frame_cache->computeSignalValue_);

  VkSubmitInfo2 info{};
  info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  info.waitSemaphoreInfoCount = 1;
  info.pWaitSemaphoreInfos = &computeWait;
  info.signalSemaphoreInfoCount = 1;
  info.pSignalSemaphoreInfos = &computeSignal;
  info.commandBufferInfoCount = 1;
  info.pCommandBufferInfos = &cmdinfo;

  vkQueueSubmit2(computeQueue_, 1, &info, nullptr);
}

void VulkanEngine::graphic(uint32_t &swapchainImageIndex) {

  auto *context = frame_cache->get_context(FrameData::ContextPass::GRAPHIC);
  if (!context) {
    spdlog::error("[VulkanEngine Error]: Invalid Graphic Pass!");
    return;
  }

  context->clean_last_frame();
  context->reset_allocator_pools();

  // now that we are sure that the commands finished executing, we can safely
  VkCommandBuffer cmd = context->_commandBuffer;

  VkImage &draw_image = frame_cache->drawImage_->image;   // Draw Image
  VkImage &depth_image = frame_cache->depthImage_->image; // Depth Image

  drawExtent_.height = static_cast<uint32_t>(
      std::min(swapchainExtent_.height,
                frame_cache->drawImage_->imageExtent.height) *
      renderScale);
  drawExtent_.width = static_cast<uint32_t>(
      std::min(swapchainExtent_.width,
                frame_cache->drawImage_->imageExtent.width) *
      renderScale);

  // reset the command buffer to begin recording again.
  vkResetCommandBuffer(cmd, 0);

  // use command buffer exactly once
  VkCommandBufferBeginInfo cmdBeginInfo = tools::command_buffer_begin_info(
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  vkBeginCommandBuffer(cmd, &cmdBeginInfo);

  // transition our main draw image into general layout so we can write into it
  // we will overwrite it all so we dont care about what was the older layout
  // util::transition_image(cmd, draw_image, VK_IMAGE_LAYOUT_UNDEFINED,
  //                                 VK_IMAGE_LAYOUT_GENERAL);
  auto undefined2General =
      ImageBarrierBuilder(draw_image, VK_FORMAT_B8G8R8A8_UNORM)
          .from(VK_IMAGE_LAYOUT_UNDEFINED)
          .to(VK_IMAGE_LAYOUT_GENERAL)
          .build();

  BarrierBuilder{}.add(undefined2General).createBarrier(cmd);

  // Draw Background
  draw_background(cmd, draw_image);

  auto general2ColorAttach =
      ImageBarrierBuilder(draw_image, VK_FORMAT_B8G8R8A8_UNORM)
          .from(VK_IMAGE_LAYOUT_GENERAL)
          .to(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
          .build();

  auto undefined2DepthAttach =
      ImageBarrierBuilder(depth_image, VK_FORMAT_D32_SFLOAT)
          .from(VK_IMAGE_LAYOUT_UNDEFINED)
          .to(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
          .aspect(VK_IMAGE_ASPECT_DEPTH_BIT)
          .build();

  BarrierBuilder{}
      .add(general2ColorAttach)
      .add(undefined2DepthAttach)
      .createBarrier(cmd);

  // Graphic Render
  sceneMgr->render(cmd, frame_cache->ctx[FrameData::ContextPass::GRAPHIC]);

  VkResult e = vkAcquireNextImageKHR(
      device_, swapchain_, std::numeric_limits<uint64_t>::max(),
            frame_cache->swapChainWait_, nullptr, &swapchainImageIndex);
  if (e == VK_ERROR_OUT_OF_DATE_KHR) {
    resize_requested = true;
  } else if (e != VK_SUCCESS && e != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  VkImage &swapchain_image =
      swapchainImages_[swapchainImageIndex]; // SwapChain Image

  VkImageView &image_view = swapchainImageViews_[swapchainImageIndex];

  // transition the draw image and the swapchain image into their correct
  // transfer layouts
  auto colorAttach2transfer =
      ImageBarrierBuilder(draw_image, VK_FORMAT_B8G8R8A8_UNORM)
          .from(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
          .to(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
          .build();

  auto undefined2Transfer =
      ImageBarrierBuilder(swapchain_image, VK_FORMAT_B8G8R8A8_UNORM)
          .from(VK_IMAGE_LAYOUT_UNDEFINED)
          .to(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
          .build();

  BarrierBuilder{}
      .add(colorAttach2transfer)
      .add(undefined2Transfer)
      .createBarrier(cmd);

  // execute a copy from the draw image into the swapchain
  util::copy_image_to_image(cmd, draw_image, swapchain_image, drawExtent_,
                            swapchainExtent_);

  // set swapchain image layout to Attachment Optimal so we can draw it
  auto transferDst2ColorAttach =
      ImageBarrierBuilder(swapchain_image, VK_FORMAT_B8G8R8A8_UNORM)
          .from(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
          .to(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
          .build();

  BarrierBuilder{}.add(transferDst2ColorAttach).createBarrier(cmd);

  draw_imgui(cmd, drawExtent_, image_view);

  auto colorAttach2Present =
      ImageBarrierBuilder(swapchain_image, VK_FORMAT_B8G8R8A8_UNORM)
          .from(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
          .to(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
          .queueIndex(graphicsQueueFamily_, presentQueueFamily_)
          .build();

  BarrierBuilder{}.add(colorAttach2Present).createBarrier(cmd);

  vkEndCommandBuffer(cmd);

  VkCommandBufferSubmitInfo cmdinfo = tools::command_buffer_submit_info(cmd);

  VkSemaphoreSubmitInfo swapChainImageWait = tools::semaphore_submit_info(
      VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
      get_current_frame().swapChainWait_, graphicsQueueFamily_);

  VkSemaphoreSubmitInfo graphicWait = tools::semaphore_submit_info(
      VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
      get_current_frame().timelineSemaphore_, graphicsQueueFamily_,
            frame_cache->graphicsWaitValue_);

  VkSemaphoreSubmitInfo graphic2Present = tools::semaphore_submit_info(
      VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
      get_current_frame().graphicToPresent_, graphicsQueueFamily_);

  VkSemaphoreSubmitInfo graphicSignal = tools::semaphore_submit_info(
      VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
      get_current_frame().timelineSemaphore_, graphicsQueueFamily_,
            frame_cache->graphicsSignalValue_);

  std::array<VkSemaphoreSubmitInfo, 2> waits = {swapChainImageWait,
                                                graphicWait};

  std::array<VkSemaphoreSubmitInfo, 2> signals = {graphic2Present,
                                                  graphicSignal};

  VkSubmitInfo2 info{};
  info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  info.waitSemaphoreInfoCount = static_cast<uint32_t>(waits.size());
  info.pWaitSemaphoreInfos = waits.data();
  info.signalSemaphoreInfoCount = static_cast<uint32_t>(signals.size());
  info.pSignalSemaphoreInfos = signals.data();
  info.commandBufferInfoCount = 1;
  info.pCommandBufferInfos = &cmdinfo;

  vkQueueSubmit2(graphicsQueue_, 1, &info, frame_cache->finalSyncFence_);
}

void VulkanEngine::presentKHR(uint32_t swapchainImageIndex) {

  // we want to wait on the _renderSemaphore for that,
  // as its necessary that drawing commands have finished before the image is
  // displayed to the user
  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapchain_;
  presentInfo.pImageIndices = &swapchainImageIndex;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &get_current_frame().graphicToPresent_;

  VkResult e = vkQueuePresentKHR(presentQueue_, &presentInfo);
  if (e == VK_ERROR_OUT_OF_DATE_KHR || e == VK_SUBOPTIMAL_KHR) {
    resize_requested = true;
    return;
  } else if (e != VK_SUCCESS && e != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }
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

  // vulkan 1.1 features
  VkPhysicalDeviceVulkan11Features vk11Features{};
  vk11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
  vk11Features.shaderDrawParameters = VK_TRUE; // DrawParameters

  // vulkan 1.2 features
  VkPhysicalDeviceVulkan12Features features12{};
  features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  features12.bufferDeviceAddress = true;
  features12.descriptorIndexing = true;
  features12.timelineSemaphore = true; // tineline sem support

  // vulkan 1.3 features
  VkPhysicalDeviceVulkan13Features features13{};
  features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
  features13.dynamicRendering = true;
  features13.synchronization2 = true;

  auto select_ret = selector.set_minimum_version(1, 3)
                        .set_required_features_11(vk11Features)
                        .set_required_features_12(features12)
                        .set_required_features_13(features13)
                        .set_surface(surface_)
                        //.select_devices()
                        .select();

  vkb_physicalDevice_ = pickPhysicalDevicesByUser(selector);

  vkb::DeviceBuilder deviceBuilder{vkb_physicalDevice_};
  vkb::Device vkbDevice = deviceBuilder.build().value();

  physicalDevice_ = vkb_physicalDevice_.physical_device;
  device_ = vkbDevice.device;

  graphicsQueue_ = vkbDevice.get_queue(vkb::QueueType::graphics).value();
  graphicsQueueFamily_ =
      vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

  spdlog::info(
      "[VulkanEngine Info]: Creating graphic queue with queueFamilyIndex = {}",
      graphicsQueueFamily_);

  presentQueue_ = vkbDevice.get_queue(vkb::QueueType::present).value();
  presentQueueFamily_ =
      vkbDevice.get_queue_index(vkb::QueueType::present).value();

  spdlog::info(
      "[VulkanEngine Info]: Creating present queue with queueFamilyIndex = {}",
      presentQueueFamily_);

  if (!vkb_physicalDevice_.has_separate_transfer_queue()) {
    isTransferQueueSupported = false;
    transferQueue_ = graphicsQueue_;
    transferQueueFamily_ = graphicsQueueFamily_;
    spdlog::warn("[VulkanEngine Warn]: Device has no dedicated transfer queue, "
                 "using graphics queue instead ");
    spdlog::warn("[VulkanEngine Warn]: Override transfer queue with "
                 "queueFamilyIndex = {}",
                 transferQueueFamily_);
  } else {
    isTransferQueueSupported = true;
    transferQueue_ = vkbDevice.get_queue(vkb::QueueType::transfer).value();
    transferQueueFamily_ =
        vkbDevice.get_queue_index(vkb::QueueType::transfer).value();
    spdlog::info("[VulkanEngine Info]: Creating transfe queue with "
                 "queueFamilyIndex = {}",
                 transferQueueFamily_);
  }

  if (!vkb_physicalDevice_.has_separate_compute_queue()) {
    isComputeQueueSupported = false;
    computeQueue_ = graphicsQueue_;
    computeQueueFamily_ = graphicsQueueFamily_;
    spdlog::warn("[VulkanEngine Warn]:Device has no dedicated compute queue, "
                 "using graphics queue instead ");
    spdlog::warn("[VulkanEngine Warn]: Override compute queue with "
                 "queueFamilyIndex = {}",
                 computeQueueFamily_);
    return;
  }

  isComputeQueueSupported = true;
  computeQueue_ = vkbDevice.get_queue(vkb::QueueType::compute).value();
  computeQueueFamily_ =
      vkbDevice.get_queue_index(vkb::QueueType::compute).value();

  spdlog::info(
      "[VulkanEngine Info]: Creating compute queue with queueFamilyIndex = {}",
      computeQueueFamily_);
  isInit = true;
}

vkb::PhysicalDevice
VulkanEngine::pickDefaultPhysicalDevice(vkb::PhysicalDeviceSelector &selector) {
  return selector.select().value();
}

vkb::PhysicalDevice
VulkanEngine::pickPhysicalDevicesByUser(vkb::PhysicalDeviceSelector &selector,
                                        bool enableDefault) {

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

  selector.set_minimum_version(1, 3)
      .set_required_features_13(features)
      .set_required_features_12(features12)
      .set_surface(surface_);

  if (enableDefault) {
    return pickDefaultPhysicalDevice(selector);
  }

  // get multiple devices
  auto devices = selector.select_devices().value();
  if (devices.empty()) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }

  std::multimap</*score = */ std::size_t, vkb::PhysicalDevice> candidates;

  for (auto &device : devices) {
    std::size_t score = 0;

    score += isDeviceSuitable(device) ? 1000 : 0;
    score += device.properties.limits.maxImageDimension2D;
    candidates.insert(std::make_pair(score, device));
  }

  if (!candidates.rbegin()->first) {
    throw std::runtime_error("failed to find a suitable GPU!");
  }
  return candidates.rbegin()->second;
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

void VulkanEngine::init_scene() {
  sceneMgr.reset();
  sceneMgr = std::make_unique<ScenesManager>(this);
  sceneMgr->init();
}

void VulkanEngine::init_camera() {
  camera_.reset();
  camera_ = std::make_unique<node::CameraNode>();
}

void VulkanEngine::init_default_color() {
  white_.reset();
  grey_.reset();
  black_.reset();
  magenta_.reset();
  loaderrorImage_.reset();

  white_ = std::make_shared<AllocatedTexture>(device_, allocator_);
  grey_ = std::make_shared<AllocatedTexture>(device_, allocator_);
  black_ = std::make_shared<AllocatedTexture>(device_, allocator_);
  magenta_ = std::make_shared<AllocatedTexture>(device_, allocator_);
  loaderrorImage_ = std::make_shared<AllocatedTexture>(device_, allocator_);

  uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
  uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
  uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 1));
  uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));

  std::array<uint32_t, 16 * 16> pixels; // for 16x16 checkerboard texture

  for (int x = 0; x < 16; x++) {
    for (int y = 0; y < 16; y++) {
      pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
    }
  }

  white_->createBuffer(reinterpret_cast<void *>(&white), VkExtent3D{1, 1, 1},
                       VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

  grey_->createBuffer(reinterpret_cast<void *>(&grey), VkExtent3D{1, 1, 1},
                      VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

  black_->createBuffer(reinterpret_cast<void *>(&black), VkExtent3D{1, 1, 1},
                       VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

  magenta_->createBuffer(reinterpret_cast<void *>(&magenta),
                         VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM,
                         VK_IMAGE_USAGE_SAMPLED_BIT);

  loaderrorImage_->createBuffer(reinterpret_cast<void *>(pixels.data()),
                                VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM,
                                VK_IMAGE_USAGE_SAMPLED_BIT);
}

void VulkanEngine::init_default_sampler() {
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_NEAREST;
  samplerInfo.minFilter = VK_FILTER_NEAREST;
  vkCreateSampler(device_, &samplerInfo, nullptr, &defaultSamplerNearest_);

  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  vkCreateSampler(device_, &samplerInfo, nullptr, &defaultSamplerLinear_);
}

void VulkanEngine::init_scene_layout() {
  sceneDescriptorSetLayout_ = create_ubo_layout();
}

void VulkanEngine::destroy_scene_layout() {
  vkDestroyDescriptorSetLayout(device_, sceneDescriptorSetLayout_, nullptr);
}

void VulkanEngine::destroy_default_sampler() {
  vkDestroySampler(device_, defaultSamplerNearest_, nullptr);
  vkDestroySampler(device_, defaultSamplerLinear_, nullptr);
}

void VulkanEngine::destroy_default_color() {

  white_->destroy();
  grey_->destroy();
  black_->destroy();
  magenta_->destroy();
  loaderrorImage_->destroy();

  white_.reset();
  grey_.reset();
  black_.reset();
  magenta_.reset();
  loaderrorImage_.reset();
}

void VulkanEngine::destroy_camera() { camera_.reset(); }

void VulkanEngine::destroy_scene() {
  sceneMgr->destroy();
  sceneMgr.reset();
}

void VulkanEngine::destroy_imgui() {
  ImGui_ImplVulkan_Shutdown();
  vkDestroyDescriptorPool(device_, imguiPool_, nullptr);
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
          //.set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR)
          .set_desired_extent(width, height)
          .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
          .build()
          .value();

  swapchainExtent_ = vkbSwapchain.extent;
  swapchain_ = vkbSwapchain.swapchain;
  swapchainImages_ = vkbSwapchain.get_images().value();
  swapchainImageViews_ = vkbSwapchain.get_image_views().value();

  FRAMES_IN_FLIGHT = swapchainImages_.size();

  spdlog::info("[VulkanEngine Info]: Setting  FRAMES_IN_FLIGHT = {}",
               FRAMES_IN_FLIGHT);

  assert(FRAMES_IN_FLIGHT != 0);
}

void VulkanEngine::init_frames(
    const uint32_t setCount, const std::vector<PoolSizeRatio> &poolSizeRatio) {

  // DeadLock Prevention!!!
  VkFenceCreateInfo fenceCreateInfo =
      tools::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
  VkSemaphoreCreateInfo semaphoreCreateInfo = tools::semaphore_create_info();

  VkSemaphoreTypeCreateInfo type{};
  type.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
  type.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
  type.initialValue = 0;

  VkSemaphoreCreateInfo timelineSemaphoreCreateInfo = {};
  timelineSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  timelineSemaphoreCreateInfo.pNext = reinterpret_cast<void *>(&type);

  VkCommandPoolCreateInfo graphicCommandPoolInfo =
      tools::command_pool_create_info(
          graphicsQueueFamily_,
          VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  VkCommandPoolCreateInfo computeCommandPoolInfo =
      tools::command_pool_create_info(
          computeQueueFamily_, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  VkExtent3D extent = {window_.getExtent().width, window_.getExtent().height,
                       1};

  assert(FRAMES_IN_FLIGHT != 0);
  frames_.resize(FRAMES_IN_FLIGHT);

  std::generate(
      frames_.begin(), frames_.end(),
      [this, graphicCommandPoolInfo, computeCommandPoolInfo,
       semaphoreCreateInfo, timelineSemaphoreCreateInfo, fenceCreateInfo,
       setCount, poolSizeRatio, extent]() {
        auto ret = std::make_unique<FrameData>(this);
        ret->init(extent, fenceCreateInfo, semaphoreCreateInfo,
                  timelineSemaphoreCreateInfo);
        ret->ctx[FrameData::ContextPass::COMPUTE] =
            std::make_unique<ComputeFrameContext>(ret.get());
        ret->ctx[FrameData::ContextPass::GRAPHIC] =
            std::make_unique<GraphicFrameContext>(ret.get());
        ret->ctx[FrameData::ContextPass::COMPUTE]->init(
            fenceCreateInfo, computeCommandPoolInfo, setCount, poolSizeRatio);
        ret->ctx[FrameData::ContextPass::GRAPHIC]->init(
            fenceCreateInfo, graphicCommandPoolInfo, setCount, poolSizeRatio);
        return ret;
      });
}

void VulkanEngine::destroy_frames() {

  for (auto &frame : frames_) {
    if (frame) {
      auto safe_destroy = [&](FrameData::ContextPass pass) {
        if (frame->ctx.count(pass) && frame->ctx[pass]) {
          frame->ctx[pass]->destroy(false);
          frame->ctx[pass].reset();
        }
      };
      safe_destroy(FrameData::ContextPass::COMPUTE);
      safe_destroy(FrameData::ContextPass::GRAPHIC);
      frame->destroy();
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

void VulkanEngine::submit_default_color(VkCommandBuffer cmd) {
  black_->uploadBufferToImage(cmd);
  white_->uploadBufferToImage(cmd);
  grey_->uploadBufferToImage(cmd);
  magenta_->uploadBufferToImage(cmd);
  loaderrorImage_->uploadBufferToImage(cmd);
}

void VulkanEngine::flush_default_color(VkFence fence) {
  black_->flushUpload(fence);
  white_->flushUpload(fence);
  grey_->flushUpload(fence);
  magenta_->flushUpload(fence);
  loaderrorImage_->flushUpload(fence);
}

FrameData &VulkanEngine::get_current_frame() {
  return *frames_[frameNumber_ % FRAMES_IN_FLIGHT];
}
void VulkanEngine::switch_to_next_frame() { ++frameNumber_; }
} // namespace engine
