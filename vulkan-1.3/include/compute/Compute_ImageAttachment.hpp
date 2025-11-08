#pragma once
#ifndef _COMPUTE_IMAGEATTACHMENT_HPP_
#define _COMPUTE_IMAGEATTACHMENT_HPP_
#include <Descriptors.hpp>
#include <compute/ComputePipeline.hpp>

namespace engine {
inline namespace compute {
class Compute_ImageAttachment {
public:
  Compute_ImageAttachment(VkDevice device)
      : device_(device), defaultComputePipeline_(device), writer_(device) {}
  virtual ~Compute_ImageAttachment() { destory(); }

  inline void init() {
    if (isinit_)
      return;

    DescriptorLayoutBuilder builder{device_};
    computeLayout_ = builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
                         .build(VK_SHADER_STAGE_COMPUTE_BIT); // add bindings

    std::vector<VkDescriptorSetLayout> layouts{computeLayout_};

    defaultComputePipeline_.create(ComputePass::DEFAULT, layouts);
    isinit_ = true;
  }

  inline void destory() {
    if (isinit_) {
      vkDestroyDescriptorSetLayout(device_, computeLayout_, nullptr);
      defaultComputePipeline_.destroy();
      isinit_ = false;
    }
  }

  [[nodiscard]]
  inline ComputeInstance
  generate_instance(ComputeResources &resources,
                    DescriptorPoolAllocator &globalDescriptorAllocator) {

    ComputeInstance ins{};
    ins.pipeline = &defaultComputePipeline_;
    ins.computeSet = globalDescriptorAllocator.allocate(computeLayout_);

    writer_.clear();
    writer_.write_image(0, resources.colorImageView, VK_NULL_HANDLE,
                        VK_IMAGE_LAYOUT_GENERAL,
                        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    writer_.update_set(ins.computeSet);
    return ins;
  }

private:
  bool isinit_ = false;
  DescriptorWriter writer_;
  VkDevice device_ = VK_NULL_HANDLE;
  ComputePipeline defaultComputePipeline_;

  VkDescriptorSetLayout computeLayout_ = VK_NULL_HANDLE;
};
} // namespace compute
} // namespace engine

#endif //_COMPUTE_IMAGEATTACHMENT_HPP_
