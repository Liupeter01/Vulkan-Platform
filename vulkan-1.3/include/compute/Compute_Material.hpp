#pragma once
#ifndef _COMPUTE_MATERIAL_HPP_
#define _COMPUTE_MATERIAL_HPP_
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS // no degresss
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace engine {

struct ComputePipeline;

enum class ComputePass { UNDEFINED, DEFAULT, SPECIALCONSTANT };

struct ComputeInstance {
  ComputePipeline *pipeline;
  ComputePass pass = ComputePass::DEFAULT;
  VkDescriptorSet computeSet;
};
} // namespace engine

#endif //_COMPUTE_MATERIAL_HPP_
