#pragma once
#ifndef _GLOBALDEF_HPP_
#define _GLOBALDEF_HPP_
#include <vulkan/vulkan.hpp>

namespace engine {

// Double Buffer
constexpr unsigned int FRAMES_IN_FLIGHT = 2;

struct FrameData {
  VkFence _renderFinishedFence;
  VkSemaphore _swapChainWait, _renderPresentKHRSignal;
  VkCommandPool _commandPool;
  VkCommandBuffer _mainCommandBuffer;
};
} // namespace engine

#endif //_GLOBALDEF_HPP_
