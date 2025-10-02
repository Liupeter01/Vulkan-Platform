#pragma once
#ifndef _GLOBALDEF_HPP_
#define _GLOBALDEF_HPP_
#include <GameObject.hpp>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS // no degresss
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#define MAX_LIGHTS 10

namespace engine {

class GameObject;
class Camera;

struct PointLight {
  glm::vec4 position{};
  glm::vec4 color{};
};

struct PushConstantPointLight {
  glm::vec4 position{};
  glm::vec4 color{};
  float radius{};
};

struct PushConstantTut {
  glm::mat4 Model{1.f};
  glm::mat4 Normal_Matrix{1.f};
};

struct GlobalUBO {
  glm::mat4 P{};     // projection matrix
  glm::mat4 V{};     // View matrix
  glm::mat4 inv_V{}; // inverse of view matrix(actually its for camera
                     // position!)
  glm::vec4 ambientColor{1.f, 1.f, 1.f, .02f};
  PointLight pointLights[MAX_LIGHTS]{};
  alignas(16) int numLights{};
};

struct FrameConfigInfo {
  float timeFrame;
  VkCommandBuffer buffer;
  std::unordered_map<typename GameObject::id_t, GameObject> &gameObjs;
  VkDescriptorSet sets;
  const Camera &camera;
};
} // namespace engine

#endif //_GLOBAL_DEF_HPP_
