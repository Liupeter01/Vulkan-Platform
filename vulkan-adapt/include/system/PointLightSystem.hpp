#pragma once
#ifndef _POINTLIGHTSYSTEM_HPP_
#define _POINTLIGHTSYSTEM_HPP_
#include <Camera.hpp>
#include <Descriptors.hpp>
#include <GameObject.hpp>
#include <Model.hpp>
#include <Pipeline.hpp>
#include <map> //sorting purpose

namespace engine {
struct FrameConfigInfo;
struct PushConstantPointLight;

class PointLightSystem {
public:
  PointLightSystem(PointLightSystem &&) = delete;
  PointLightSystem &operator=(PointLightSystem &&) = delete;

  PointLightSystem(MyEngineDevice &device, VkRenderPass renderpass,
                   VkDescriptorSetLayout layout);
  virtual ~PointLightSystem();

  void updateLightObjOnly(FrameConfigInfo &info, GlobalUBO &ubo);
  void renderGameObject(FrameConfigInfo &info);

protected:
  void createPipelineLayout(VkDescriptorSetLayout layout);
  void createPipeline(VkRenderPass renderpass);

private:
  MyEngineDevice &device_;
  std::unique_ptr<Pipeline> pipeline_;
  VkPipelineLayout pipelineLayout_;
};
} // namespace engine
#endif //_POINTLIGHTSYSTEM_HPP_
