#pragma once
#ifndef _GLTF_METALLIC_ROUGHNESS_
#define _GLTF_METALLIC_ROUGHNESS_
#include <Descriptors.hpp>
#include <pipeline/MaterialPipeline.hpp>
#include <material/GLTFPBR_Material.hpp>

namespace engine {

          struct MaterialInstance {
                    MaterialPass passType;
                    MaterialPipeline* pipeline;
                    VkDescriptorSet materialSet;
          };

          inline namespace material {
                    class GLTFMetallic_Roughness {
                    public:
                              GLTFMetallic_Roughness(VkDevice device);
                              virtual ~GLTFMetallic_Roughness();

                              void init(VkDescriptorSetLayout globalSceneLayout);
                              void destory();

                              MaterialInstance generate_instance(MaterialPass pass,
                                        const MaterialResources& resources,
                                        DescriptorPoolAllocator& globalDescriptorAllocator);

                    private:
                              bool isinit_ = false;
                              DescriptorWriter writer_;
                              VkDevice device_ = VK_NULL_HANDLE;
                              MaterialPipeline opaquePipeline;
                              MaterialPipeline transparentPipeline;

                              VkDescriptorSetLayout materialLayout_ = VK_NULL_HANDLE;
                    };
          }
}

#endif //_GLTF_METALLIC_ROUGHNESS_