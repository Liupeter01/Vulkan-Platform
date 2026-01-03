#pragma once
#ifndef _POINT_SPRITE_PARTICLE_SYSTEM_2D_HPP_
#define _POINT_SPRITE_PARTICLE_SYSTEM_2D_HPP_
#include <particle/PointSpriteParticleSystemBase.hpp>

namespace engine {

class VulkanEngine;

namespace particle2d {

template <typename ComputePushConstantType = particle::ParticlePushConstant,
          typename ComputeResourcesType = particle::ParticleComputeResourcesAOS,
          typename GraphicPushConstantType = particle::ParticleDrawPushConstant,
          typename GraphicResourcesType = particle::ParticleGraphicResourcesAOS>
class PointSpriteParticleSystemAOS2D
    : public particle::PointSpriteParticleSystemBaseAOS<
          ComputePushConstantType, ComputeResourcesType,
          GraphicPushConstantType, GraphicResourcesType> {

public:
  PointSpriteParticleSystemAOS2D(VkDevice device)
      : particle::PointSpriteParticleSystemBaseAOS<
            ComputePushConstantType, ComputeResourcesType,
            GraphicPushConstantType, GraphicResourcesType>(device) {

    this->set_vertex_shader(
        SLANG_SHADER_PATH "SpriteParticleDrawAOS2D.slang.spv", "vertMain");
    this->set_fragment_shader(
        SLANG_SHADER_PATH "SpriteParticleDrawAOS2D.slang.spv", "fragMain");
  }
};

template <typename ComputePushConstantType = particle::ParticlePushConstant,
          typename ComputeResourcesType = particle::ParticleCompResourcesSOA,
          typename GraphicPushConstantType = particle::ParticleDrawPushConstant,
          typename GraphicResourcesType = particle::ParticleGraphicResourcesSOA>
class PointSpriteParticleSystemSOA2D
    : public particle::PointSpriteParticleSystemBaseSOA<
          ComputePushConstantType, ComputeResourcesType,
          GraphicPushConstantType, GraphicResourcesType> {

  using BaseType = particle::PointSpriteParticleSystemBaseSOA<
      ComputePushConstantType, ComputeResourcesType, GraphicPushConstantType,
      GraphicResourcesType>;

public:
  PointSpriteParticleSystemSOA2D(VkDevice device) : BaseType(device) {

    this->set_vertex_shader(
        SLANG_SHADER_PATH "SpriteParticleDrawSOA2D.slang.spv", "vertMain");
    this->set_fragment_shader(
        SLANG_SHADER_PATH "SpriteParticleDrawSOA2D.slang.spv", "fragMain");
  }
};
} // namespace particle2d
} // namespace engine

#endif //_POINT_SPRITE_PARTICLE_SYSTEM_2D_HPP_
