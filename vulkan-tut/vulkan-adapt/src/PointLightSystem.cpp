#include <GlobalDef.hpp>
#include <system/PointLightSystem.hpp>

#define GLM_FORCE_RADIANS // no degresss
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

engine::PointLightSystem::PointLightSystem(MyEngineDevice &device,
                                           VkRenderPass renderpass,
                                           VkDescriptorSetLayout layout)
    : device_(device) {
  createPipelineLayout(layout);
  createPipeline(renderpass);
}

engine::PointLightSystem::~PointLightSystem() {
  vkDestroyPipelineLayout(device_.device(), pipelineLayout_, nullptr);
}

void engine::PointLightSystem::updateLightObjOnly(FrameConfigInfo &info,
                                                  GlobalUBO &ubo) {

  int numOfLights{0};
  for (auto &obj : info.gameObjs) {
    // filter non light objects
    if (!obj.second.pointLightAttribute_)
      continue;

    auto R = glm::rotate(glm::one<glm::mat4>(), info.timeFrame,
                         glm::vec3{0.f, -1.f, 0.f});
    obj.second.transform.translation =
        glm::vec3(R * glm::vec4{obj.second.transform.translation, 1});

    ubo.pointLights[numOfLights].color = glm::vec4(
        obj.second.color, obj.second.pointLightAttribute_->lightIntensity);
    ubo.pointLights[numOfLights].position =
        glm::vec4(obj.second.transform.translation, 1.f);
    ++numOfLights;
  }
  ubo.numLights = numOfLights;
}

void engine::PointLightSystem::renderGameObject(FrameConfigInfo &info) {

  std::map</*distance*/ float, GameObject::id_t> distance_to_camera;

  for (auto &obj : info.gameObjs) {
    // filter non light objects
    if (!obj.second.pointLightAttribute_)
      continue;

    const glm::vec3 C2L =
        info.camera.getCameraPosition() - obj.second.transform.translation;
    const float distanceSquare = glm::dot(C2L, C2L);
    if (-std::numeric_limits<float>::epsilon() < distanceSquare &&
        distanceSquare < std::numeric_limits<float>::epsilon()) {
      continue;
    }
    distance_to_camera[distanceSquare] = obj.second.getId();
  }

  pipeline_->bind(info.buffer);

  vkCmdBindDescriptorSets(info.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipelineLayout_, 0, 1, &info.sets, 0, nullptr);

  for (auto ib = distance_to_camera.rbegin(); ib != distance_to_camera.rend();
       ++ib) {

    auto &obj = info.gameObjs.at(ib->second);

    PushConstantPointLight push{};
    push.color = glm::vec4(obj.color, obj.pointLightAttribute_->lightIntensity);
    push.position = glm::vec4(obj.transform.translation, 1.f);
    push.radius = obj.transform.scale.x;

    vkCmdPushConstants(info.buffer, pipelineLayout_,
                       VK_SHADER_STAGE_VERTEX_BIT |
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(PushConstantPointLight), &push);

    vkCmdDraw(info.buffer, 6, 1, 0, 0);
  }
}

void engine::PointLightSystem::createPipelineLayout(
    VkDescriptorSetLayout layout) {
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(PushConstantPointLight);

  std::vector<VkDescriptorSetLayout> descriptorSetLayouts{layout};

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount =
      static_cast<uint32_t>(descriptorSetLayouts.size());
  pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
  // pipelineLayoutInfo.pushConstantRangeCount = 0;
  // pipelineLayoutInfo.pPushConstantRanges = nullptr;

  if (vkCreatePipelineLayout(device_.device(), &pipelineLayoutInfo, nullptr,
                             &pipelineLayout_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }
}

void engine::PointLightSystem::createPipeline(VkRenderPass renderpass) {
  PipelineConf pipelineConf;
  pipelineConf.renderPass = renderpass;
  pipelineConf.pipelineLayout = pipelineLayout_;
  PipelineConf::enableAlphaBlending(pipelineConf);

  pipeline_ = std::make_unique<Pipeline>(
      device_, CONFIG_HOME "shaders/point_light_vertex.vert.spv",
      CONFIG_HOME "shaders/point_light_fragment.frag.spv",
      std::move(pipelineConf));
}
