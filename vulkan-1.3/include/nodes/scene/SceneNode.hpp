#pragma once
#ifndef _SCENE_NODE_HPP_
#define _SCENE_NODE_HPP_
#include <Descriptors.hpp>
#include <GlobalDef.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <material/GLTFMetallic_Roughness.hpp>
#include <memory>
#include <mesh/MeshLoader.hpp>
#include <nodes/core/BaseNode.hpp>
#include <nodes/scene/NodeManager.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace engine {

struct SceneNodeBuilder;

namespace node {

struct SceneNodeConf {
  std::string sceneRootName = "__custom_scene__";
  uint32_t setCount = 1;
  std::vector<PoolSizeRatio> poolSizeRatio {
    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1} };

  VkDescriptorSetLayout globalSceneLayout;
};

class SceneNode : public NodeManager {

  /*
   * Those Data Structures Are already Included in NodeManager
   * std::shared_ptr<node::BaseNode> sceneRoot_ = nullptr;
   * std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes_;
   * std::unordered_map<std::string, std::shared_ptr<node::BaseNode>> nodes_;
   */

public:
  friend struct engine::SceneNodeBuilder;
  SceneNode(VulkanEngine *eng);
  virtual ~SceneNode();

public:
  void init(const SceneNodeConf &conf);
  void destroy();
  void reset_allocator_pools();
  void Draw(const glm::mat4 &topMatrix, DrawContext &ctx) override;

protected:
  void init_allocator(const uint32_t setCount,
                      const std::vector<PoolSizeRatio> &poolSizeRatio);

  void init_material(VkDescriptorSetLayout globalSceneLayout);

  void destroy_material();
  void destroy_allocator();
  void destroy_sampler();
  void destroy_images();

  bool insert_to_lookup_table(std::shared_ptr<node::BaseNode> &node,
                              std::string_view parentPath);

private:
  bool isinit = false;
  VulkanEngine *engine_;
  DescriptorPoolAllocator pool_;

  // storage for all the data on a given glTF file
  std::unordered_map<std::string, std::shared_ptr<AllocatedTexture>> images_;
  std::unordered_map<std::string, std::shared_ptr<MaterialInstance>> materials_;

  std::unique_ptr<GLTFMetallic_Roughness> metalRoughMaterial = nullptr;

  std::shared_ptr<AllocatedBuffer> materialBuffer = nullptr;

  std::vector<VkSampler> samplers;
};

} // namespace node
} // namespace engine

#endif //_SCENE_NODE_HPP_
