#pragma once
#ifndef _GLTF_METALLIC_ROUGHNESS_
#define _GLTF_METALLIC_ROUGHNESS_
#include <Descriptors.hpp>
#include <material/GLTFPBR_Material.hpp>
#include <material/MaterialPipeline.hpp>

namespace engine {

inline namespace material {
class GLTFMetallic_Roughness {
public:
  GLTFMetallic_Roughness(VkDevice device);
  virtual ~GLTFMetallic_Roughness();

  void init(VkDescriptorSetLayout globalSceneLayout);
  void destory();

  [[nodiscard]] 
  MaterialInstance
  generate_instance(MaterialPass pass, MaterialResources &resources,
                    DescriptorPoolAllocator &globalDescriptorAllocator);

private:
  bool isinit_ = false;
  DescriptorWriter writer_;
  VkDevice device_ = VK_NULL_HANDLE;
  MaterialPipeline opaquePipeline;
  MaterialPipeline transparentPipeline;

  VkDescriptorSetLayout materialLayout_ = VK_NULL_HANDLE;
};
} // namespace material
} // namespace engine

#endif //_GLTF_METALLIC_ROUGHNESS_
