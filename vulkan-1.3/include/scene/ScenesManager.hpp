#pragma once
#ifndef _SCENES_NODES_MANAGER_HPP_
#define _SCENES_NODES_MANAGER_HPP_
#include <tuple>
#include <memory>
#include <nodes/scene/SceneNode.hpp>
#include <optional>
#include <string>
#include <memory>
#include <unordered_map>
#include <compute/Compute_ImageAttachment.hpp>
#include <material/GLTFMetallic_Roughness.hpp>

namespace engine {
class ScenesManager {
          ScenesManager(const ScenesManager&) = delete;
          ScenesManager& operator=(const ScenesManager&) = delete;

public:
  ScenesManager(VulkanEngine *eng);
  virtual ~ScenesManager();

public:
  void init();
  void destroy();
  void update_scene();
  void Draw(const glm::mat4 &parentMatrix, DrawContext &ctx);

  void render(VkCommandBuffer cmd, FrameData& frame);
  void compute(VkCommandBuffer cmd, FrameData& frame);

  bool addScene(std::shared_ptr<NodeManager> scene);

  ComputeShaderPushConstants& getComputeData();

  void submit();

protected:
  void destroy_scene();

  // Material
  void init_default_material();
  void destroy_default_material();

  // Compute
  void init_default_compute();
  void destroy_default_compute();

protected:
          void submitMesh(VkCommandBuffer cmd);
          void submitColorImage(VkCommandBuffer cmd);
          void flushUpload(VkFence fence);

          [[nodiscard]]
          std::tuple<VkDescriptorSet, std::shared_ptr<AllocatedBuffer>>
                    createSceneSet(FrameData& frame);

          [[nodiscard]]
          std::tuple<MaterialInstance, std::shared_ptr<AllocatedBuffer>>
                    createDefaultMaterialInstance(FrameData& frame);

private:
          bool isinit = false;
          std::string last_mesh;

  VulkanEngine *engine_{};

  DrawContext ctx{}; // Export ALL subsurfaces

  /*  Scene Control System (set = 0, binding = 0 ) */
  struct SceneControl {
            ComputeShaderPushConstants computeShaderData{};
            GPUSceneData globalSceneData{}; // Scene Data For this scene only
            VkDescriptorSetLayout sceneDescriptorSetLayout_{};
  } myScene{};

  // Node System(MeshNode, ...) or Scene Mgr
  std::unordered_map<
      /*scene name = */ std::string,
      /*scene nodes = */ std::shared_ptr<NodeManager>>
      loadedScenes_;

  std::unique_ptr<GLTFMetallic_Roughness> metalRoughMaterial{};
  std::unique_ptr<Compute_ImageAttachment> imageAttachmentCompute{};
};

} // namespace engine

#endif //_SCENES_NODES_MANAGER_HPP_
