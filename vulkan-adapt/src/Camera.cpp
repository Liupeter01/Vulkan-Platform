#include <Camera.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

void engine::Camera::setOrthoProjection(float left, float right, float top,
                                        float bottom, float near, float far) {
  projectionMatrix_ = glm::mat4{1.f};
  projectionMatrix_ = glm::orthoLH_ZO(left, right, bottom, top, near, far);
}

void engine::Camera::setPerspectiveProjection(float fovy, float aspect,
                                              float near, float far) {
  projectionMatrix_ = glm::mat4{0.f};
  projectionMatrix_ = glm::perspectiveLH_ZO(fovy, aspect, near, far);
}

void engine::Camera::setViewDirection(glm::vec3 position, glm::vec3 direction,
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

  inverseViewMatrix_ = glm::mat4{1.f};
  inverseViewMatrix_ = glm::inverse(viewMatrix_);
}

void engine::Camera::setViewTarget(glm::vec3 position, glm::vec3 target,
                                   glm::vec3 up) {
  viewMatrix_ = glm::mat4{1.f};
  viewMatrix_ = glm::lookAtLH(position, target, up);
  // setViewDirection(position, target - position, up);
  inverseViewMatrix_ = glm::mat4{1.f};
  inverseViewMatrix_ = glm::inverse(viewMatrix_);
}

void engine::Camera::setEularAngle(glm::vec3 position,
                                   glm::vec3 rotationRadians) {
  viewMatrix_ = glm::mat4{1.f};
  viewMatrix_ = glm::eulerAngleZYX(rotationRadians.z, rotationRadians.y,
                                   rotationRadians.x);
  viewMatrix_ = glm::transpose(viewMatrix_);
  viewMatrix_[3][0] = -glm::dot(glm::vec3(viewMatrix_[0]), position);
  viewMatrix_[3][1] = -glm::dot(glm::vec3(viewMatrix_[1]), position);
  viewMatrix_[3][2] = -glm::dot(glm::vec3(viewMatrix_[2]), position);

  inverseViewMatrix_ = glm::mat4{1.f};
  inverseViewMatrix_ = glm::inverse(viewMatrix_);
}

void engine::Camera::setYXZ(glm::vec3 position, glm::vec3 rotation) {
  setEularAngle(position, rotation);
}

const glm::mat4 &engine::Camera::getProjectionMatrix() const {
  return projectionMatrix_;
}

const glm::mat4 &engine::Camera::getViewMatrix() const { return viewMatrix_; }

const glm::mat4 &engine::Camera::getInverseViewMatrix() const {
  return inverseViewMatrix_;
}

glm::vec3 engine::Camera::getCameraPosition() const {
  return glm::vec3(inverseViewMatrix_[3]);
}
