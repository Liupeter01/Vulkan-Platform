#pragma once
#ifndef _NODE_BASE_HPP_
#define _NODE_BASE_HPP_
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <mesh/MeshLoader.hpp>

#define GLM_FORCE_RADIANS // no degresss
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace engine {
struct MaterialInstance;
struct KeyBoardController;
struct SceneNodeBuilder;
struct NodeManager;

struct RenderObject {
  uint32_t indexCount;
  uint32_t firstIndex;
  VkBuffer indexBuffer;

  MaterialInstance *material;

  std::string mesh_name;

  glm::mat4 transform;
  VkDeviceAddress vertexBufferAddress;

  mesh::MeshAsset* parent{};
};

struct TransformComponent {
  friend struct engine::KeyBoardController;
  friend struct engine::SceneNodeBuilder;

  enum class RotationControll { Quat, EularAngle };

  void setRotation(const glm::vec3 &euler);
  void setQuaternion(const glm::quat &q);

  glm::mat4
  mat4(RotationControll ctrl = RotationControll::EularAngle) const noexcept;
  void update();

  glm::vec3 scale{1.f};
  glm::vec3 translation{0.f};
  glm::vec3 rotation{};

protected:
  // could be disabled at the begining stage
  mutable glm::mat4 localMatrix{1.f};
  mutable bool dirty = true;
  glm::quat quat{};
};

struct DrawContext {
  std::vector<RenderObject> OpaqueSurfaces;
};

namespace node {
class SceneNode;

struct IRenderable {
  virtual void Draw(const glm::mat4 &topMatrix, DrawContext &ctx) = 0;
};

struct BaseNode : public IRenderable {
  friend struct engine::NodeManager;
  friend struct engine::SceneNodeBuilder;
  friend class engine::node::SceneNode;

  BaseNode(std::shared_ptr<BaseNode> base = nullptr);

  void Draw(const glm::mat4 &parentMatrix, DrawContext &ctx) override;
  void refreshTransform(const glm::mat4 &parentMatrix);
  void setupTransform(const TransformComponent &Model);

  TransformComponent localTransform;

  // glm::mat4 localTransform{1.f};
  glm::mat4 worldTransform{1.f};

  std::string node_name{};
  std::string node_path{};

protected:
  // parent pointer must be a weak pointer to avoid circular dependencies
  std::weak_ptr<BaseNode> parent_;

  // Adjacent Nodes
  std::vector<std::shared_ptr<BaseNode>> children;
};
} // namespace node
} // namespace engine

#endif //_NODE_BASE_HPP_
