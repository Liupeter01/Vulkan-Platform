#include "GameObject.hpp"
#include <GameObject.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

glm::mat4 engine::TransformComponent::mat4() {
  const auto T = glm::translate(glm::one<glm::mat4>(), translation);
  const auto R = glm::eulerAngleYXZ(rotation.y, rotation.x, rotation.z);
  return T * R * glm::scale(glm::one<glm::mat4>(), scale);
}

glm::mat3 engine::TransformComponent::normalMatrix() {
  const auto inv_S = glm::inverse(glm::scale(glm::one<glm::mat4>(), scale));
  const auto R = glm::eulerAngleYXZ(rotation.y, rotation.x, rotation.z);
  return {R * inv_S};
}

engine::GameObject engine::GameObject::createGameObject() {
  static unsigned int curId = 0;
  return GameObject{curId++};
}

engine::GameObject engine::GameObject::createPointLight(float intensity,
                                                        float radius,
                                                        glm::vec3 color) {
  GameObject obj = createGameObject();
  obj.color = color;
  obj.transform.scale.x = radius;

  obj.pointLightAttribute_.reset();
  obj.pointLightAttribute_ = std::make_unique<PointLightAttribute>();
  obj.pointLightAttribute_->lightIntensity = intensity;
  return obj;
}
