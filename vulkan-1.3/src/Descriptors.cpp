#include <Descriptors.hpp>
#include <Tools.hpp>
#include <iostream>

namespace engine {

DescriptorLayoutBuilder::DescriptorLayoutBuilder(VkDevice device)
    : device_(device) {}

void DescriptorLayoutBuilder::clear() { bindings.clear(); }

DescriptorLayoutBuilder &
DescriptorLayoutBuilder::add_binding(uint32_t binding, VkDescriptorType type) {
  bindings.emplace_back(tools::descriptor_set_layout_binding(binding, type));
  return *this;
}

VkDescriptorSetLayout
DescriptorLayoutBuilder::build(VkShaderStageFlags shaderStages, void *pNext,
                               VkDescriptorSetLayoutCreateFlags flags) {
  for (auto &item : bindings) {
    item.stageFlags |= shaderStages;
  }
  VkDescriptorSetLayoutCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  info.pNext = pNext;

  info.pBindings = bindings.data();
  info.bindingCount = (uint32_t)bindings.size();
  info.flags = flags;

  VkDescriptorSetLayout set{};
  VkResult result = vkCreateDescriptorSetLayout(device_, &info, nullptr, &set);
  if (result != VK_SUCCESS) {
    std::cerr << "Failed to create DescriptorSetLayout! VkResult = " << result
              << std::endl;
    throw std::runtime_error("vkCreateDescriptorSetLayout failed.");
  }

  return set;
}

DescriptorAllocator::DescriptorAllocator(VkDevice device)
    : device_(device), isInit_(false) {}

DescriptorAllocator::~DescriptorAllocator() { destroy_pool(); }

void DescriptorAllocator::init_pool(
    uint32_t maxSets, const std::vector<PoolSizeRatio> &poolRatios) {

  std::vector<VkDescriptorPoolSize> poolSizeArray;
  for (const auto &ratio : poolRatios) {
    VkDescriptorPoolSize info{};
    info.type = ratio.type;
    info.descriptorCount = static_cast<uint32_t>(maxSets * ratio.ratio);
    poolSizeArray.push_back(std::move(info));
  }

  auto pool_info = tools::descriptor_pool_create_info(maxSets, poolSizeArray);

  VkResult result =
      vkCreateDescriptorPool(device_, &pool_info, nullptr, &pool_);
  if (result != VK_SUCCESS) {
    std::cerr << "Failed to create descriptor pool! VkResult = " << result
              << std::endl;
    throw std::runtime_error("vkCreateDescriptorPool failed.");
  }

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
} // namespace engine
