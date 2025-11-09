#pragma once
#ifndef _COMPUTE_EFFECT_BASE_HPP_
#define _COMPUTE_EFFECT_BASE_HPP_
#include <type_traits>
#include <Descriptors.hpp>
#include <compute/ComputePipeline.hpp>

namespace engine {
          inline namespace compute {

                    template<typename PushConstantType, typename ResourcesType, typename T = void>
                    class Compute_EffectBase;

                    template<typename PushConstantType, typename ResourcesType, typename T>
                    class Compute_EffectBase : public std::false_type {
                    public:
                              static_assert(!std::is_same_v<T, T>,
                                        "Invalid Compute_EffectBase instantiation: PushConstantType must be trivially copyable and standard layout.");
                    };

                    template<typename ResourcesType>
                    class Compute_EffectBase< void, ResourcesType> : public std::true_type {
                    public:
                              Compute_EffectBase(VkDevice device)
                                        : device_(device),
                                        defaultComputePipeline_(device),
                                        specialConstantPipeline_(device),
                                        writer_(device) {
                              }
                              virtual ~Compute_EffectBase() { destory(); }

                              void destory() {
                                        if (isinit_) {
                                                  vkDestroyDescriptorSetLayout(device_, computeLayout_, nullptr);
                                                  defaultComputePipeline_.destroy();
                                                  isinit_ = false;
                                        }
                              }

                              void set_dispatch_size(uint32_t x, uint32_t y, uint32_t z) {
                                        dispatchGroups_ = { x,y,z };
                              }

                              virtual void init() = 0;
                              virtual ComputeInstance generate_instance(
                                        ResourcesType& resources,
                                        DescriptorPoolAllocator& globalDescriptorAllocator) = 0;
                              virtual void dispatch(VkCommandBuffer cmd, const ComputeInstance& ins) = 0;

                    protected:
                              bool isinit_ = false;
                              DescriptorWriter writer_;
                              VkDevice device_ = VK_NULL_HANDLE;
                              VkDescriptorSetLayout computeLayout_ = VK_NULL_HANDLE;

                              glm::uvec3 dispatchGroups_{ 16, 16, 1 };

                              ComputePipeline defaultComputePipeline_;
                              ComputePipeline specialConstantPipeline_;
                    };

                    template<typename PushConstantType, typename ResourcesType>
                    class Compute_EffectBase<PushConstantType,  ResourcesType,
                                                                      std::void_t<std::enable_if_t<std::is_trivially_copyable_v<PushConstantType> && 
                                                                                          std::is_standard_layout_v<PushConstantType>, void>>>
                              : public Compute_EffectBase<void, ResourcesType> {

                              using BaseType = Compute_EffectBase<void, ResourcesType>;

                    public:
                              Compute_EffectBase(VkDevice device)
                                        : BaseType(device) {
                                        pushConstantData_ = {};
                              }
                              virtual ~Compute_EffectBase() = default;

                    public:
                              PushConstantType& getPushConstantData() { return pushConstantData_; }
                              const PushConstantType& getPushConstantData() const { return pushConstantData_; }

                    protected:
                              PushConstantType pushConstantData_{};
                    };

          } // namespace compute
} // namespace engine

#endif //_COMPUTE_EFFECT_BASE_HPP_