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

DescriptorPoolManager::DescriptorPoolManager(VkDevice device)
    : device_(device), isInit_(false) {}

DescriptorPoolManager::DescriptorPoolManager(
    DescriptorPoolManager &&other) noexcept
    : isInit_(other.isInit_), device_(other.device_), pool_(other.pool_) {
  other.isInit_ = false;
  other.device_ = VK_NULL_HANDLE;
  other.pool_ = VK_NULL_HANDLE;
}

DescriptorPoolManager &
DescriptorPoolManager::operator=(DescriptorPoolManager &&other) noexcept {
  if (this != &other) {
    destroy();

    isInit_ = other.isInit_;
    device_ = other.device_;
    pool_ = other.pool_;

    other.isInit_ = false;
    other.device_ = VK_NULL_HANDLE;
    other.pool_ = VK_NULL_HANDLE;
  }
  return *this;
}

DescriptorPoolManager DescriptorPoolManager::create(
    VkDevice device, const uint32_t set_count,
    const std::vector<VkDescriptorPoolSize> &poolSizes) {

  DescriptorPoolManager single_pool{device};
  single_pool.init(set_count, poolSizes);
  return single_pool;
}

DescriptorPoolManager::~DescriptorPoolManager() { destroy(); }

void DescriptorPoolManager::init(
    const uint32_t set_count,
    const std::vector<VkDescriptorPoolSize> &poolSizes) {

  auto pool_info = tools::descriptor_pool_create_info(set_count, poolSizes);
  VkResult result =
      vkCreateDescriptorPool(device_, &pool_info, nullptr, &pool_);
  if (result != VK_SUCCESS) {
    std::cerr << "Failed to create descriptor pool! VkResult = " << result
              << std::endl;
    throw std::runtime_error("vkCreateDescriptorPool failed.");
  }

  isInit_ = true;
}

void DescriptorPoolManager::reset() {
  if (isInit_) {
    vkResetDescriptorPool(device_, pool_, 0);
  }
}
void DescriptorPoolManager::destroy() {
  if (isInit_) {
    vkDestroyDescriptorPool(device_, pool_, nullptr);
    isInit_ = false;
  }
}

std::pair<VkDescriptorSet, VkResult>
DescriptorPoolManager::allocate(VkDescriptorSetLayout layout, void *pNext) {
  assert(isInit_);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.pNext = pNext;
  allocInfo.descriptorPool = pool_;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &layout;

  VkDescriptorSet descriptorSet{};
  auto result = vkAllocateDescriptorSets(device_, &allocInfo, &descriptorSet);
  return {descriptorSet, result};
}

DescriptorPoolAllocator::DescriptorPoolAllocator(VkDevice device)
    : device_{device} {}

DescriptorPoolAllocator::~DescriptorPoolAllocator() { destroy_pools(); }

void DescriptorPoolAllocator::init(
    const uint32_t setCount, const std::vector<PoolSizeRatio> &poolSizeRatios) {
  create_pool(setCount, poolSizeRatios);
}

void DescriptorPoolAllocator::create_pool(
    const uint32_t setCount, const std::vector<PoolSizeRatio> &poolSizeRatios) {

  init_poolSize(setCount, poolSizeRatios);

  readyPools_.push_back(
      DescriptorPoolManager::create(device_, setCount, poolSizes_));

  setCount_ = setCount << 1;
}

DescriptorPoolManager DescriptorPoolAllocator::get_pool() {
  if (readyPools_.empty()) {
    create_pool(setCount_, poolSizeRatio_);
    if (setCount_ > 4096) {
      setCount_ = 4096;
    }
  }
  DescriptorPoolManager ret = std::move(readyPools_.back());
  readyPools_.pop_back();
  return ret;
}

void DescriptorPoolAllocator::reset_pools() {
  readyPools_.reserve(readyPools_.size() + invalidPools_.size());
  for (auto &pool : readyPools_)
    pool.reset();
  for (auto &&pool : invalidPools_) {
    pool.reset();
    readyPools_.push_back(std::move(pool));
  }
  invalidPools_.clear();
}

void DescriptorPoolAllocator::destroy_pools() {
  for (auto &pool : readyPools_)
    pool.destroy();
  for (auto &pool : invalidPools_)
    pool.destroy();
  readyPools_.clear();
  invalidPools_.clear();
}

void DescriptorPoolAllocator::init_poolSize(
    const uint32_t setCount, const std::vector<PoolSizeRatio> &poolSizeRatio) {
  setCount_ = setCount;
  poolSizeRatio_ = poolSizeRatio;
  poolSizes_.clear();

  for (const auto &ratio : poolSizeRatio_) {
    VkDescriptorPoolSize info{};
    info.type = ratio.type;
    info.descriptorCount = static_cast<uint32_t>(setCount_ * ratio.ratio);
    poolSizes_.push_back(std::move(info));
  }
}

VkDescriptorSet DescriptorPoolAllocator::allocate(VkDescriptorSetLayout layout,
                                                  void *pNext) {
  auto poolManager = get_pool();
  auto [descriptorSet, result] = poolManager.allocate(layout, pNext);
  if (result == VK_SUCCESS) {
    readyPools_.push_back(std::move(poolManager));
    return descriptorSet;
  } else if (result == VK_ERROR_OUT_OF_POOL_MEMORY ||
             result == VK_ERROR_FRAGMENTED_POOL) {

    invalidPools_.push_back(std::move(poolManager));

    auto newPool = get_pool();
    auto [ret, res2] = newPool.allocate(layout, pNext);

    if (res2 != VK_SUCCESS)
      throw std::runtime_error("Descriptor allocation failed in new pool");

    readyPools_.push_back(std::move(newPool));
    return ret;
  } else {
    throw std::runtime_error("Unknown issue during descriptor allocation");
  }
}

DescriptorWriter::DescriptorWriter(VkDevice device) : device_(device) {}

void DescriptorWriter::clear() {

  imageInfos.clear();
  bufferInfos.clear();
  writes.clear();
}

void DescriptorWriter::write_buffer(int binding, VkBuffer buffer, size_t size,
                                    size_t offset, VkDescriptorType type) {

  auto &ref =
      bufferInfos.emplace_back(VkDescriptorBufferInfo{buffer, offset, size});

  VkWriteDescriptorSet descriptorSet{};
  descriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorSet.descriptorCount = 1;
  descriptorSet.descriptorType = type;
  descriptorSet.dstBinding = binding;
  descriptorSet.dstSet = VK_NULL_HANDLE; // setup set later
  descriptorSet.pBufferInfo = &ref;
  writes.push_back(std::move(descriptorSet));
}

void DescriptorWriter::write_image(int binding, VkImageView imageView,
                                   VkSampler sampler, VkImageLayout layout,
                                   VkDescriptorType type) {

  auto &ref = imageInfos.emplace_back(
      VkDescriptorImageInfo{sampler, imageView, layout});

  VkWriteDescriptorSet descriptorSet{};
  descriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorSet.descriptorCount = 1;
  descriptorSet.descriptorType = type;
  descriptorSet.dstBinding = binding;
  descriptorSet.dstSet = VK_NULL_HANDLE; // setup set later
  descriptorSet.pImageInfo = &ref;
  writes.push_back(std::move(descriptorSet));
}

void DescriptorWriter::update_set(VkDescriptorSet set) {
  for (VkWriteDescriptorSet &write : writes) {
    write.dstSet = set;
  }
  vkUpdateDescriptorSets(device_, static_cast<uint32_t>(writes.size()),
                         writes.data(), 0, nullptr);
}

} // namespace engine
