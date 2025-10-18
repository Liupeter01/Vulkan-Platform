#pragma once
#ifndef _NODE_BASE_HPP_
#define _NODE_BASE_HPP_
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS // no degresss
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace engine {
struct MaterialInstance;
class Scene;

struct RenderObject {
  uint32_t indexCount;
  uint32_t firstIndex;
  VkBuffer indexBuffer;

  MaterialInstance *material;

  glm::mat4 transform;
  VkDeviceAddress vertexBufferAddress;
};

struct DrawContext {
  std::vector<RenderObject> OpaqueSurfaces;
};

namespace node {
struct IRenderable {
  virtual void Draw(const glm::mat4 &topMatrix, DrawContext &ctx) = 0;
};

struct BaseNode : public IRenderable {
  friend class Scene;
  BaseNode(std::shared_ptr<BaseNode> base = nullptr);

  virtual void Draw(const glm::mat4 &parentMatrix, DrawContext &ctx) override;
  void refreshTransform(const glm::mat4 &parentMatrix);

  glm::mat4 localTransform{1.f};
  glm::mat4 worldTransform{1.f};

protected:
  // parent pointer must be a weak pointer to avoid circular dependencies
  std::weak_ptr<BaseNode> parent_;

  // Adjacent Nodes
  std::vector<std::shared_ptr<BaseNode>> children;
};
} // namespace node
} // namespace engine

#endif //_NODE_BASE_HPP_
