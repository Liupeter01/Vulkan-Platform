#pragma once
#ifndef _POINT_SPRITE_PARTICLE_SYSTEM_3D_HPP_
#define _POINT_SPRITE_PARTICLE_SYSTEM_3D_HPP_
#include <particle/PointSpriteParticleSystemBase.hpp>

namespace engine {

namespace particle3d {

          template <typename ComputePushConstantType = particle::ParticlePushConstant,
                    typename ComputeResourcesType = particle::ParticleComputeResourcesAOS,
                    typename GraphicPushConstantType = particle::ParticleDrawPushConstant,
                    typename GraphicResourcesType = particle::ParticleGraphicResourcesAOS>

class PointSpriteParticleSystemAOS3D
    : public particle::PointSpriteParticleSystemBaseAOS<ComputePushConstantType, ComputeResourcesType,
          GraphicPushConstantType, GraphicResourcesType> {

public:
  PointSpriteParticleSystemAOS3D(VkDevice device)
      : particle::PointSpriteParticleSystemBaseAOS<ComputePushConstantType, ComputeResourcesType,
            GraphicPushConstantType, GraphicResourcesType>(device) {

    this->set_vertex_shader(SLANG_SHADER_PATH "SpriteParticleDrawAOS3D.slang.spv",
                            "vertMain");
    this->set_fragment_shader(
        SLANG_SHADER_PATH "SpriteParticleDrawAOS3D.slang.spv", "fragMain");
  }
};

template <typename ComputePushConstantType = particle::ParticlePushConstant,
          typename ComputeResourcesType = particle::ParticleCompResourcesSOA,
          typename GraphicPushConstantType = particle::ParticleDrawPushConstant,
          typename GraphicResourcesType = particle::ParticleGraphicResourcesSOA>
class PointSpriteParticleSystemSOA3D
          : public particle::PointSpriteParticleSystemBaseSOA<ComputePushConstantType, ComputeResourcesType,
          GraphicPushConstantType, GraphicResourcesType> {

          using BaseType = particle::PointSpriteParticleSystemBaseSOA<ComputePushConstantType, ComputeResourcesType,
                    GraphicPushConstantType, GraphicResourcesType>;

public:
          PointSpriteParticleSystemSOA3D(VkDevice device)
                    : BaseType(device) {

                    this->set_vertex_shader(SLANG_SHADER_PATH "SpriteParticleDrawSOA3D.slang.spv",
                              "vertMain");
                    this->set_fragment_shader(
                              SLANG_SHADER_PATH "SpriteParticleDrawSOA3D.slang.spv", "fragMain");
          }
};
} // namespace particle3d
} // namespace engine

#endif //_POINT_SPRITE_PARTICLE_SYSTEM_3D_HPP_
