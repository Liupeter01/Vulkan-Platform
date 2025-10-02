#include <Model.hpp>
#include <Pipeline.hpp>
#include <fstream>
#include <istream>

engine::PipelineConf::PipelineConf() {

  rasterizationInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizationInfo.lineWidth = 1.0f;
  rasterizationInfo.depthBiasEnable = VK_FALSE;
  rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
  rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
  rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizationInfo.depthBiasClamp = rasterizationInfo.depthBiasConstantFactor =
      rasterizationInfo.depthBiasSlopeFactor = 0.0f;

  multisampleInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampleInfo.minSampleShading = 1.0f;
  multisampleInfo.pSampleMask = nullptr;
  multisampleInfo.sampleShadingEnable = multisampleInfo.alphaToOneEnable =
      multisampleInfo.alphaToCoverageEnable = VK_FALSE;

  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  colorBlendInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendInfo.logicOpEnable = VK_FALSE;
  colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
  colorBlendInfo.attachmentCount = 1;
  colorBlendInfo.pAttachments = &colorBlendAttachment;
  colorBlendInfo.blendConstants[0] = 0.0f;
  colorBlendInfo.blendConstants[1] = 0.0f;
  colorBlendInfo.blendConstants[2] = 0.0f;
  colorBlendInfo.blendConstants[3] = 0.0f;

  depthStencilInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencilInfo.depthTestEnable = VK_TRUE;
  depthStencilInfo.depthWriteEnable = VK_TRUE;
  depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
  depthStencilInfo.minDepthBounds = 0.0f; // Optional
  depthStencilInfo.maxDepthBounds = 1.0f; // Optional
  depthStencilInfo.stencilTestEnable = VK_FALSE;
  depthStencilInfo.front = {}; // Optional
  depthStencilInfo.back = {};  // Optional

  subpass = 0;

  inputAssemblyInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

  dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicStateInfo.dynamicStateCount =
      static_cast<uint32_t>(dynamicStateEnables.size());
  dynamicStateInfo.pDynamicStates = dynamicStateEnables.data();
  dynamicStateInfo.flags = 0;
}

void engine::PipelineConf::enableAlphaBlending(PipelineConf &conf) {
  conf.colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  // performance cost!
  conf.colorBlendAttachment.blendEnable = VK_TRUE;
  conf.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  conf.colorBlendAttachment.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  conf.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  conf.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  conf.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  conf.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

engine::Pipeline::Pipeline(MyEngineDevice &device, const std::string &vertPath,
                           const std::string &fragPath, PipelineConf &&conf)
    : device_(device), conf_(std::move(conf)) {
  createPipeline(vertPath, fragPath);
}

engine::Pipeline::~Pipeline() {
  vkDestroyShaderModule(device_.device(), vertShaderModule_, nullptr);
  vkDestroyShaderModule(device_.device(), fragShaderModule_, nullptr);
  vkDestroyPipeline(device_.device(), graphicPipeline_, nullptr);
}

std::vector<uint32_t> engine::Pipeline::readSpv(const std::string &path) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f)
    throw std::runtime_error("open failed: " + path);

  size_t sz = static_cast<size_t>(f.tellg());
  f.seekg(0);

  std::vector<uint32_t> buf((sz + 3) / 4, 0);

  f.read(reinterpret_cast<char *>(buf.data()), sz);
  if (!f)
    throw std::runtime_error("read failed: " + path);

  return buf;
}

void engine::Pipeline::bind(VkCommandBuffer commandBuffer) {

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphicPipeline_);
}

void engine::Pipeline::createPipeline(const std::string &vertPath,
                                      const std::string &fragPath) {

  loadShader(vertPath, &vertShaderModule_);
  loadShader(fragPath, &fragShaderModule_);

  VkPipelineShaderStageCreateInfo shaderStages[2]{};
  // Vert
  shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shaderStages[0].module = vertShaderModule_;
  shaderStages[0].pName = "main";
  shaderStages[0].flags = 0;
  shaderStages[0].pNext = nullptr;
  shaderStages[0].pSpecializationInfo = nullptr;
  // Frag
  shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shaderStages[1].module = fragShaderModule_;
  shaderStages[1].pName = "main";
  shaderStages[1].flags = 0;
  shaderStages[1].pNext = nullptr;
  shaderStages[1].pSpecializationInfo = nullptr;

  VkPipelineViewportStateCreateInfo viewportStateInfo{};
  viewportStateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportStateInfo.viewportCount = 1;
  viewportStateInfo.pViewports = nullptr;
  viewportStateInfo.scissorCount = 1;
  viewportStateInfo.pScissors = nullptr;

  auto attributes = Vertex::getAttributeDescriptions();
  auto bindings = Vertex::getBindingDescriptions();

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount =
      static_cast<uint32_t>(bindings.size());
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributes.size());
  vertexInputInfo.pVertexBindingDescriptions = bindings.data();
  vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &conf_.inputAssemblyInfo;
  pipelineInfo.pViewportState = &viewportStateInfo;
  pipelineInfo.pRasterizationState = &conf_.rasterizationInfo;
  pipelineInfo.pMultisampleState = &conf_.multisampleInfo;
  pipelineInfo.pColorBlendState = &conf_.colorBlendInfo;
  pipelineInfo.pDepthStencilState = &conf_.depthStencilInfo;
  pipelineInfo.pDynamicState = &conf_.dynamicStateInfo;

  pipelineInfo.layout = conf_.pipelineLayout;
  pipelineInfo.renderPass = conf_.renderPass;
  pipelineInfo.subpass = conf_.subpass;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex = -1;

  if (vkCreateGraphicsPipelines(device_.device(), VK_NULL_HANDLE, 1,
                                &pipelineInfo, nullptr,
                                &graphicPipeline_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create graphics pipeline!");
  }
}

void engine::Pipeline::loadShader(const std::string &shaderPath,
                                  VkShaderModule *shaderModule) {
  auto vec = std::move(readSpv(shaderPath));
  if (!vec.size())
    throw std::runtime_error("Create Shader Failed!");

  VkShaderModuleCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  info.codeSize = vec.size() * sizeof(uint32_t);
  info.pCode = vec.data();

  if (vkCreateShaderModule(device_.device(), &info, nullptr, shaderModule)) {
    throw std::runtime_error("vkCreateShaderModule Failed!");
  }
}
