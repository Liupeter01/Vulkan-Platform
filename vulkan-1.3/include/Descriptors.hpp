#pragma once
#ifndef _DESCRIPTOR_HPP_
#define _DESCRIPTOR_HPP_
#include <deque>
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

struct PoolSizeRatio {
  VkDescriptorType type;
  float ratio;
};

struct DescriptorPoolManager {
  DescriptorPoolManager(VkDevice device);

  DescriptorPoolManager(const DescriptorPoolManager &) = delete;
  DescriptorPoolManager &operator=(const DescriptorPoolManager &) = delete;

  DescriptorPoolManager(DescriptorPoolManager &&other) noexcept;
  DescriptorPoolManager &operator=(DescriptorPoolManager &&other) noexcept;

  static DescriptorPoolManager
  create(VkDevice device, const uint32_t set_count,
         const std::vector<VkDescriptorPoolSize> &poolRatios);

  virtual ~DescriptorPoolManager();
  void init(const uint32_t set_count,
            const std::vector<VkDescriptorPoolSize> &poolRatios);

  void reset();
  void destroy();

  [[nodiscard]] std::pair<VkDescriptorSet, VkResult>
  allocate(VkDescriptorSetLayout layout, void *pNext = VK_NULL_HANDLE);

private:
  bool isInit_ = false;
  VkDevice device_ = VK_NULL_HANDLE;
  VkDescriptorPool pool_ = VK_NULL_HANDLE;
};

struct DescriptorPoolAllocator {
  DescriptorPoolAllocator(const DescriptorPoolAllocator &) = delete;
  DescriptorPoolAllocator &operator=(const DescriptorPoolAllocator &) = delete;
  DescriptorPoolAllocator(VkDevice device);

  virtual ~DescriptorPoolAllocator();

  void init(const uint32_t setCount,
            const std::vector<PoolSizeRatio> &poolSizeRatio);

  void reset_pools();
  void destroy_pools();

  [[nodiscard]] VkDescriptorSet allocate(VkDescriptorSetLayout layout,
                                         void *pNext = VK_NULL_HANDLE);

protected:
  [[nodiscard]] DescriptorPoolManager get_pool();
  void create_pool(const uint32_t setCount,
                   const std::vector<PoolSizeRatio> &poolSizeRatio);
  void init_poolSize(const uint32_t setCount,
                     const std::vector<PoolSizeRatio> &poolSizeRatio);

private:
  bool isInit_ = false;
  VkDevice device_ = VK_NULL_HANDLE;
  uint32_t setCount_{};
  std::vector<PoolSizeRatio> poolSizeRatio_;
  std::vector<VkDescriptorPoolSize> poolSizes_;
  std::vector<DescriptorPoolManager> readyPools_;
  std::vector<DescriptorPoolManager> invalidPools_;
};

struct DescriptorWriter {
  DescriptorWriter(VkDevice device);
  void clear();
  void write_buffer(int binding, VkBuffer buffer, size_t size, size_t offset,
                    VkDescriptorType type);
  void write_image(int binding, VkImageView imageView, VkSampler sampler,
                   VkImageLayout layout, VkDescriptorType type);
  void update_set(VkDescriptorSet set);

  std::deque<VkDescriptorImageInfo> imageInfos;
  std::deque<VkDescriptorBufferInfo> bufferInfos;
  std::vector<VkWriteDescriptorSet> writes;

private:
  VkDevice device_;
};

} // namespace engine

#endif //_DESCRIPTOR_HPP_
