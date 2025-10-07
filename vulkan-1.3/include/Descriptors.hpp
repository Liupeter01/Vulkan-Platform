#pragma once
#ifndef _DESCRIPTOR_HPP_
#define _DESCRIPTOR_HPP_
#include <vector>
#include <vulkan/vulkan.hpp>

namespace engine {

struct DescriptorLayoutBuilder {

  DescriptorLayoutBuilder(VkDevice device);
  DescriptorLayoutBuilder &add_binding(uint32_t binding, VkDescriptorType type);
  void clear();

  [[nodiscard]] VkDescriptorSetLayout
  build(VkShaderStageFlags shaderStages, void *pNext = nullptr,
        VkDescriptorSetLayoutCreateFlags flags = 0);

  VkDevice device_;
  std::vector<VkDescriptorSetLayoutBinding> bindings;
};

struct DescriptorAllocator {
  struct PoolSizeRatio {
    VkDescriptorType type;
    float ratio;
  };

  DescriptorAllocator(const DescriptorAllocator &) = delete;
  DescriptorAllocator &operator=(const DescriptorAllocator &) = delete;

  DescriptorAllocator(VkDevice device);

  virtual ~DescriptorAllocator();

  void init_pool(uint32_t maxSets,
                 const std::vector<PoolSizeRatio> &poolRatios);

  void reset_pool(VkDevice device);
  void destroy_pool();

  [[nodiscard]] VkDescriptorSet allocate(VkDescriptorSetLayout layout);

private:
  bool isInit_ = false;
  VkDevice device_ = VK_NULL_HANDLE;
  VkDescriptorPool pool_ = VK_NULL_HANDLE;
};
} // namespace engine

#endif //_DESCRIPTOR_HPP_
