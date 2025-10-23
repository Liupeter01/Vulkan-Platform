#include <nodes/MeshNode.hpp>

#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace engine {

          glm::mat4 TransformComponent::mat4() const{
                    const auto T = glm::translate(glm::one<glm::mat4>(), translation);
                    const auto R = glm::eulerAngleYXZ(rotation.y, rotation.x, rotation.z);
                    return T * R * glm::scale(glm::one<glm::mat4>(), scale);
          }

          glm::mat3 TransformComponent::normalMatrix() const {
                    const auto inv_S = glm::inverse(glm::scale(glm::one<glm::mat4>(), scale));
                    const auto R = glm::eulerAngleYXZ(rotation.y, rotation.x, rotation.z);
                    return { R * inv_S };
          }

namespace node {
BaseNode::BaseNode(std::shared_ptr<BaseNode> base) : parent_(base) {}

void BaseNode::Draw(const glm::mat4 &parentMatrix, DrawContext &ctx) {

  for (auto &child : children) {
    child->Draw(parentMatrix, ctx);
  }
}

void BaseNode::refreshTransform(const glm::mat4 &parentMatrix) {
  worldTransform = parentMatrix * localTransform.mat4();
  for (auto &child : children)
    child->refreshTransform(worldTransform);
}

void BaseNode::setupTransform(const TransformComponent& Model) {
          localTransform = Model;
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
