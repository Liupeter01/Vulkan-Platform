#include <Tools.hpp>
#include <algorithm>
#include <iostream>
#include <pipeline/GraphicPipeline.hpp>

namespace engine {
GraphicPipelineBuilder::GraphicPipelineBuilder(VkDevice device)
    : device_(device) {
  clear();

  init_dynamic_state();
  init_viewport_state();
}

void GraphicPipelineBuilder::clear() {
  rasterizer_ = {};

  inputAssembly_ = {};
  multisampling_ = {};
  pipelineLayout_ = {};
  depthStencil_ = {};
  renderInfo_ = {};
  dynamicInfo_ = {};
  viewportState_ = {};
  colorBlendState_ = {};
  colorBlendAttachment_ = {};
  vertexInputInfo_ = {};

  vertexInputInfo_.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  colorBlendState_.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  inputAssembly_.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  rasterizer_.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  multisampling_.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  depthStencil_.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  renderInfo_.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  dynamicInfo_.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  viewportState_.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

  colorBlendAttachment_.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  shaderStages_.clear();
}

void GraphicPipelineBuilder::init_dynamic_state() {
  dynamicInfo_.pDynamicStates = dynamicStates_.data();
  dynamicInfo_.dynamicStateCount = static_cast<uint32_t>(dynamicStates_.size());
}

void GraphicPipelineBuilder::init_viewport_state() {
  viewportState_.viewportCount = 1;
  viewportState_.scissorCount = 1;
}

void GraphicPipelineBuilder::init_colorBlend_state() {
  colorBlendState_.logicOpEnable = VK_FALSE;
  colorBlendState_.logicOp = VK_LOGIC_OP_COPY;
  colorBlendState_.attachmentCount = 1;
  colorBlendState_.pAttachments = &colorBlendAttachment_;
}

GraphicPipelineBuilder &GraphicPipelineBuilder::disable_blending() {
  colorBlendAttachment_.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment_.blendEnable = VK_FALSE;
  return *this;
}

GraphicPipelineBuilder &GraphicPipelineBuilder::set_multisampling() {
  multisampling_.sampleShadingEnable = VK_FALSE;
  // multisampling defaulted to no multisampling (1 sample per pixel)
  multisampling_.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling_.minSampleShading = 1.0f;
  multisampling_.pSampleMask = nullptr;
  // no alpha to coverage either
  multisampling_.alphaToCoverageEnable = VK_FALSE;
  multisampling_.alphaToOneEnable = VK_FALSE;
  return *this;
}

GraphicPipelineBuilder &
GraphicPipelineBuilder::set_color_attachment_format(VkFormat format) {
  colorAttachmentformat_ = format;
  // connect the format to the renderInfo  structure
  renderInfo_.colorAttachmentCount = 1;
  renderInfo_.pColorAttachmentFormats = &colorAttachmentformat_;
  return *this;
}

GraphicPipelineBuilder &
GraphicPipelineBuilder::set_depth_format(VkFormat format) {
  renderInfo_.depthAttachmentFormat = format;
  return *this;
}

GraphicPipelineBuilder &
GraphicPipelineBuilder::set_shaders(const std::string &vertexShaderPath,
                                    const std::string &fragmentShaderPath) {
  shaderStages_.clear();
  shaderStages_.resize(2);

  shaderStages_[0] = std::move(tools::shader_stage_create_info(
      device_, vertexShaderPath, VK_SHADER_STAGE_VERTEX_BIT));
  shaderStages_[1] = std::move(tools::shader_stage_create_info(
      device_, fragmentShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT));
  return *this;
}

GraphicPipelineBuilder &
GraphicPipelineBuilder::set_blending_additive(bool status) {
  colorBlendAttachment_.blendEnable = status;
  colorBlendAttachment_.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment_.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment_.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment_.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment_.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment_.alphaBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment_.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  return *this;
}

GraphicPipelineBuilder &
GraphicPipelineBuilder::set_blending_alphablend(bool status) {
  colorBlendAttachment_.blendEnable = status;
  colorBlendAttachment_.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment_.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment_.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment_.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment_.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment_.alphaBlendOp = VK_BLEND_OP_ADD;

  colorBlendAttachment_.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  return *this;
}

GraphicPipelineBuilder &
GraphicPipelineBuilder::set_input_topology(const VkPrimitiveTopology topology,
                                           const bool restart) {
  inputAssembly_.topology = topology;
  inputAssembly_.primitiveRestartEnable = restart;
  return *this;
}

GraphicPipelineBuilder &GraphicPipelineBuilder::set_depthtest(bool status,
                                                              VkCompareOp op) {
  depthStencil_.depthTestEnable = VK_TRUE;
  depthStencil_.depthWriteEnable = status;
  depthStencil_.depthCompareOp = op;
  depthStencil_.depthBoundsTestEnable = VK_FALSE;
  depthStencil_.stencilTestEnable = VK_FALSE;
  depthStencil_.front = {};
  depthStencil_.back = {};
  depthStencil_.minDepthBounds = 0.f;
  depthStencil_.maxDepthBounds = 1.f;
  return *this;
}

GraphicPipelineBuilder &
GraphicPipelineBuilder::set_polygon_mode(const VkPolygonMode mode,
                                         const float linewidth) {
  rasterizer_.polygonMode = mode;
  rasterizer_.lineWidth = 1.f;
  return *this;
}

GraphicPipelineBuilder &
GraphicPipelineBuilder::set_cull_mode(const VkCullModeFlags cullMode,
                                      const VkFrontFace frontFace) {
  rasterizer_.cullMode = cullMode;
  rasterizer_.frontFace = frontFace;
  return *this;
}

VkPipeline GraphicPipelineBuilder::build() {

  init_colorBlend_state();

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.pNext = &renderInfo_;

  pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages_.size());
  pipelineInfo.pStages = shaderStages_.data();

  pipelineInfo.pDynamicState = &dynamicInfo_;

  pipelineInfo.pVertexInputState = &vertexInputInfo_;
  pipelineInfo.pInputAssemblyState = &inputAssembly_;
  pipelineInfo.pViewportState = &viewportState_;
  pipelineInfo.pRasterizationState = &rasterizer_;
  pipelineInfo.pMultisampleState = &multisampling_;
  pipelineInfo.pDepthStencilState = &depthStencil_;
  pipelineInfo.layout = pipelineLayout_;

  init_colorBlend_state();
  pipelineInfo.pColorBlendState = &colorBlendState_;

  VkPipeline graphicPipeline = VK_NULL_HANDLE;
  if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &graphicPipeline) != VK_SUCCESS) {
    return VK_NULL_HANDLE; // failed to create graphics pipeline
  }

  return graphicPipeline;
}

namespace graphic {

GraphicPipelinePacked::GraphicPipelinePacked(VkDevice device,
                                             VmaAllocator allocator)
    : PipelineBasic(device, allocator, PipelineType::GRAPHIC) {}

GraphicPipelinePacked::~GraphicPipelinePacked() { destroy(); }

void GraphicPipelinePacked::init() {
  if (isInit_)
    return;

  init_layout();
  init_pipeline();
  load_default_colors();

  init_finished();
}

void GraphicPipelinePacked::destroy() {

  if (isInit_) {
    destroy_default_colors();
    destroy_layout();
    destroy_pipeline();
    reset_init();
  }
}

void GraphicPipelinePacked::draw(VkExtent2D drawExtent,
                                 AllocatedImage &offscreen_draw,
                                 AllocatedImage &offscreen_depth,
                                 FrameData &currentFrame) {

  // now that we are sure that the commands finished executing, we can safely
  VkCommandBuffer cmd = currentFrame._mainCommandBuffer;

  std::shared_ptr<AllocatedBuffer> sceneDataBuffer =
      std::make_shared<AllocatedBuffer>(allocator_);

  sceneDataBuffer->create(
      sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, "GraphicPipeline::Draw::SceneDataBuffer");

  currentFrame.destroy_by_deferred(
      [sceneDataBuffer]() { sceneDataBuffer->destroy(); });

  GPUSceneData *data = reinterpret_cast<GPUSceneData *>(sceneDataBuffer->map());
  *data = sceneData_;
  sceneDataBuffer->unmap();

  VkDescriptorSet sceneSet = currentFrame.allocate(sceneDescriptorSetLayout_);

  DescriptorWriter writer{device_};
  writer.write_buffer(0, sceneDataBuffer->buffer, sizeof(GPUSceneData), 0,
                      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
  writer.update_set(sceneSet);

  auto colorAttachmentInfo =
      tools::color_attachment_info(offscreen_draw.imageView);
  auto depthAttachmentInfo =
      tools::depth_attachment_info(offscreen_depth.imageView);
  auto renderInfo = tools::rendering_info(drawExtent, &colorAttachmentInfo,
                                          &depthAttachmentInfo);

  vkCmdBeginRendering(cmd, &renderInfo);

  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

  // set dynamic viewport and scissor
  VkViewport viewport = {};
  viewport.width = static_cast<float>(drawExtent.width);
  viewport.height = static_cast<float>(drawExtent.height);
  viewport.minDepth = 0.f;
  viewport.maxDepth = 1.f;

  vkCmdSetViewport(cmd, 0, 1, &viewport);

  VkRect2D scissor = {};
  scissor.extent.width = drawExtent.width;
  scissor.extent.height = drawExtent.height;

  vkCmdSetScissor(cmd, 0, 1, &scissor);

  GPUGeoPushConstants constants{};
  constants.matrix = {1.f};
  constants.vertexBuffer = meshes_["Suzanne"]->getVertexDeviceAddr();

  vkCmdPushConstants(cmd, pipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0,
                     sizeof(GPUGeoPushConstants), &constants);

  if (meshes_["Suzanne"]->meshBuffers.indicies_.empty()) {
    assert(!meshes_["Suzanne"]->meshBuffers.indicies_.empty() &&
           "Index buffer is empty!");
  }

  vkCmdBindIndexBuffer(
      cmd, meshes_["Suzanne"]->meshBuffers.indexBuffer.buffer, 0,
      getIndexType<decltype(meshes_["Suzanne"]->meshBuffers.indicies_[0])>());

  for (const GeoSurface &surface : meshes_["Suzanne"]->meshSurfaces) {
    vkCmdDrawIndexed(cmd,
                     surface.count,      // index
                     1,                  // instance
                     surface.startIndex, // index
                     0,                  // vertex
                     0                   // instance
    );
  }

  vkCmdEndRendering(cmd);
}

void GraphicPipelinePacked::submitMesh(VkCommandBuffer cmd) {
  // std::for_each(meshes_.begin(), meshes_.end(), [this,
  // cmd](decltype(*meshes_.begin())& asset) {
  //           asset.second->submitMesh(cmd);
  //           });
  meshes_["Suzanne"]->submitMesh(cmd);
}

void GraphicPipelinePacked::submitColorImage(VkCommandBuffer cmd) {
  black_->uploadBufferToImage(cmd);
  white_->uploadBufferToImage(cmd);
  grey_->uploadBufferToImage(cmd);
  magenta_->uploadBufferToImage(cmd);
  loaderrorImage_->uploadBufferToImage(cmd);
}

void GraphicPipelinePacked::flushUpload(VkFence fence) {
  // std::for_each(meshes_.begin(), meshes_.end(), [this,
  // fence](decltype(*meshes_.begin())& asset) {
  //           asset.second->flushUpload(fence);
  //           });
  meshes_["Suzanne"]->flushUpload(fence);

  black_->flushUpload(fence);
  white_->flushUpload(fence);
  grey_->flushUpload(fence);
  magenta_->flushUpload(fence);
  loaderrorImage_->flushUpload(fence);
}

std::function<void(VkCommandBuffer)> GraphicPipelinePacked::getMeshFunctor() {
  return [this](VkCommandBuffer cmd) { submitMesh(cmd); };
}
std::function<void(VkCommandBuffer)> GraphicPipelinePacked::getColorFunctor() {
  return [this](VkCommandBuffer cmd) { submitColorImage(cmd); };
}

void GraphicPipelinePacked::init_layout() {
  DescriptorLayoutBuilder builder{device_};
  sceneDescriptorSetLayout_ =
      builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
          .build(VK_SHADER_STAGE_VERTEX_BIT |
                 VK_SHADER_STAGE_FRAGMENT_BIT); // add bindings
}

void GraphicPipelinePacked::destroy_layout() {
  vkDestroyDescriptorSetLayout(device_, sceneDescriptorSetLayout_, nullptr);
}

void GraphicPipelinePacked::init_pipeline() { init_mesh_pipline(); }

void GraphicPipelinePacked::init_triangle_pipline() {
  if (isInit_)
    return;

  VkPipelineLayoutCreateInfo graphicLayout{};
  graphicLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  vkCreatePipelineLayout(device_, &graphicLayout, nullptr, &pipelineLayout_);

  GraphicPipelineBuilder builder{device_};
  builder.pipelineLayout_ = pipelineLayout_;
  pipeline_ = builder
                  .set_shaders(CONFIG_HOME "shaders/triangle.vert.spv",
                               CONFIG_HOME "shaders/triangle.frag.spv")
                  .disable_blending()
                  .set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                  .set_polygon_mode(VK_POLYGON_MODE_FILL)
                  .set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
                  .set_multisampling()
                  .set_depthtest(false)
                  .set_depth_format(VK_FORMAT_UNDEFINED)
                  .set_color_attachment_format(VK_FORMAT_R16G16B16A16_SFLOAT)
                  .build();

  vkDestroyShaderModule(device_, builder.shaderStages_[0].module, nullptr);
  vkDestroyShaderModule(device_, builder.shaderStages_[1].module, nullptr);

  init_finished(); // set isinit flag = true
}

void GraphicPipelinePacked::init_mesh_pipline() {
  VkPushConstantRange bufferRange{};
  bufferRange.offset = 0;
  bufferRange.size = sizeof(GPUGeoPushConstants);
  bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkPipelineLayoutCreateInfo graphicLayout{};
  graphicLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  graphicLayout.pushConstantRangeCount = 1;
  graphicLayout.pPushConstantRanges = &bufferRange;

  vkCreatePipelineLayout(device_, &graphicLayout, nullptr, &pipelineLayout_);

  GraphicPipelineBuilder builder{device_};
  builder.pipelineLayout_ = pipelineLayout_;

  pipeline_ = builder
                  .set_shaders(CONFIG_HOME "shaders/mesh.vert.spv",
                               CONFIG_HOME "shaders/triangle.frag.spv")
                  //.disable_blending()
                  .set_blending_alphablend(true)
                  .set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                  .set_polygon_mode(VK_POLYGON_MODE_FILL)
                  .set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
                  .set_multisampling()
                  .set_depthtest(VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL)
                  .set_depth_format(VK_FORMAT_D32_SFLOAT)
                  .set_color_attachment_format(VK_FORMAT_R16G16B16A16_SFLOAT)
                  .build();

  vkDestroyShaderModule(device_, builder.shaderStages_[0].module, nullptr);
  vkDestroyShaderModule(device_, builder.shaderStages_[1].module, nullptr);
}

void GraphicPipelinePacked::destroy_pipeline() {
  vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
  vkDestroyPipeline(device_, pipeline_, nullptr);
}

void GraphicPipelinePacked::load_asset(const std::string &name,
                                       std::vector<Vertex> &&vertices,
                                       std::vector<uint32_t> &&indices) {

  std::shared_ptr<MeshAsset> ptr =
      std::make_shared<MeshAsset>(device_, allocator_);
  ptr->meshName = name;
  ptr->createAsset(std::move(vertices), std::move(indices));

  load_asset(ptr);
}

void GraphicPipelinePacked::load_asset(std::shared_ptr<MeshAsset> asset) {

  if (!asset)
    return;

  auto [_, status] = meshes_.try_emplace(asset->meshName, asset);
  if (!status) {
    std::cerr << "Create Asset: " << asset->meshName << " Failed!\n";
  }
}

void GraphicPipelinePacked::load_asset(
    std::vector<std::shared_ptr<MeshAsset>> &&assets) {

  std::for_each(
      assets.begin(), assets.end(),
      [this](std::shared_ptr<MeshAsset> asset) { load_asset(asset); });
}

void GraphicPipelinePacked::destroy_default_colors() {
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

void GraphicPipelinePacked::load_default_colors() {

  white_.reset();
  grey_.reset();
  black_.reset();
  magenta_.reset();
  loaderrorImage_.reset();

  white_ = std::make_unique<AllocatedTexture>(device_, allocator_);
  grey_ = std::make_unique<AllocatedTexture>(device_, allocator_);
  black_ = std::make_unique<AllocatedTexture>(device_, allocator_);
  magenta_ = std::make_unique<AllocatedTexture>(device_, allocator_);
  loaderrorImage_ = std::make_unique<AllocatedTexture>(device_, allocator_);

  uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
  uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
  uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
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

} // namespace graphic
} // namespace engine
