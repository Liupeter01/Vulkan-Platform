#pragma once
#ifndef _COMPUTE_MATERIAL_HPP_
#define _COMPUTE_MATERIAL_HPP_
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS // no degresss
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace engine {

struct ComputePipeline;

struct ComputeShaderPushConstants {
  glm::vec4 topLeft = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);     // Red
  glm::vec4 topRight = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);    // Yellow
  glm::vec4 bottomLeft = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);  // Blue
  glm::vec4 bottomRight = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f); // Cyan
};

struct ComputeResources {
  VkImageView colorImageView{};
};

struct ComputeInstance {
  ComputePipeline *pipeline;
  VkDescriptorSet computeSet;
};
} // namespace engine

#endif //_COMPUTE_MATERIAL_HPP_
