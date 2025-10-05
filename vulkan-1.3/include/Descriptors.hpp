#pragma once
#ifndef _DESCRIPTOR_HPP_
#define _DESCRIPTOR_HPP_
#include <vector>
#include <vulkan/vulkan.hpp>

namespace engine {

          struct DescriptorLayoutBuilder {
                    DescriptorLayoutBuilder() = default;

                    void add_binding(uint32_t binding, VkDescriptorType type);
                    void clear();

                    [[nodiscard]]VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);

                    std::vector<VkDescriptorSetLayoutBinding> bindings;
          };

          struct DescriptorAllocator {
                    struct PoolSizeRatio {
                              VkDescriptorType type;
                              float ratio;
                    };

                    DescriptorAllocator() = default;
                    DescriptorAllocator(VkDevice& device, uint32_t maxSets, const std::vector<PoolSizeRatio>& poolRatios);
                    virtual ~DescriptorAllocator();

                    void init_pool(uint32_t maxSets, const std::vector<PoolSizeRatio>& poolRatios);

                    void reset_pool();
                    void destroy_pool();

                    [[nodiscard]]VkDescriptorSet allocate(VkDescriptorSetLayout layout);

                    VkDevice& device_;
                    VkDescriptorPool pool_;

          private:
                    bool isInit_ = false;
          };
}

#endif //_DESCRIPTOR_HPP_