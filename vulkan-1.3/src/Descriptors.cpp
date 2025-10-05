#include <Tools.hpp>
#include <Descriptors.hpp>

namespace engine {
          void DescriptorLayoutBuilder::clear() { bindings.clear(); }

          void DescriptorLayoutBuilder::add_binding(uint32_t binding, VkDescriptorType type) {
                    bindings.emplace_back(tools::descriptor_set_layout_binding(binding, type));
          }

          VkDescriptorSetLayout 
                    DescriptorLayoutBuilder::build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext, VkDescriptorSetLayoutCreateFlags flags) {
                    for (auto& item : bindings) {
                              item.stageFlags |= shaderStages;
                    }
                    VkDescriptorSetLayoutCreateInfo info{}; 
                    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                    info.pNext = pNext;

                    info.pBindings = bindings.data();
                    info.bindingCount = (uint32_t)bindings.size();
                    info.flags = flags;

                    VkDescriptorSetLayout set;
                    vkCreateDescriptorSetLayout(device, &info, nullptr, &set);
                    return set;
          }

          DescriptorAllocator::DescriptorAllocator(VkDevice& device, uint32_t maxSets, const std::vector<PoolSizeRatio>& poolRatios) 
                    : device_(device)
          {
                    init_pool(maxSets, poolRatios);
          }

          DescriptorAllocator::~DescriptorAllocator() {
                    destroy_pool();
          }

          void  DescriptorAllocator::init_pool( uint32_t maxSets, const std::vector<PoolSizeRatio>& poolRatios) {
                    destroy_pool();

                    std::vector< VkDescriptorPoolSize> poolSizeArray;
                    for (const auto& ratio : poolRatios) {
                              VkDescriptorPoolSize info{};
                              info.type = ratio.type;
                              info.descriptorCount = static_cast<uint32_t>(maxSets * ratio.ratio);
                              poolSizeArray.push_back(std::move(info));
                    }

                    auto pool_info = tools::descriptor_pool_create_info(maxSets, poolSizeArray);
                    vkCreateDescriptorPool(device_, &pool_info, nullptr, &pool_);

                    isInit_ = true;
          }

          void DescriptorAllocator::reset_pool() {
                    if (isInit_) {
                              vkResetDescriptorPool(device_, pool_, 0);
                    }
          }

          void DescriptorAllocator::destroy_pool() {
                    if (isInit_) {
                              vkDestroyDescriptorPool(device_, pool_, nullptr);
                              isInit_ = false;
                    }
          }

          VkDescriptorSet DescriptorAllocator::allocate(VkDescriptorSetLayout layout) {
                    assert(isInit_);
                    VkDescriptorSetAllocateInfo allocInfo{};
                    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                    allocInfo.descriptorPool = pool_;
                    allocInfo.descriptorSetCount = 1;
                    allocInfo.pSetLayouts = &layout;

                    VkDescriptorSet descriptorSet{};
                    auto result = vkAllocateDescriptorSets(device_, &allocInfo, &descriptorSet);
                    if (result != VK_SUCCESS) {
                              throw std::runtime_error("Failed to allocate descriptor set!");
                    }
                    return descriptorSet;
          }
}
