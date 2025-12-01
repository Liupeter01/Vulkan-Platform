#include <builder/BarrierBuilder.hpp>
#include <spdlog/fmt/fmt.h>

namespace engine {
ImageBarrierBuilder::ImageBarrierBuilder(VkImage image, VkFormat format)
    : image_(image), srcQueueFamilyIndex_(VK_QUEUE_FAMILY_IGNORED),
      dstQueueFamilyIndex_(VK_QUEUE_FAMILY_IGNORED) {
  range_ = {aspectMask_, 0, VK_REMAINING_MIP_LEVELS, 0,
            VK_REMAINING_ARRAY_LAYERS};

  switch (format) {
  case VK_FORMAT_D32_SFLOAT_S8_UINT:
  case VK_FORMAT_D24_UNORM_S8_UINT:
  case VK_FORMAT_D16_UNORM_S8_UINT:
    aspectMask_ = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    break;
  case VK_FORMAT_D32_SFLOAT:
  case VK_FORMAT_D16_UNORM:
    aspectMask_ = VK_IMAGE_ASPECT_DEPTH_BIT;
    break;
  default:
    aspectMask_ = VK_IMAGE_ASPECT_COLOR_BIT;
    break;
  }
}

ImageBarrierBuilder &ImageBarrierBuilder::from(VkImageLayout oldLayout) {
  oldLayout_ = oldLayout;
  return *this;
}

ImageBarrierBuilder &ImageBarrierBuilder::to(VkImageLayout newLayout) {
  newLayout_ = newLayout;
  return *this;
}

ImageBarrierBuilder &ImageBarrierBuilder::aspect(VkImageAspectFlags aspect) {
  aspectMask_ = aspect;
  range_.aspectMask = aspect;
  return *this;
}

ImageBarrierBuilder &ImageBarrierBuilder::withRange(uint32_t baseMip,
                                                    uint32_t mipCount,
                                                    uint32_t baseLayer,
                                                    uint32_t layerCount) {

  range_.aspectMask = aspectMask_;
  range_.baseMipLevel = baseMip;
  range_.levelCount = mipCount;
  range_.baseArrayLayer = baseLayer;
  range_.layerCount = layerCount;
  return *this;
}

ImageBarrierBuilder &
ImageBarrierBuilder::queueIndex(uint32_t srcQueueFamilyIndex,
                                uint32_t dstQueueFamilyIndex) {

  srcQueueFamilyIndex_ = srcQueueFamilyIndex;
  dstQueueFamilyIndex_ = dstQueueFamilyIndex;
  return *this;
}

VkImageMemoryBarrier2 ImageBarrierBuilder::build() {

  VkImageMemoryBarrier2 imageBarrier{};
  imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  imageBarrier.oldLayout = oldLayout_;
  imageBarrier.newLayout = newLayout_;
  imageBarrier.subresourceRange = range_;
  imageBarrier.image = image_;
  imageBarrier.srcQueueFamilyIndex = srcQueueFamilyIndex_;
  imageBarrier.dstQueueFamilyIndex = dstQueueFamilyIndex_;

  for (const auto &rule : rules_) {
    if (rule.oldLayout == oldLayout_ && rule.newLayout == newLayout_) {
      imageBarrier.srcStageMask = rule.srcStage;
      imageBarrier.dstStageMask = rule.dstStage;
      imageBarrier.srcAccessMask = rule.srcAccess;
      imageBarrier.dstAccessMask = rule.dstAccess;
      return imageBarrier;
    }
  }

  throw std::runtime_error(
      fmt::format("Unsupported layout transition: {} -> {}", int(oldLayout_),
                  int(newLayout_)));
}

void BarrierBuilder::addBarrier(const VkMemoryBarrier2 &b) {
  memBarriers_.push_back(b);
}
void BarrierBuilder::addBarrier(const VkBufferMemoryBarrier2 &b) {
  bufBarriers_.push_back(b);
}
void BarrierBuilder::addBarrier(const VkImageMemoryBarrier2 &b) {
  imgBarriers_.push_back(b);
}

VkDependencyInfo BarrierBuilder::build() {
  VkDependencyInfo info{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
  info.dependencyFlags = 0;
  info.memoryBarrierCount = static_cast<uint32_t>(memBarriers_.size());
  info.pMemoryBarriers = memBarriers_.data();
  info.bufferMemoryBarrierCount = static_cast<uint32_t>(bufBarriers_.size());
  info.pBufferMemoryBarriers = bufBarriers_.data();
  info.imageMemoryBarrierCount = static_cast<uint32_t>(imgBarriers_.size());
  info.pImageMemoryBarriers = imgBarriers_.data();
  return info;
}

void BarrierBuilder::clear() {
  memBarriers_.clear();
  bufBarriers_.clear();
  imgBarriers_.clear();
}

void BarrierBuilder::createBarrier(VkCommandBuffer cmd) {
  auto dependency = build();
  vkCmdPipelineBarrier2(cmd, &dependency);
  clear();
}
} // namespace engine
