#include <glm/gtc/matrix_transform.hpp>
#include <nodes/mesh/MeshNode.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace engine {

void TransformComponent::setRotation(const glm::vec3 &euler) {
  rotation = euler;
  quat = glm::quat(euler);
  update();
}

void TransformComponent::setQuaternion(const glm::quat &q) {
  quat = q;
  rotation = glm::eulerAngles(q);
  update();
}

glm::mat4 TransformComponent::mat4(RotationControll ctrl) const noexcept {

  if (!dirty)
    return localMatrix;

  const auto T = glm::translate(glm::one<glm::mat4>(), translation);
  const auto S = glm::scale(glm::one<glm::mat4>(), scale);
  glm::mat4 R{1.f};

  if (ctrl == RotationControll::Quat) {
    // quat
    R = glm::toMat4(quat);
  } else {
    // euler
    R = glm::eulerAngleYXZ(rotation.y, rotation.x, rotation.z);
  }

  localMatrix = T * R * S;
  dirty = false;
  return localMatrix;
}

void TransformComponent::update() { dirty = true; }

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

void BaseNode::setupTransform(const TransformComponent &Model) {
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
