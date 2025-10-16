#pragma once
#ifndef _GRAPHIC_PIPELINE_HPP_
#define _GRAPHIC_PIPELINE_HPP_
#include <GlobalDef.hpp>
#include <functional>
#include <mesh/MeshLoader.hpp>
#include <pipeline/PipelineBasic.hpp>
#include <pipeline/GraphicPipelineBuilder.hpp>
#include <material/GLTFMetallic_Roughness.hpp>
#include <string>
#include <type_traits>
#include <vector>

namespace engine {
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

  // VkDescriptorSetLayout Configs
  //  1. sceneDescriptorSetLayout_;
  //  2. singleImageDescriptorSetLayout_;
  std::array<VkDescriptorSetLayout, 2> setLayouts_;

  GLTFMetallic_Roughness metalRoughMaterial;

  std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes_;
};
} // namespace graphic
} // namespace engine

#endif //_GRAPHIC_PIPELINE_HPP_
