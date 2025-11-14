#pragma once
#ifndef _POINT_SPRITE_PARTICLE_SYSTEM_3D_HPP_
#define _POINT_SPRITE_PARTICLE_SYSTEM_3D_HPP_
#include <particle/PointSpriteParticleSystemBase.hpp>

namespace engine {

          namespace particle3d {

                    template <typename PushConstantType = particle::ParticlePushConstant,
                              typename ResourcesType = particle::ParticleResources>
                    class PointSpriteParticleSystem3D
                              : public particle::PointSpriteParticleSystemBase< PushConstantType, ResourcesType>{

                          public:
                            PointSpriteParticleSystem3D(VkDevice device)
                                : particle::PointSpriteParticleSystemBase< PushConstantType, ResourcesType>(device) {

                                    this->set_vertex_shader(SLANG_SHADER_PATH"SpriteParticleDraw3D.slang.spv", "vertMain");
                                    this->set_fragment_shader(SLANG_SHADER_PATH"SpriteParticleDraw3D.slang.spv", "fragMain");
                          }

                    };
          } // namespace particle3d
} // namespace engine

#endif //_POINT_SPRITE_PARTICLE_SYSTEM_3D_HPP_
