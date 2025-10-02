#pragma once
#ifndef _TUT_RENDER_SYSTEM_HPP_
#define _TUT_RENDER_SYSTEM_HPP_
#include <Camera.hpp>
#include <Descriptors.hpp>
#include <GameObject.hpp>
#include <Model.hpp>
#include <Pipeline.hpp>

namespace engine {

struct PushConstantTut;
class TutRenderSystem {
public:
  TutRenderSystem(TutRenderSystem &&) = delete;
  TutRenderSystem &operator=(TutRenderSystem &&) = delete;

  TutRenderSystem(MyEngineDevice &device, VkRenderPass renderpass,
                  VkDescriptorSetLayout layout);
  virtual ~TutRenderSystem();

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

#endif //_RENDER_SYSTEM_HPP_
