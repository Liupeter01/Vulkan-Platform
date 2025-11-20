#pragma once
#ifndef _POINT_SPRITE_PARTICLE_SYSTEM_2D_HPP_
#define _POINT_SPRITE_PARTICLE_SYSTEM_2D_HPP_
#include <particle/PointSpriteParticleSystemBase.hpp>

namespace engine {

class VulkanEngine;

namespace particle2d {

template <typename PushConstantType = particle::ParticlePushConstant,
          typename ResourcesType = particle::ParticleResources>
class PointSpriteParticleSystem2D
    : public particle::PointSpriteParticleSystemBase<PushConstantType,
                                                     ResourcesType> {

public:
  PointSpriteParticleSystem2D(VkDevice device)
      : particle::PointSpriteParticleSystemBase<PushConstantType,
                                                ResourcesType>(device) {

    this->set_vertex_shader(SLANG_SHADER_PATH "SpriteParticleDraw2D.slang.spv",
                            "vertMain");
    this->set_fragment_shader(
        SLANG_SHADER_PATH "SpriteParticleDraw2D.slang.spv", "fragMain");
  }
};
} // namespace particle2d
} // namespace engine

#endif //_POINT_SPRITE_PARTICLE_SYSTEM_2D_HPP_
