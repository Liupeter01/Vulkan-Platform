#pragma once
#ifndef _GAME_OBJECT_HPP_
#define _GAME_OBJECT_HPP_
#include <Model.hpp>
#include <glm/glm.hpp>
#include <memory>

namespace engine {
struct TransformComponent {
  glm::vec3 translation{0.f};
  glm::vec3 scale{1.f};
  glm::vec3 rotation{};

  glm::mat4 mat4();
  glm::mat3 normalMatrix();
};

struct PointLightAttribute {
  float lightIntensity = 1.f;
};

class GameObject {
  GameObject(unsigned int id) : id_(id) {}

public:
  using id_t = unsigned int;
  GameObject(const GameObject &) = delete;
  GameObject &operator=(const GameObject &) = delete;
  GameObject(GameObject &&) = default;
  GameObject &operator=(GameObject &&) = default;

  unsigned int getId() const { return id_; }
  static GameObject createGameObject();
  static GameObject createPointLight(float intensity = 1.f, float radius = 1.f,
                                     glm::vec3 color = glm::vec3{1.f});

  glm::vec3 color;
  TransformComponent transform{};

  std::shared_ptr<Model> model_;
  std::unique_ptr<PointLightAttribute> pointLightAttribute_ = nullptr;

private:
  id_t id_;
};
} // namespace engine

#endif //_GAME_OBJECT_HPP_
