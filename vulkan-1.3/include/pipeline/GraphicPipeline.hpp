#pragma once
#ifndef _GRAPHIC_PIPELINE_HPP_
#define _GRAPHIC_PIPELINE_HPP_
#include <GlobalDef.hpp>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace engine {

struct GraphicPipelineBuilder {

  void clear();
  VkPipeline build();
  GraphicPipelineBuilder(VkDevice device);

  void init_dynamic_state();
  void init_viewport_state();
  void init_colorBlend_state();

  GraphicPipelineBuilder &set_depthtest(bool status = VK_FALSE);
  GraphicPipelineBuilder &set_blending(bool status = VK_FALSE);
  GraphicPipelineBuilder &set_multisampling();
  GraphicPipelineBuilder &set_color_attachment_format(VkFormat format);
  GraphicPipelineBuilder &set_depth_format(VkFormat format);
  GraphicPipelineBuilder &set_shaders(const std::string &vertexShaderPath,
                                      const std::string &fragmentShaderPath);

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

private:
  VkDevice device_;
};

inline namespace graphic {
// Graphic Pipeline
struct GraphicPipelinePacked {
  std::string name = " ComputePipelinePacked";

  //// Initializing the layout and descriptors; store image, or vertex indicies
  // DescriptorAllocator descriptorAllocator_;
  // VkDescriptorSet drawCompDescriptor_;
  // VkDescriptorSetLayout drawCompDescriptorLayout_;

  // VkPipeline gradientComputePipeline_;
  // VkPipelineLayout gradientComputePipelineLayout_;
};
} // namespace graphic
} // namespace engine

#endif //_GRAPHIC_PIPELINE_HPP_
