#pragma once
#ifndef _TOOLS_HPP_
#define _TOOLS_HPP_
#include <Util.hpp>
#include <optional>
#include <vector>

namespace engine {
namespace tools {
template <typename SizeT>
inline void hash_combine_impl(SizeT &seed, SizeT value) {
  seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

[[nodiscard]]
inline static VkCommandPoolCreateInfo
command_pool_create_info(uint32_t queueFamilyIndex,
                         VkCommandPoolCreateFlags flags = 0) {
  VkCommandPoolCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  info.pNext = nullptr;
  info.queueFamilyIndex = queueFamilyIndex;
  info.flags = flags;
  return info;
}

[[nodiscard]]
inline static VkCommandBufferAllocateInfo
command_buffer_allocate_info(VkCommandPool pool, uint32_t count = 1) {
  VkCommandBufferAllocateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  info.pNext = nullptr;

  info.commandPool = pool;
  info.commandBufferCount = count;
  info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  return info;
}

[[nodiscard]]
inline static VkFenceCreateInfo
fence_create_info(VkFenceCreateFlags flags = 0) {
  VkFenceCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  info.pNext = nullptr;

  info.flags = flags;

  return info;
}

[[nodiscard]]
inline static VkSemaphoreCreateInfo
semaphore_create_info(VkSemaphoreCreateFlags flags = 0) {
  VkSemaphoreCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  info.pNext = nullptr;
  info.flags = flags;
  return info;
}

[[nodiscard]]
inline static VkCommandBufferBeginInfo
command_buffer_begin_info(VkCommandBufferUsageFlags flags /*= 0*/) {
  VkCommandBufferBeginInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  info.pNext = nullptr;

  info.pInheritanceInfo = nullptr;
  info.flags = flags;
  return info;
}

[[nodiscard]]
inline static VkImageSubresourceRange
image_subresource_range(VkImageAspectFlags aspectMask) {
  VkImageSubresourceRange subImage{};
  subImage.aspectMask = aspectMask;
  subImage.baseMipLevel = 0;
  subImage.levelCount = VK_REMAINING_MIP_LEVELS;
  subImage.baseArrayLayer = 0;
  subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

  return subImage;
}

[[nodiscard]]
inline static VkSemaphoreSubmitInfo
semaphore_submit_info(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore) {
  VkSemaphoreSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  submitInfo.pNext = nullptr;
  submitInfo.semaphore = semaphore;
  submitInfo.stageMask = stageMask;
  submitInfo.deviceIndex = 0;
  submitInfo.value = 1;

  return submitInfo;
}

[[nodiscard]]
inline static VkCommandBufferSubmitInfo
command_buffer_submit_info(VkCommandBuffer cmd) {
  VkCommandBufferSubmitInfo info{};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  info.pNext = nullptr;
  info.commandBuffer = cmd;
  info.deviceMask = 0;

  return info;
}

[[nodiscard]]
inline static VkSubmitInfo2
submit_info(VkCommandBufferSubmitInfo *cmd,
            VkSemaphoreSubmitInfo *signalSemaphoreInfo,
            VkSemaphoreSubmitInfo *waitSemaphoreInfo) {
  VkSubmitInfo2 info = {};
  info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  info.pNext = nullptr;

  info.waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0 : 1;
  info.pWaitSemaphoreInfos = waitSemaphoreInfo;

  info.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
  info.pSignalSemaphoreInfos = signalSemaphoreInfo;

  info.commandBufferInfoCount = 1;
  info.pCommandBufferInfos = cmd;

  return info;
}

[[nodiscard]]
inline static VkImageCreateInfo image_create_info(VkFormat format,
                                                  VkImageUsageFlags usageFlags,
                                                  VkExtent3D extent) {
  VkImageCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  info.pNext = nullptr;

  info.imageType = VK_IMAGE_TYPE_2D;

  info.format = format;
  info.extent = extent;

  info.mipLevels = 1;
  info.arrayLayers = 1;

  // for MSAA. we will not be using it by default, so default it to 1 sample per
  // pixel.
  info.samples = VK_SAMPLE_COUNT_1_BIT;

  // optimal tiling, which means the image is stored on the best gpu format
  info.tiling = VK_IMAGE_TILING_OPTIMAL;
  info.usage = usageFlags;

  return info;
}

[[nodiscard]]
inline static VkImageViewCreateInfo
imageview_create_info(VkFormat format, VkImage image,
                      VkImageAspectFlags aspectFlags) {
  // build a image-view for the depth image to use for rendering
  VkImageViewCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  info.pNext = nullptr;

  info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  info.image = image;
  info.format = format;
  info.subresourceRange.baseMipLevel = 0;
  info.subresourceRange.levelCount = 1;
  info.subresourceRange.baseArrayLayer = 0;
  info.subresourceRange.layerCount = 1;
  info.subresourceRange.aspectMask = aspectFlags;

  return info;
}

[[nodiscard]]
inline static VkShaderModuleCreateInfo
shader_module_create_info(const std::vector<uint32_t> &vec) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = vec.size() * sizeof(vec[0]);
  createInfo.pCode = vec.data();
  return createInfo;
}

[[nodiscard]]
inline static VkDescriptorSetLayoutBinding
descriptor_set_layout_binding(uint32_t binding, VkDescriptorType type) {
  VkDescriptorSetLayoutBinding newbind{};
  newbind.binding = binding;
  newbind.descriptorCount = 1;
  newbind.descriptorType = type;
  return newbind;
}

[[nodiscard]]
inline static VkDescriptorPoolCreateInfo descriptor_pool_create_info(
    const uint32_t maxSets,
    const std::vector<VkDescriptorPoolSize> &poolSizes) {
  VkDescriptorPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = 0;
  pool_info.maxSets = maxSets;
  pool_info.poolSizeCount = (uint32_t)poolSizes.size();
  pool_info.pPoolSizes = poolSizes.data();
  return pool_info;
}

[[nodiscard]]
inline static VkRenderingAttachmentInfo color_attachment_info(
    VkImageView view, std::optional<VkClearValue> clear = std::nullopt,
    VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
  VkRenderingAttachmentInfo colorAttachment{};
  colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

  colorAttachment.imageView = view;
  colorAttachment.imageLayout = layout;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  if (!clear.has_value()) {
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    return colorAttachment;
  }

  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.clearValue = *clear;
  return colorAttachment;
}

[[nodiscard]]
inline static VkRenderingAttachmentInfo depth_attachment_info(
    VkImageView view,
    VkImageLayout layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {

  VkRenderingAttachmentInfo depthAttachment{};
  depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  depthAttachment.imageView = view;
  depthAttachment.imageLayout = layout;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  depthAttachment.clearValue.depthStencil.depth = 0.f;
  return depthAttachment;
}

[[nodiscard]]
inline static VkRenderingInfo
rendering_info(VkExtent2D rect, VkRenderingAttachmentInfo *pColorAttachments,
               VkRenderingAttachmentInfo *pDepthAttachment = nullptr,
               VkRenderingAttachmentInfo *pStencilAttachment = nullptr) {

  VkRenderingInfo renderingInfo{};
  renderingInfo.layerCount = 1;
  renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderingInfo.renderArea.extent = rect;
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.pColorAttachments = pColorAttachments;
  renderingInfo.pDepthAttachment = pDepthAttachment;
  renderingInfo.pStencilAttachment = pStencilAttachment;
  return renderingInfo;
}

[[nodiscard]]
inline static VkPipelineShaderStageCreateInfo
shader_stage_create_info(VkDevice &device, const std::string &shaderPath,
                         VkShaderStageFlagBits stage) {

  VkShaderModule shaderModule;

  util::load_shader(shaderPath, device, &shaderModule);

  VkPipelineShaderStageCreateInfo shaderStage{};
  shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStage.stage = stage;
  shaderStage.module = shaderModule;
  shaderStage.pName = "main";
  shaderStage.flags = 0;
  shaderStage.pSpecializationInfo = nullptr;
  return shaderStage;
}

static constexpr uint32_t bytes_per_pixel(VkFormat format) {
  switch (format) {
  case VK_FORMAT_R8_UNORM:
  case VK_FORMAT_R8_SRGB:
    return 1;

  case VK_FORMAT_R8G8_UNORM:
  case VK_FORMAT_R8G8_SRGB:
    return 2;

  case VK_FORMAT_R8G8B8_UNORM:
  case VK_FORMAT_R8G8B8_SRGB:
    return 3;

  case VK_FORMAT_R8G8B8A8_UNORM:
  case VK_FORMAT_R8G8B8A8_SRGB:
    return 4;

  case VK_FORMAT_R16G16B16A16_SFLOAT:
    return 8;

  case VK_FORMAT_R32G32B32A32_SFLOAT:
    return 16;

  default:
    throw std::invalid_argument(
        "Unsupported VkFormat in constexpr bytes_per_pixel()");
  }
}

template <typename T> inline static constexpr VkIndexType getIndexType() {
          using Decayed = std::decay_t<T>;
          if constexpr (std::is_same_v<Decayed, uint32_t>) {
                    return VK_INDEX_TYPE_UINT32;
          }
          else if constexpr (std::is_same_v<Decayed, uint16_t>) {
                    return VK_INDEX_TYPE_UINT16;
          }
          else if constexpr (std::is_same_v<Decayed, uint8_t>) {
                    return VK_INDEX_TYPE_UINT8;
          }
          else {
                    static_assert(false, "Unsupported index type");
          }
}

} // namespace tools
} // namespace engine

#endif //_TOOLS_HPP_
