#pragma once
#ifndef _MATERIAL_PIPELINE_HPP_
#define _MATERIAL_PIPELINE_HPP_
#include <builder/GraphicPipelineBuilder.hpp>

namespace engine {
enum class MaterialPass;

struct MaterialPipeline {
  MaterialPipeline(VkDevice device);
  virtual ~MaterialPipeline();

  void set_vertex_shader(const std::string& path, const std::string& entry);
  void set_fragment_shader(const std::string& path, const std::string& entry);
  void create(MaterialPass pass,
              const std::vector<VkDescriptorSetLayout> &layouts);

  void destroy();

  VkPipeline getPipeline() { return pipeline_; }
  VkPipelineLayout getPipelineLayout() { return pipelineLayout_; }

protected:
          std::vector<GraphicPipelineBuilder::ShaderStageDesc> stages{
                    GraphicPipelineBuilder::ShaderStageDesc{ 
                              GLSL_SHADER_PATH "pbr.vert.spv","main", VK_SHADER_STAGE_VERTEX_BIT },
                    GraphicPipelineBuilder::ShaderStageDesc{ 
                              GLSL_SHADER_PATH "pbr.frag.spv","main", VK_SHADER_STAGE_FRAGMENT_BIT }
          };

  void create_opaque_pipeline(GraphicPipelineBuilder &builder);
  void create_transparent_pipeline(GraphicPipelineBuilder &builder);

private:
  bool isinit_ = false;
  bool isUsingDefaultShader = true;

  VkDevice device_ = VK_NULL_HANDLE;
  VkPipeline pipeline_ = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
};
} // namespace engine

#endif //_MATERIAL_PIPELINE_HPP_
