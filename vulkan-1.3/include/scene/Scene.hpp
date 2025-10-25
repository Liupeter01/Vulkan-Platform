#pragma once
#ifndef _SCENE_HPP_
#define _SCENE_HPP_
#include <Descriptors.hpp>
#include <GlobalDef.hpp>
#include <compute/Compute_ImageAttachment.hpp>
#include <material/GLTFMetallic_Roughness.hpp>
#include <memory>
#include <nodes/MeshNode.hpp>
#include <nodes/NodeManager.hpp>
#include <tuple>

namespace engine {
// forward declaration
class VulkanEngine;

class Scene {
  Scene(const Scene &) = delete;
  Scene &operator=(const Scene &) = delete;

public:
  Scene(VulkanEngine *pointer);
  virtual ~Scene();

  void init(const std::string &root_name = "/root");
  void destroy();
  void update_scene();

  void render(VkCommandBuffer cmd, FrameData &frame);
  void compute(VkCommandBuffer cmd, FrameData &frame);

  bool attachChildren(const std::string &parentName,
                      std::shared_ptr<MeshAsset> asset);
  bool attachChildren(const std::string &parentName,
                      std::shared_ptr<node::BaseNode> child);
  bool attachChildrens(
      const std::string &parentName,
      const std::vector<std::shared_ptr<node::BaseNode>> &childrens);
  bool
  attachChildrens(const std::string &parentName,
                  const std::vector<std::shared_ptr<MeshAsset>> &childrens);

  ComputeShaderPushConstants &getComputeData();

  void submit();

protected:
  // Material
  void init_material();
  void destroy_material();

  // Compute
  void init_compute();
  void destroy_compute();

  // Scene Data
  void init_scene_layout();
  void destroy_scene_layout();
  [[nodiscard]] VkDescriptorSetLayout create_ubo_layout();

  // default color & default material
  void init_default_color();
  void init_default_sampler();

  void destroy_default_color();
  void destroy_default_sampler();

private:
  void submitMesh(VkCommandBuffer cmd);
  void submitColorImage(VkCommandBuffer cmd);
  void flushUpload(VkFence fence);

  [[nodiscard]]
  std::tuple<VkDescriptorSet, std::shared_ptr<AllocatedBuffer>>
  createSceneSet(FrameData &frame);

  [[nodiscard]]
  std::tuple<MaterialInstance, std::shared_ptr<AllocatedBuffer>>
  createDefaultMaterialInstance(FrameData &frame);

private:
  bool isInit = false;
  std::string last_mesh;

  VulkanEngine *engine{};

  DrawContext ctx{}; // Export ALL subsurfaces

  /*  Scene Control System (set = 0, binding = 0 ) */
  struct SceneControl {
    ComputeShaderPushConstants computeShaderData{};
    GPUSceneData globalSceneData{}; // Scene Data For this scene only
    VkDescriptorSetLayout sceneDescriptorSetLayout_{};
  } myScene{};

  // Node System(MeshNode, ...)
  node::NodeManager node_mgr;

  std::unique_ptr<GLTFMetallic_Roughness> metalRoughMaterial;
  std::unique_ptr<Compute_ImageAttachment> imageAttachmentCompute;

  // Default Color => Default Materal
  std::unique_ptr<AllocatedTexture> white_{};
  std::unique_ptr<AllocatedTexture> grey_{};
  std::unique_ptr<AllocatedTexture> black_{};
  std::unique_ptr<AllocatedTexture> magenta_{};
  std::unique_ptr<AllocatedTexture> loaderrorImage_{};

  VkSampler defaultSamplerLinear_;
  VkSampler defaultSamplerNearest_;
};
} // namespace engine

#endif //_SCENE_HPP_
