#pragma once
#ifndef _GLTF_PBR_MATERIAL_
#define _GLTF_PBR_MATERIAL_
#include <GlobalDef.hpp>

#define GLM_FORCE_RADIANS // no degresss
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace engine {

struct MaterialPipeline;

enum class MaterialPass { UNDEFINED, OPAQUE, TRANSPARENT };

struct MaterialConstants {
  glm::vec4 colorFactors;
  glm::vec4 metal_rough_factors; // R & B
  glm::vec4 extra[14]; // padding, we need it anyway for uniform buffers
};

struct MaterialResources {
  VkImageView colorImage{};
  VkImageView metalRoughImage{};

  VkSampler colorSampler{};
  VkSampler metalRoughSampler{};

  VkBuffer materialConstantsData{};
  size_t materialConstantsOffset{};
};

struct MaterialInstance {
  MaterialPass passType;
  MaterialPipeline *pipeline;
  VkDescriptorSet materialSet;
};

} // namespace engine

#endif
