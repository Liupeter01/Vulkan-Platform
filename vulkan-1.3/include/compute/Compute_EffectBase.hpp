#pragma once
#ifndef _COMPUTE_EFFECT_BASE_HPP_
#define _COMPUTE_EFFECT_BASE_HPP_
#include <Descriptors.hpp>
#include <compute/ComputePipeline.hpp>
#include <type_traits>

namespace engine {
inline namespace compute {

template <typename PushConstantType, typename ResourcesType, typename T = void>
class Compute_EffectBase;

template <typename PushConstantType, typename ResourcesType, typename T>
class Compute_EffectBase : public std::false_type {
public:
  static_assert(!std::is_same_v<T, T>,
                "Invalid Compute_EffectBase instantiation: PushConstantType "
                "must be trivially copyable and standard layout.");
};

template <typename ResourcesType>
class Compute_EffectBase<void, ResourcesType> : public std::true_type {
public:
  Compute_EffectBase(VkDevice device)
      : device_(device), defaultComputePipeline_(device),
        computeWriter_(device), specialConstantPipeline_(device) {}

  virtual ~Compute_EffectBase() {
    if (isComputeInit_) {
      vkDestroyDescriptorSetLayout(device_, computeLayout_, nullptr);
      defaultComputePipeline_.destroy();
      isComputeInit_ = false;
    }
  }

  void ensure_compute_initialized() {
    if (isComputeInit_)
      return;
    init_compute();
    isComputeInit_ = true;
  }

  void set_dispatch_size(uint32_t x, uint32_t y, uint32_t z) {
    dispatchGroups_ = {x, y, z};
  }

  virtual ComputeInstance generate_comp_instance(
      ResourcesType &resources,
      DescriptorPoolAllocator &globalDescriptorAllocator) = 0;

  virtual bool has_compute() const = 0;
  virtual void dispatch(VkCommandBuffer cmd, const ComputeInstance &ins) = 0;

protected:
  virtual void init_compute() = 0;

protected:
  bool isComputeInit_ = false;
  VkDevice device_ = VK_NULL_HANDLE;
  VkDescriptorSetLayout computeLayout_ = VK_NULL_HANDLE;

  DescriptorWriter computeWriter_;

  glm::uvec3 dispatchGroups_{16, 16, 1};

  ComputePipeline defaultComputePipeline_;
  ComputePipeline specialConstantPipeline_;
};

template <typename PushConstantType, typename ResourcesType>
class Compute_EffectBase<PushConstantType, ResourcesType,
                         std::void_t<std::enable_if_t<
                             std::is_trivially_copyable_v<PushConstantType> &&
                                 std::is_standard_layout_v<PushConstantType>,
                             void>>>
    : public Compute_EffectBase<void, ResourcesType> {

  using BaseType = Compute_EffectBase<void, ResourcesType>;

public:
  Compute_EffectBase(VkDevice device) : BaseType(device) {
    compPushConstantData_ = {};
  }
  virtual ~Compute_EffectBase() = default;

  PushConstantType &getCompPushConstantData() { return compPushConstantData_; }

protected:
  PushConstantType compPushConstantData_{};
};

} // namespace compute
} // namespace engine

#endif //_COMPUTE_EFFECT_BASE_HPP_
