#pragma once
#ifndef _GRAPHIC_EFFECT_BASE_HPP_
#define _GRAPHIC_EFFECT_BASE_HPP_
#include <Descriptors.hpp>
#include <material/MaterialPipeline.hpp>
#include <type_traits>

namespace engine {

inline namespace material {
template <typename PushConstantType, typename ResourcesType, typename T = void>
class Graphic_EffectBase;

template <typename PushConstantType, typename ResourcesType, typename T>
class Graphic_EffectBase : public std::false_type {
public:
  static_assert(!std::is_same_v<T, T>,
                "Invalid Graphic_EffectBase instantiation: PushConstantType "
                "must be trivially copyable and standard layout.");
};

template <typename ResourcesType>
class Graphic_EffectBase<void, ResourcesType> : public std::true_type {
public:
  Graphic_EffectBase(VkDevice device)
      : device_(device), defaultOpaquePipeline_(device), graphicWriter_(device),
        defaultOpaquePointPipeline_(device), transparentPipeline_(device) {}

  virtual ~Graphic_EffectBase() {
    if (isGraphicInit_) {
      vkDestroyDescriptorSetLayout(device_, graphicLayout_, nullptr);
      defaultOpaquePipeline_.destroy();
      defaultOpaquePointPipeline_.destroy();
      transparentPipeline_.destroy();
      isGraphicInit_ = false;
    }
  }

  virtual MaterialInstance generate_graphic_instance(
      ResourcesType &resources,
      DescriptorPoolAllocator &globalDescriptorAllocator) = 0;

  virtual bool has_graphic() const = 0;
  virtual void render(VkCommandBuffer cmd, const MaterialInstance &ins) = 0;

  void ensure_graphic_initialized() {
    if (isGraphicInit_)
      return;
    init_graphic();
    isGraphicInit_ = true;
  }

protected:
  virtual void init_graphic() = 0;

protected:
  bool isGraphicInit_ = false;
  VkDevice device_ = VK_NULL_HANDLE;
  VkDescriptorSetLayout graphicLayout_ = VK_NULL_HANDLE;

  DescriptorWriter graphicWriter_;

  MaterialPipeline defaultOpaquePipeline_;
  MaterialPipeline defaultOpaquePointPipeline_;
  MaterialPipeline transparentPipeline_;
};

template <typename PushConstantType, typename ResourcesType>
class Graphic_EffectBase<PushConstantType, ResourcesType,
                         std::void_t<std::enable_if_t<
                             std::is_trivially_copyable_v<PushConstantType> &&
                                 std::is_standard_layout_v<PushConstantType>,
                             void>>>
    : public Graphic_EffectBase<void, ResourcesType> {

  using BaseType = Graphic_EffectBase<void, ResourcesType>;

public:
  Graphic_EffectBase(VkDevice device) : BaseType(device) {
    graphicPushConstantData_ = {};
  }
  virtual ~Graphic_EffectBase() = default;

  PushConstantType &getGraphicPushConstantData() {
    return graphicPushConstantData_;
  }

protected:
  PushConstantType graphicPushConstantData_{};
};
} // namespace material
} // namespace engine

#endif
