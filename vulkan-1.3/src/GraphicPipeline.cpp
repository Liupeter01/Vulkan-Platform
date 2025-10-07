#include <Tools.hpp>
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

GraphicPipelineBuilder &GraphicPipelineBuilder::set_blending(bool status) {
  colorBlendAttachment_.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment_.blendEnable = status;
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
GraphicPipelineBuilder::set_input_topology(const VkPrimitiveTopology topology,
                                           const bool restart) {
  inputAssembly_.topology = topology;
  inputAssembly_.primitiveRestartEnable = restart;
  return *this;
}

GraphicPipelineBuilder &GraphicPipelineBuilder::set_depthtest(bool status) {
  depthStencil_.depthTestEnable = status;
  depthStencil_.depthWriteEnable = status;
  depthStencil_.depthCompareOp = VK_COMPARE_OP_NEVER;
  depthStencil_.depthBoundsTestEnable = status;
  depthStencil_.stencilTestEnable = status;
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

          GraphicPipelinePacked::GraphicPipelinePacked(VkDevice device)
                    :PipelineBasic(device, PipelineType::GRAPHIC) {}

          GraphicPipelinePacked::~GraphicPipelinePacked() { destroy(); }

          void GraphicPipelinePacked::init() {
                    init_pipeline();
          }

          void GraphicPipelinePacked::destroy() {
                    if (isInit_) {
                              vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
                              vkDestroyPipeline(device_, pipeline_, nullptr);
                              reset_init();
                    }
          }

          void GraphicPipelinePacked::draw(VkCommandBuffer cmd, VkExtent2D drawExtent, VkImageView imageView) {

                    auto colorAttachmentInfo = tools::attachment_info(imageView);
                    auto renderInfo =
                              tools::rendering_info(drawExtent, &colorAttachmentInfo);

                    vkCmdBeginRendering(cmd, &renderInfo);

                    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

                    //set dynamic viewport and scissor
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

                    vkCmdDraw(cmd, /*vertex count*/3, 1, 0, 0);

                    vkCmdEndRendering(cmd);
          }

          void GraphicPipelinePacked::init_pipeline() {
                    if (isInit_) return;

                    VkPipelineLayoutCreateInfo computeLayout{};
                    computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

                    vkCreatePipelineLayout(device_, &computeLayout, nullptr,
                              &pipelineLayout_);

                    GraphicPipelineBuilder builder{ device_ } ;
                    builder.pipelineLayout_ = pipelineLayout_;

                    pipeline_ = builder.set_shaders(CONFIG_HOME"shaders/triangle.vert.spv",
                              CONFIG_HOME"shaders/triangle.frag.spv")
                              .set_blending(false)
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

                    init_finished();    //set isinit flag = true
          }
}

} // namespace engine
