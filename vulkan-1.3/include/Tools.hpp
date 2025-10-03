#pragma once
#ifndef _TOOLS_HPP_
#define _TOOLS_HPP_
#include <vulkan/vulkan.hpp>

namespace engine {
namespace tools {
template <typename SizeT>
inline void hash_combine_impl(SizeT &seed, SizeT value) {
  seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

[[nodiscard]]
inline static VkCommandPoolCreateInfo command_pool_create_info(uint32_t queueFamilyIndex,
          VkCommandPoolCreateFlags flags = 0)
{
          VkCommandPoolCreateInfo info = {};
          info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
          info.pNext = nullptr;
          info.queueFamilyIndex = queueFamilyIndex;
          info.flags = flags;
          return info;
}

[[nodiscard]]
inline static VkCommandBufferAllocateInfo command_buffer_allocate_info(
          VkCommandPool pool, uint32_t count = 1)
{
          VkCommandBufferAllocateInfo info = {};
          info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
          info.pNext = nullptr;

          info.commandPool = pool;
          info.commandBufferCount = count;
          info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
          return info;
}

[[nodiscard]]
inline static VkFenceCreateInfo  fence_create_info(VkFenceCreateFlags flags = 0)
{
          VkFenceCreateInfo info = {};
          info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
          info.pNext = nullptr;

          info.flags = flags;

          return info;
}

[[nodiscard]]
inline static VkSemaphoreCreateInfo  semaphore_create_info(VkSemaphoreCreateFlags flags = 0)
{
          VkSemaphoreCreateInfo info = {};
          info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
          info.pNext = nullptr;
          info.flags = flags;
          return info;
}

[[nodiscard]]
inline static VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags /*= 0*/)
{
          VkCommandBufferBeginInfo info = {};
          info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
          info.pNext = nullptr;

          info.pInheritanceInfo = nullptr;
          info.flags = flags;
          return info;
}

[[nodiscard]]
inline static  VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspectMask)
{
          VkImageSubresourceRange subImage{};
          subImage.aspectMask = aspectMask;
          subImage.baseMipLevel = 0;
          subImage.levelCount = VK_REMAINING_MIP_LEVELS;
          subImage.baseArrayLayer = 0;
          subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

          return subImage;
}

} // namespace tools
} // namespace engine

#endif //_TOOLS_HPP_
