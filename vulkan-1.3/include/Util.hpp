#pragma once
#ifndef _UTIL_HPP_
#define _UTIL_HPP_
#include <Tools.hpp>

namespace engine {
namespace util {
static inline void transition_image(VkCommandBuffer cmd, VkImage image,
                      VkImageLayout currentLayout, VkImageLayout newLayout) {
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

  imageBarrier.subresourceRange = tools::image_subresource_range(aspectMask);
  imageBarrier.image = image;

  VkDependencyInfo depInfo{};
  depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  depInfo.pNext = nullptr;

  depInfo.imageMemoryBarrierCount = 1;
  depInfo.pImageMemoryBarriers = &imageBarrier;

  vkCmdPipelineBarrier2(cmd, &depInfo);
}

static inline void copy_image_to_image(VkCommandBuffer cmd, 
          VkImage source, VkImage destination, 
          VkExtent2D srcSize, VkExtent2D dstSize) {

		  VkImageBlit2 blitRegion{};
		  blitRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;

		  blitRegion.srcOffsets[1].x = srcSize.width;
		  blitRegion.srcOffsets[1].y = srcSize.height;

		  blitRegion.dstOffsets[1].x = dstSize.width;
		  blitRegion.dstOffsets[1].y = dstSize.height;
		  blitRegion.dstOffsets[1].z = blitRegion.srcOffsets[1].z = 1;

		  blitRegion.srcSubresource.aspectMask = blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		  blitRegion.srcSubresource.baseArrayLayer = blitRegion.dstSubresource.baseArrayLayer = 0;
		  blitRegion.srcSubresource.layerCount = blitRegion.dstSubresource.layerCount = 1;
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
} // namespace util
} // namespace engine

#endif // _UTIL_HPP_
