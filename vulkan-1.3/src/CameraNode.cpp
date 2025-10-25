#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <nodes/CameraNode.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace engine {
namespace node {
CameraNode::CameraNode(std::shared_ptr<BaseNode> base) : BaseNode(base) {}

void CameraNode::Draw(const glm::mat4 &parentMatrix, DrawContext &ctx) {
  worldTransform = parentMatrix * localTransform.mat4();
  viewMatrix_ = glm::inverse(worldTransform);
}

void CameraNode::update() {
  worldTransform = localTransform.mat4();
  viewMatrix_ = glm::inverse(worldTransform);
}

void CameraNode::setOrthoProjection(float left, float right, float top,
                                    float bottom, float near, float far) {
  projectionMatrix_ = glm::mat4{1.f};
  projectionMatrix_ = glm::orthoLH_ZO(left, right, bottom, top, near, far);
}

void CameraNode::setPerspectiveProjection(float fovy, float aspect, float near,
                                          float far) {
  projectionMatrix_ = glm::mat4{0.f};
  projectionMatrix_ = glm::perspectiveLH_ZO(fovy, aspect, near, far);
}

void CameraNode::setViewDirection(glm::vec3 position, glm::vec3 direction,
                                  glm::vec3 up) {

  viewMatrix_ = glm::mat4{1.f};
  glm::vec3 cameraDirection = glm::normalize(direction);
  glm::vec3 cameraRight = glm::normalize(glm::cross(up, cameraDirection));
  glm::vec3 cameraUp = glm::cross(cameraDirection, cameraRight);

  viewMatrix_[0][0] = cameraRight.x;
  viewMatrix_[1][0] = cameraRight.y;
  viewMatrix_[2][0] = cameraRight.z;
  viewMatrix_[3][0] = -glm::dot(cameraRight, position);

  viewMatrix_[0][1] = cameraUp.x;
  viewMatrix_[1][1] = cameraUp.y;
  viewMatrix_[2][1] = cameraUp.z;
  viewMatrix_[3][1] = -glm::dot(cameraUp, position);

  viewMatrix_[0][2] = cameraDirection.x;
  viewMatrix_[1][2] = cameraDirection.y;
  viewMatrix_[2][2] = cameraDirection.z;
  viewMatrix_[3][2] = -glm::dot(cameraDirection, position);

  inverseViewMatrix_ = glm::inverse(viewMatrix_);
  localTransform.translation = getCameraPosition();
  localTransform.rotation = getEulerAnglesYXZFromView();
}

void CameraNode::setViewTarget(glm::vec3 position, glm::vec3 target,
                               glm::vec3 up) {

  viewMatrix_ = glm::mat4{1.f};
  viewMatrix_ = glm::lookAtLH(position, target, up);
  // setViewDirection(position, target - position, up);

  inverseViewMatrix_ = glm::inverse(viewMatrix_);
  localTransform.translation = getCameraPosition();
  localTransform.rotation = getEulerAnglesYXZFromView();
}

void CameraNode::setEularAngleYXZ(glm::vec3 translation,
                                  glm::vec3 rotationRadians) {

  viewMatrix_ = glm::mat4{1.f};
  viewMatrix_ = glm::eulerAngleYXZ(rotationRadians.y, rotationRadians.x,
                                   rotationRadians.z);
  viewMatrix_ = glm::transpose(viewMatrix_);
  viewMatrix_[3][0] = -glm::dot(glm::vec3(viewMatrix_[0]), translation);
  viewMatrix_[3][1] = -glm::dot(glm::vec3(viewMatrix_[1]), translation);
  viewMatrix_[3][2] = -glm::dot(glm::vec3(viewMatrix_[2]), translation);

  inverseViewMatrix_ = glm::inverse(viewMatrix_);
  localTransform.translation = translation;
  localTransform.rotation = rotationRadians;
}

void CameraNode::setYXZ(glm::vec3 translation, glm::vec3 rotation) {
  setEularAngleYXZ(translation, rotation);
}

glm::vec3 CameraNode::getEulerAnglesYXZFromView() const {
  glm::mat3 rot = glm::transpose(glm::mat3(viewMatrix_));
  float yaw, pitch, roll;
  glm::extractEulerAngleYXZ(glm::mat4(rot), yaw, pitch, roll);
  return glm::vec3(pitch, yaw, roll);
}

const glm::mat4 &CameraNode::getProjectionMatrix() const {
  return projectionMatrix_;
}

const glm::mat4 &CameraNode::getViewMatrix() const { return viewMatrix_; }

const glm::mat4 &CameraNode::getInverseViewMatrix() const {
  return inverseViewMatrix_;
}

glm::vec3 CameraNode::getCameraPosition() const {
  return glm::vec3(inverseViewMatrix_[3]);
}
} // namespace node
} // namespace engine
