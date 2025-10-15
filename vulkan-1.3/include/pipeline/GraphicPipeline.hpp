#pragma once
#ifndef _GRAPHIC_PIPELINE_HPP_
#define _GRAPHIC_PIPELINE_HPP_
#include <GlobalDef.hpp>
#include <functional>
#include <mesh/MeshLoader.hpp>
#include <pipeline/PipelineBasic.hpp>
#include <string>
#include <type_traits>
#include <vector>

namespace engine {

struct GraphicPipelineBuilder {

  void clear();
  VkPipeline build();
  GraphicPipelineBuilder(VkDevice device);

  GraphicPipelineBuilder &
  set_depthtest(bool status, VkCompareOp op = VK_COMPARE_OP_GREATER_OR_EQUAL);
  GraphicPipelineBuilder &disable_blending();
  GraphicPipelineBuilder &set_multisampling();
  GraphicPipelineBuilder &set_color_attachment_format(VkFormat format);
  GraphicPipelineBuilder &set_depth_format(VkFormat format);
  GraphicPipelineBuilder &set_shaders(const std::string &vertexShaderPath,
                                      const std::string &fragmentShaderPath);

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

inline namespace graphic {
// Graphic Pipeline
class GraphicPipelinePacked : public PipelineBasic {
public:
  std::string name = " GraphicPipelinePacked";
  GraphicPipelinePacked(VkDevice device, VmaAllocator allocator);
  virtual ~GraphicPipelinePacked();

  GraphicPipelinePacked(const GraphicPipelinePacked &) = delete;
  GraphicPipelinePacked &operator=(const GraphicPipelinePacked &) = delete;

  void init() override;
  void destroy() override;
  void draw(VkExtent2D drawExtent, AllocatedImage &offscreen_draw,
            AllocatedImage &offscreen_depth, FrameData &curr_frame) override;

  void load_asset(const std::string &name, std::vector<Vertex> &&vertices,
                  std::vector<uint32_t> &&indices);
  void load_asset(std::shared_ptr<MeshAsset> asset);
  void load_asset(std::vector<std::shared_ptr<MeshAsset>> &&assets);

  void submitMesh(VkCommandBuffer cmd);
  void submitColorImage(VkCommandBuffer cmd);
  void flushUpload(VkFence fence);

  [[nodiscard]]
  std::function<void(VkCommandBuffer)> getMeshFunctor();

  [[nodiscard]]
  std::function<void(VkCommandBuffer)> getColorFunctor();

  template <typename T> inline static constexpr VkIndexType getIndexType() {
    using Decayed = std::decay_t<T>;
    if constexpr (std::is_same_v<Decayed, uint32_t>) {
      return VK_INDEX_TYPE_UINT32;
    } else if constexpr (std::is_same_v<Decayed, uint16_t>) {
      return VK_INDEX_TYPE_UINT16;
    } else if constexpr (std::is_same_v<Decayed, uint8_t>) {
      return VK_INDEX_TYPE_UINT8;
    } else {
      static_assert(false, "Unsupported index type");
    }
  }

protected:
  void init_layout();
  void init_pipeline();
  void init_default_colors();
  void init_sampler();

  void destroy_sampler();
  void destroy_default_colors();
  void destroy_layout();
  void destroy_pipeline();

private:
  void init_triangle_pipline();
  void init_mesh_pipline();
  [[nodiscard]] VkDescriptorSetLayout create_ubo_layout();
  [[nodiscard]] VkDescriptorSetLayout create_sampler_layout();

private:

  std::unique_ptr<AllocatedTexture> white_{};
  std::unique_ptr<AllocatedTexture> grey_{};
  std::unique_ptr<AllocatedTexture> black_{};
  std::unique_ptr<AllocatedTexture> magenta_{};
  std::unique_ptr<AllocatedTexture> loaderrorImage_{};

  VkSampler defaultSamplerLinear_;
  VkSampler defaultSamplerNearest_;

  GPUSceneData sceneData_{};

  //VkDescriptorSetLayout Configs
  // 1. sceneDescriptorSetLayout_;
 //  2. singleImageDescriptorSetLayout_;
  std::array<VkDescriptorSetLayout, 2> setLayouts_;

  std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes_;
};
} // namespace graphic
} // namespace engine

#endif //_GRAPHIC_PIPELINE_HPP_
