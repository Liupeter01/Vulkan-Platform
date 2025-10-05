#pragma once
#ifndef _DESCRIPTOR_HPP_
#define _DESCRIPTOR_HPP_
#include <vector>
#include <vulkan/vulkan.hpp>

namespace engine {

struct DescriptorLayoutBuilder {
  DescriptorLayoutBuilder() = default;

  DescriptorLayoutBuilder &add_binding(uint32_t binding, VkDescriptorType type);
  void clear();

  [[nodiscard]] VkDescriptorSetLayout
  build(VkDevice device, VkShaderStageFlags shaderStages, void *pNext = nullptr,
        VkDescriptorSetLayoutCreateFlags flags = 0);

  std::vector<VkDescriptorSetLayoutBinding> bindings;
};

struct DescriptorAllocator {
  struct PoolSizeRatio {
    VkDescriptorType type;
    float ratio;
  };

  DescriptorAllocator(const DescriptorAllocator &) = delete;
  DescriptorAllocator &operator=(const DescriptorAllocator &) = delete;

  DescriptorAllocator() = default;
  DescriptorAllocator(VkDevice device, uint32_t maxSets,
                      const std::vector<PoolSizeRatio> &poolRatios);

  DescriptorAllocator(DescriptorAllocator &&other) noexcept;
  DescriptorAllocator &operator=(DescriptorAllocator &&other) noexcept;

  virtual ~DescriptorAllocator();

  void init_pool(VkDevice device, uint32_t maxSets,
                 const std::vector<PoolSizeRatio> &poolRatios);

  void reset_pool();
  void destroy_pool();

  [[nodiscard]] VkDescriptorSet allocate(VkDescriptorSetLayout layout);

  VkDescriptorPool pool_ = VK_NULL_HANDLE;

private:
  VkDevice device_ = VK_NULL_HANDLE;
  bool isInit_ = false;
};
} // namespace engine

#endif //_DESCRIPTOR_HPP_
