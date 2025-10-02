#include <GlobalDef.hpp>
#include <system/TutRenderSystem.hpp>

engine::TutRenderSystem::TutRenderSystem(MyEngineDevice &device,
                                         VkRenderPass renderpass,
                                         VkDescriptorSetLayout layout)
    : device_(device) {
  createPipelineLayout(layout);
  createPipeline(renderpass);
}

engine::TutRenderSystem::~TutRenderSystem() {
  vkDestroyPipelineLayout(device_.device(), pipelineLayout_, nullptr);
}

void engine::TutRenderSystem::renderGameObject(FrameConfigInfo &info) {

  pipeline_->bind(info.buffer);

  vkCmdBindDescriptorSets(info.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipelineLayout_, 0, 1, &info.sets, 0, nullptr);

  for (auto &obj : info.gameObjs) {

    if (obj.second.pointLightAttribute_)
      continue;

    PushConstantTut push{};
    push.Model = obj.second.transform.mat4();
    push.Normal_Matrix = obj.second.transform.normalMatrix(); //

    vkCmdPushConstants(info.buffer, pipelineLayout_,
                       VK_SHADER_STAGE_VERTEX_BIT |
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(PushConstantTut), &push);

    obj.second.model_->bind(info.buffer);
    obj.second.model_->draw(info.buffer);
  }
}

void engine::TutRenderSystem::createPipelineLayout(
    VkDescriptorSetLayout layout) {
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(PushConstantTut);

  std::vector<VkDescriptorSetLayout> descriptorSetLayouts{layout};

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount =
      static_cast<uint32_t>(descriptorSetLayouts.size());
  pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(device_.device(), &pipelineLayoutInfo, nullptr,
                             &pipelineLayout_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }
}

void engine::TutRenderSystem::createPipeline(VkRenderPass renderpass) {
  PipelineConf pipelineConf;
  pipelineConf.renderPass = renderpass;
  pipelineConf.pipelineLayout = pipelineLayout_;
  pipeline_ = std::make_unique<Pipeline>(
      device_, CONFIG_HOME "shaders/tut_vertex.vert.spv",
      CONFIG_HOME "shaders/tut_fragment.frag.spv", std::move(pipelineConf));
}
