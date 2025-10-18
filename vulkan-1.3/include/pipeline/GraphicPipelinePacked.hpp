#pragma once
#ifndef _GRAPHIC_PIPELINE_PACKED_HPP_
#define _GRAPHIC_PIPELINE_PACKED_HPP_
#include <GlobalDef.hpp>
#include <functional>
#include <material/GLTFMetallic_Roughness.hpp>
#include <mesh/MeshLoader.hpp>
#include <string>
#include <type_traits>
#include <vector>

namespace engine {
inline namespace graphic {
// Graphic Pipeline
class GraphicPipelinePacked{
public:
  std::string name = " GraphicPipelinePacked";
  GraphicPipelinePacked(VkDevice device, VmaAllocator allocator);
  virtual ~GraphicPipelinePacked();

  GraphicPipelinePacked(const GraphicPipelinePacked &) = delete;
  GraphicPipelinePacked &operator=(const GraphicPipelinePacked &) = delete;

  void init() ;
  void destroy() ;
  void draw(VkExtent2D drawExtent, AllocatedImage &offscreen_draw,
            AllocatedImage &offscreen_depth, FrameData &curr_frame) ;

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
  void init_default_colors();
  void init_sampler();

  void destroy_sampler();
  void destroy_default_colors();
  void destroy_layout();

private:
  [[nodiscard]] VkDescriptorSetLayout create_ubo_layout();

private:
  std::unique_ptr<AllocatedTexture> white_{};
  std::unique_ptr<AllocatedTexture> grey_{};
  std::unique_ptr<AllocatedTexture> black_{};
  std::unique_ptr<AllocatedTexture> magenta_{};
  std::unique_ptr<AllocatedTexture> loaderrorImage_{};

  VkSampler defaultSamplerLinear_;
  VkSampler defaultSamplerNearest_;

  GPUSceneData sceneData_{};
  VkDescriptorSetLayout sceneDescriptorSetLayout_;
  GLTFMetallic_Roughness metalRoughMaterial;

  bool isInit_  = false;
  VkDevice device_;
  VmaAllocator allocator_;

  std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes_;
};
} // namespace graphic
} // namespace engine

#endif //_GRAPHIC_PIPELINE_PACKED_HPP_
