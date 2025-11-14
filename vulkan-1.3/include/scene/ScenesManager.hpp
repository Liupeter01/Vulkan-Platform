#pragma once
#ifndef _SCENES_NODES_MANAGER_HPP_
#define _SCENES_NODES_MANAGER_HPP_
#include <compute/Compute_ImageAttachment.hpp>
#include <material/GLTFMetallic_Roughness.hpp>
#include <memory>
#include <nodes/scene/SceneNode.hpp>
#include <optional>
#include <particle/sprite/PointSpriteParticleSystem2D.hpp>
#include <particle/sprite/PointSpriteParticleSystem3D.hpp>
#include <string>
#include <tuple>
#include <unordered_map>

namespace engine {
class ScenesManager {
  ScenesManager(const ScenesManager &) = delete;
  ScenesManager &operator=(const ScenesManager &) = delete;

public:
  ScenesManager(VulkanEngine *eng);
  virtual ~ScenesManager();

public:
  void init();
  void destroy();
  void update_scene();
  void Draw(const glm::mat4 &parentMatrix, DrawContext &ctx);

  void render(VkCommandBuffer cmd, FrameData &frame);
  void compute(VkCommandBuffer cmd, FrameData &frame);

  bool addScene(std::shared_ptr<NodeManager> scene);

  ComputeShaderPushConstants &getComputeData();
  auto &getParticleData() {
    return particleSysCompute->getPushConstantData();
  }

  void submit();

  void on_gui();

protected:
  void destroy_scene();

  // Material
  void init_default_material();
  void destroy_default_material();

  // Compute
  void init_default_compute();
  void destroy_default_compute();

  void init_particle_sys();
  void destroy_particle_sys();

  // Pool
  void init_pool();
  void destroy_pool();

  void create_scene_set();
  void destroy_scene_set();

protected:
  void submitMesh(VkCommandBuffer cmd);
  void flushUpload(VkFence fence);

  void update_scene_set();
  VkDescriptorSet get_scene_set();

  [[nodiscard]]
  std::tuple<MaterialInstance, std::shared_ptr<AllocatedBuffer>>
  createDefaultMaterialInstance(FrameData &frame);

protected:
  const uint32_t setCount_ = 1000;
  const std::vector<PoolSizeRatio> frame_sizes = {
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
  };

private:
  bool isinit = false;
  VulkanEngine *engine_{};

  DrawContext ctx{}; // Export ALL subsurfaces

  struct SceneControl {
    /*  Graphic Scene Control System (set = 0, binding = 0 ) */
    GPUSceneData globalSceneData{}; // Scene Data For this scene only
    VkDescriptorSetLayout sceneDescriptorSetLayout_{};
    std::shared_ptr<AllocatedBuffer> sceneDataBuffer;
    VkDescriptorSet sceneDescriptorSet;
  } myScene{};

  // Node System(MeshNode, ...) or Scene Mgr
  std::unordered_map<
      /*scene name = */ std::string,
      /*scene nodes = */ std::shared_ptr<NodeManager>>
      loadedScenes_;

  std::unique_ptr<GLTFMetallic_Roughness> metalRoughMaterial{}; // Graphic
  std::unique_ptr<Compute_ImageAttachment<>>
      imageAttachmentCompute{}; // Compute

  std::unique_ptr<ParticleSysDataBuffer<particle::GPUParticle>>
      particleSysBuffer{};
  std::shared_ptr<particle::PointSpriteParticleSystemBase<>> particleSysCompute{};

  DescriptorPoolAllocator scenePool_;
};

} // namespace engine

#endif //_SCENES_NODES_MANAGER_HPP_
