#include <material/GLTFMetallic_Roughness.hpp>
#include <mesh/MeshBuffers.hpp>

namespace engine {

namespace material {
GLTFMetallic_Roughness::GLTFMetallic_Roughness(VkDevice device)
    : device_(device), opaquePipeline{device}, transparentPipeline{device},
      writer_(device) {}

GLTFMetallic_Roughness::~GLTFMetallic_Roughness() { destory(); }

void GLTFMetallic_Roughness::init(VkDescriptorSetLayout globalSceneLayout) {
  if (isinit_)
    return;

  if (!globalSceneLayout)
    throw std::runtime_error("Invalid Global Scene Layout!");

  materialLayout_ =
      DescriptorLayoutBuilder(device_)
          .add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
          .add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
          .add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
          .build(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

  std::vector<VkDescriptorSetLayout> layouts{globalSceneLayout,
                                             materialLayout_};

  opaquePipeline.create(MaterialPass::OPAQUE, layouts);
  transparentPipeline.create(MaterialPass::TRANSPARENT, layouts);
  isinit_ = true;
}

void GLTFMetallic_Roughness::destory() {
  if (isinit_) {
    vkDestroyDescriptorSetLayout(device_, materialLayout_, nullptr);
    opaquePipeline.destroy();
    transparentPipeline.destroy();
    isinit_ = false;
  }
}

MaterialInstance GLTFMetallic_Roughness::generate_instance(
    MaterialPass pass, const MaterialResources &resources,
    DescriptorPoolAllocator &globalDescriptorAllocator) {

  if (pass == MaterialPass::UNDEFINED)
    throw std::runtime_error("Undefined MaterialPass!");

  MaterialInstance ins{};
  ins.passType = pass;

  if (ins.passType == MaterialPass::TRANSPARENT) {
    ins.pipeline = &transparentPipeline;
  } else if (ins.passType == MaterialPass::OPAQUE) {
    ins.pipeline = &opaquePipeline;
  }

  ins.materialSet = globalDescriptorAllocator.allocate(materialLayout_);

  writer_.clear();
  writer_.write_buffer(
      0, resources.materialConstantsData, sizeof(MaterialConstants),
      resources.materialConstantsOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

  writer_.write_image(1, resources.colorImage, resources.colorSampler,
                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

  writer_.write_image(2, resources.metalRoughImage, resources.metalRoughSampler,
                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

  writer_.update_set(ins.materialSet);
  return ins;
}
} // namespace material
} // namespace engine
