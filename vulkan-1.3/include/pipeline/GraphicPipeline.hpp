#pragma once
#ifndef _GRAPHIC_PIPELINE_HPP_
#define _GRAPHIC_PIPELINE_HPP_
#include <string>
#include <vector>
#include <GlobalDef.hpp>
#include <pipeline/PipelineBasic.hpp>

namespace engine {

struct GraphicPipelineBuilder {

  void clear();
  VkPipeline build();
  GraphicPipelineBuilder(VkDevice device);

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

  const std::vector<VkDynamicState> dynamicStates_ = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
  };

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

inline namespace graphic {
// Graphic Pipeline
class GraphicPipelinePacked:public PipelineBasic {
public:
          std::string name = " GraphicPipelinePacked";
  GraphicPipelinePacked(VkDevice device);
  virtual ~GraphicPipelinePacked();

  GraphicPipelinePacked(const GraphicPipelinePacked&) = delete;
  GraphicPipelinePacked&operator=(const GraphicPipelinePacked&) = delete;
  void init() override;
  void destroy()override;
  void draw(VkCommandBuffer cmd, VkExtent2D drawExtent, VkImageView imageView)override;

protected:
  void init_pipeline();
  void destroy_pipeline();

  //// Initializing the layout and descriptors; store image, or vertex indicies
  // DescriptorAllocator descriptorAllocator_;
  // VkDescriptorSet drawCompDescriptor_;
  // VkDescriptorSetLayout drawCompDescriptorLayout_;
};
} // namespace graphic
} // namespace engine

#endif //_GRAPHIC_PIPELINE_HPP_
