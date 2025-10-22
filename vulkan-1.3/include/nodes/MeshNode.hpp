#pragma once
#ifndef _MESH_NODE_HPP_
#define _MESH_NODE_HPP_
#include <mesh/MeshLoader.hpp>
#include <nodes/BaseNode.hpp>

namespace engine {
namespace node {
struct MeshNode : public BaseNode {
  MeshNode(std::shared_ptr<BaseNode> base = nullptr);

  void Draw(const glm::mat4 &topMatrix, DrawContext &ctx) override;

  std::shared_ptr<MeshAsset> mesh_{};
};
} // namespace node
} // namespace engine

#endif // _MESH_NODE_HPP_
