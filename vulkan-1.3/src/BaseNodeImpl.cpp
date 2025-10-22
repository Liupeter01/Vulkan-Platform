#include <nodes/MeshNode.hpp>

namespace engine {
namespace node {
BaseNode::BaseNode(std::shared_ptr<BaseNode> base) : parent_(base) {}

void BaseNode::Draw(const glm::mat4 &parentMatrix, DrawContext &ctx) {

  for (auto &child : children) {
    child->Draw(parentMatrix, ctx);
  }
}

void BaseNode::refreshTransform(const glm::mat4 &parentMatrix) {
  worldTransform = parentMatrix * localTransform;
  for (auto &child : children)
    child->refreshTransform(worldTransform);
}

MeshNode::MeshNode(std::shared_ptr<BaseNode> base) : BaseNode(base) {}

void MeshNode::Draw(const glm::mat4 &topMatrix, DrawContext &ctx) {
  glm::mat4 nodeMatrix = topMatrix * worldTransform;

  for (const auto &surface : mesh_->meshSurfaces) {
    RenderObject object;
    object.transform = nodeMatrix;
    object.firstIndex = surface.startIndex;
    object.indexCount = surface.count;
    object.indexBuffer = mesh_->meshBuffers.indexBuffer.buffer;
    object.vertexBufferAddress = mesh_->meshBuffers.vertexBufferAddress;
    object.material = surface.material.get();
    object.mesh_name = this->node_name;
    ctx.OpaqueSurfaces.push_back(std::move(object));
  }

  BaseNode::Draw(topMatrix, ctx);
}
} // namespace node
} // namespace engine
