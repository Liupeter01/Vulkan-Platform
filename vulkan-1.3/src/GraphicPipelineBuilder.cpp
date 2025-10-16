#include <Tools.hpp>
#include <pipeline/GraphicPipelineBuilder.hpp>

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
} // namespace engine
