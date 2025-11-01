#include <glm/gtc/matrix_transform.hpp>
#include <nodes/mesh/MeshNode.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace engine {

          bool  RenderObject::isVisible(const glm::mat4& ProjView) {
                    const auto PVM = ProjView * this->transform;

                    glm::vec3 max{ std::numeric_limits<float>::infinity() };
                    glm::vec3 min{ -std::numeric_limits<float>::infinity() };

                    for (std::size_t i = 0; i < 8; ++i) {
                              glm::vec3 res = glm::vec3(PVM * glm::vec4(bounds.getCorner(static_cast<Bounds3::CornerType>(i)), 1.f));
                              min = glm::min(res, min);
                              max = glm::max(res, max);
                    }

                    if (min.z < 0.f || min.z > 1.f || min.x > 1.f || max.x < -1.f || min.y > 1.f || max.y < -1.f) {
                              return false;
                    }
                    return true;
          }

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
    object.bounds = surface.bounds;
    object.parent = mesh_.get();
    ctx.OpaqueSurfaces.push_back(std::move(object));
  }

  BaseNode::Draw(topMatrix, ctx);
}
} // namespace node
} // namespace engine
