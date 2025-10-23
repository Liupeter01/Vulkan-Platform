#pragma once
#ifndef _CAMERA_HPP_
#define _CAMERA_HPP_
#include <nodes/BaseNode.hpp>

#define GLM_FORCE_RADIANS // no degresss
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace engine {
namespace node {
class CameraNode : public BaseNode {
public:
  CameraNode(std::shared_ptr<BaseNode> base = nullptr);

  void Draw(const glm::mat4 &parentMatrix, DrawContext &ctx) override;

  void setOrthoProjection(float left, float right, float top, float bottom,
                          float near, float far);
  void setPerspectiveProjection(float fovy, float aspect, float near,
                                float far);

  void setViewDirection(glm::vec3 position, glm::vec3 direction,
                        glm::vec3 up = glm::vec3{0.f, 1.f, 0.f});
  void setViewTarget(glm::vec3 position, glm::vec3 target,
                     glm::vec3 up = glm::vec3{0.f, 1.f, 0.f});

  /*For Interactive Camera operation!*/
  void setEularAngleYXZ(glm::vec3 translation, glm::vec3 rotation);
  void setYXZ(glm::vec3 translation, glm::vec3 rotation);

  glm::vec3 getEulerAnglesYXZFromView() const;

  const glm::mat4 &getProjectionMatrix() const;
  const glm::mat4 &getViewMatrix() const;
  const glm::mat4 &getInverseViewMatrix() const;
  glm::vec3 getCameraPosition() const;

private:
  glm::mat4 projectionMatrix_{1.f};
  glm::mat4 viewMatrix_{1.f};
  glm::mat4 inverseViewMatrix_{1.f};
};
} // namespace node
} // namespace engine

#endif //_CAMERA_HPP_
