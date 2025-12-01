#pragma once
#ifndef _GRAPHIC_PIPELINE_BUILDER_HPP_
#define _GRAPHIC_PIPELINE_BUILDER_HPP_
#include <vector>
#include <vulkan/vulkan.hpp>

namespace engine {

struct GraphicPipelineBuilder {

  void clear();
  VkPipeline build();
  GraphicPipelineBuilder(VkDevice device);
  virtual ~GraphicPipelineBuilder();

  struct ShaderStageDesc {
    std::string path;
    std::string entry;
    VkShaderStageFlagBits stage;
  };

  GraphicPipelineBuilder &
  set_depthtest(bool status, VkCompareOp op = VK_COMPARE_OP_GREATER_OR_EQUAL);
  GraphicPipelineBuilder &disable_blending();
  GraphicPipelineBuilder &set_multisampling();
  GraphicPipelineBuilder &set_color_attachment_format(VkFormat format);
  GraphicPipelineBuilder &set_depth_format(VkFormat format);
  GraphicPipelineBuilder &
  set_shaders_stages(const std::vector<ShaderStageDesc> &stages);
  GraphicPipelineBuilder &set_blending_additive(bool status);
  GraphicPipelineBuilder &set_blending_alphablend(bool status);

  GraphicPipelineBuilder &set_input_topology(
      const VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      const bool restart = VK_FALSE);

  GraphicPipelineBuilder &
  set_polygon_mode(const VkPolygonMode mode = VK_POLYGON_MODE_FILL,
                   const float linewidth = 1.f);

  GraphicPipelineBuilder &set_cull_mode(const VkCullModeFlags cullMode,
                                        const VkFrontFace frontFace);

  const std::vector<VkDynamicState> dynamicStates_ = {VK_DYNAMIC_STATE_VIEWPORT,
                                                      VK_DYNAMIC_STATE_SCISSOR};

  std::vector<VkPipelineShaderStageCreateInfo> shaderStages_;
  VkPipelineInputAssemblyStateCreateInfo inputAssembly_{};
  VkPipelineRasterizationStateCreateInfo rasterizer_{};
  VkPipelineColorBlendStateCreateInfo colorBlendState_{};
  VkPipelineColorBlendAttachmentState colorBlendAttachment_{};
  VkPipelineMultisampleStateCreateInfo multisampling_{};
  VkPipelineLayout pipelineLayout_{};
  VkPipelineDepthStencilStateCreateInfo depthStencil_{};
  VkPipelineRenderingCreateInfo renderInfo_{};
  VkFormat colorAttachmentformat_{};
  VkPipelineDynamicStateCreateInfo dynamicInfo_{};
  VkPipelineViewportStateCreateInfo viewportState_{};
  VkPipelineVertexInputStateCreateInfo vertexInputInfo_{};

protected:
  void init_dynamic_state();
  void init_viewport_state();
  void init_colorBlend_state();

private:
  VkDevice device_;
};
} // namespace engine

#endif //_GRAPHIC_PIPELINE_BUILDER_HPP_
