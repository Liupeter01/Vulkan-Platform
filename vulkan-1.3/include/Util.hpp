#pragma once
#ifndef _UTIL_HPP_
#define _UTIL_HPP_
#include <builder/BarrierBuilder.hpp>
#include <fstream>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace engine {
namespace util {

static inline std::size_t generate_mipmap_levels(VkExtent2D imageSize) {
  return std::floor(std::log2(std::max(imageSize.width, imageSize.height))) + 1;
}

static inline void transition_image(VkCommandBuffer cmd, VkImage image,
                                    VkImageLayout currentLayout,
                                    VkImageLayout newLayout) {
  VkImageMemoryBarrier2 imageBarrier{};
  imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  imageBarrier.pNext = nullptr;

  imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
  imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;

  imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
  imageBarrier.dstAccessMask =
      VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

  imageBarrier.oldLayout = currentLayout;
  imageBarrier.newLayout = newLayout;

  VkImageAspectFlags aspectMask =
      (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
          ? VK_IMAGE_ASPECT_DEPTH_BIT
          : VK_IMAGE_ASPECT_COLOR_BIT;

  VkImageSubresourceRange subImage{};
  subImage.aspectMask = aspectMask;
  subImage.baseMipLevel = 0;
  subImage.levelCount = VK_REMAINING_MIP_LEVELS;
  subImage.baseArrayLayer = 0;
  subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

  imageBarrier.subresourceRange = subImage;
  imageBarrier.image = image;

  VkDependencyInfo depInfo{};
  depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  depInfo.pNext = nullptr;

  depInfo.imageMemoryBarrierCount = 1;
  depInfo.pImageMemoryBarriers = &imageBarrier;

  vkCmdPipelineBarrier2(cmd, &depInfo);
}

static inline void generate_mipmaps(VkCommandBuffer cmd, VkImage image,
                                    VkExtent2D imageSize) {

  const std::size_t mips = generate_mipmap_levels(imageSize);

  for (std::size_t perviousLevel = 0; perviousLevel < mips - 1;
       ++perviousLevel) {

    const std::size_t currentLevel = perviousLevel + 1;
    VkExtent2D currSize{std::max(imageSize.width >> 1, 1u),
                        std::max(imageSize.height >> 1, 1u)};

    auto imageBarrier = ImageBarrierBuilder{image}
                            .aspect(VK_IMAGE_ASPECT_COLOR_BIT)
                            .from(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
                            .to(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
                            .withRange(static_cast<uint32_t>(perviousLevel), 1)
                            .build();

    BarrierBuilder{}.add(imageBarrier).createBarrier(cmd);

    VkImageBlit2 blitRegion{};
    blitRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
    blitRegion.srcOffsets[1].x = static_cast<int32_t>(imageSize.width);
    blitRegion.srcOffsets[1].y = static_cast<int32_t>(imageSize.height);
    blitRegion.srcOffsets[1].z = 1;
    blitRegion.dstOffsets[1].x = static_cast<int32_t>(currSize.width);
    blitRegion.dstOffsets[1].y = static_cast<int32_t>(currSize.height);
    blitRegion.dstOffsets[1].z = 1;

    blitRegion.srcSubresource.aspectMask =
        blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.srcSubresource.layerCount =
        blitRegion.dstSubresource.layerCount = 1;
    blitRegion.srcSubresource.mipLevel = static_cast<uint32_t>(perviousLevel);
    blitRegion.dstSubresource.mipLevel = static_cast<uint32_t>(currentLevel);

    VkBlitImageInfo2 blitInfo{};
    blitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
    blitInfo.dstImage = blitInfo.srcImage = image;
    blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blitInfo.filter = VK_FILTER_LINEAR;
    blitInfo.regionCount = 1;
    blitInfo.pRegions = &blitRegion;
    vkCmdBlitImage2(cmd, &blitInfo);

    imageSize = currSize;
  }

  auto mipmapBarrier = ImageBarrierBuilder{image}
                           .aspect(VK_IMAGE_ASPECT_COLOR_BIT)
                           .from(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
                           .to(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                           .build();

  BarrierBuilder{}.add(mipmapBarrier).createBarrier(cmd);
}

static inline void copy_image_to_image(VkCommandBuffer cmd, VkImage source,
                                       VkImage destination, VkExtent2D srcSize,
                                       VkExtent2D dstSize) {

  VkImageBlit2 blitRegion{};
  blitRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;

  blitRegion.srcOffsets[1].x = srcSize.width;
  blitRegion.srcOffsets[1].y = srcSize.height;

  blitRegion.dstOffsets[1].x = dstSize.width;
  blitRegion.dstOffsets[1].y = dstSize.height;
  blitRegion.dstOffsets[1].z = blitRegion.srcOffsets[1].z = 1;

  blitRegion.srcSubresource.aspectMask = blitRegion.dstSubresource.aspectMask =
      VK_IMAGE_ASPECT_COLOR_BIT;
  blitRegion.srcSubresource.baseArrayLayer =
      blitRegion.dstSubresource.baseArrayLayer = 0;
  blitRegion.srcSubresource.layerCount = blitRegion.dstSubresource.layerCount =
      1;
  blitRegion.srcSubresource.mipLevel = blitRegion.dstSubresource.mipLevel = 0;

  VkBlitImageInfo2 blitInfo{};
  blitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
  blitInfo.dstImage = destination;
  blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  blitInfo.srcImage = source;
  blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  blitInfo.filter = VK_FILTER_LINEAR;
  blitInfo.regionCount = 1;
  blitInfo.pRegions = &blitRegion;

  vkCmdBlitImage2(cmd, &blitInfo);
}

[[nodiscard]]
static inline std::vector<uint32_t> read_spv(const std::string &path) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f)
    throw std::runtime_error("open failed: " + path);

  size_t sz = static_cast<size_t>(f.tellg());
  f.seekg(0);

  size_t wordCount = (sz + 3) / 4;
  std::vector<uint32_t> buf(wordCount, 0);

  f.read(reinterpret_cast<char *>(buf.data()), sz);
  if (!f)
    throw std::runtime_error("read failed: " + path);

  return buf;
}

static inline void load_shader(const std::string &shaderPath, VkDevice &device,
                               VkShaderModule *shaderModule) {

  auto vec = read_spv(shaderPath);

  if (!vec.size())
    throw std::runtime_error("Create Shader Failed!");

  VkShaderModuleCreateInfo shaderCreateInfo{};
  shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shaderCreateInfo.codeSize = vec.size() * sizeof(vec[0]);
  shaderCreateInfo.pCode = vec.data();

  if (vkCreateShaderModule(device, &shaderCreateInfo, nullptr, shaderModule))
    throw std::runtime_error("vkCreateShaderModule Failed!");
}

} // namespace util
} // namespace engine

#endif // _UTIL_HPP_
