#pragma once
#ifndef _MATERIAL_PIPELINE_HPP_
#define _MATERIAL_PIPELINE_HPP_
#include <pipeline/GraphicPipelineBuilder.hpp>

namespace engine {
enum class MaterialPass;

struct MaterialPipeline {
  MaterialPipeline(VkDevice device);
  virtual ~MaterialPipeline();

  void create(MaterialPass pass,
              const std::vector<VkDescriptorSetLayout> &layouts);
  void destroy();

  VkPipeline getPipeline() { return pipeline_; }
  VkPipelineLayout getPipelineLayout() { return pipelineLayout_; }

protected:
  void create_opaque_pipeline(GraphicPipelineBuilder &builder);
  void create_transparent_pipeline(GraphicPipelineBuilder &builder);

private:
  bool isinit_ = false;
  VkDevice device_ = VK_NULL_HANDLE;

  VkPipeline pipeline_ = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
};
} // namespace engine

#endif //_MATERIAL_PIPELINE_HPP_
