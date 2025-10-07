#pragma once
#ifndef _GLOBALDEF_HPP_
#define _GLOBALDEF_HPP_
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS // no degresss
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace engine {

// Double Buffer
constexpr unsigned int FRAMES_IN_FLIGHT = 2;

struct FrameData {
  VkFence _renderFinishedFence;
  VkSemaphore _swapChainWait, _renderPresentKHRSignal;
  VkCommandPool _commandPool;
  VkCommandBuffer _mainCommandBuffer;
};

struct ComputeShaderPushConstants {
  glm::vec4 topLeft = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);     // Red
  glm::vec4 topRight = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);    // Yellow
  glm::vec4 bottomLeft = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);  // Blue
  glm::vec4 bottomRight = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f); // Cyan
};
} // namespace engine

#endif //_GLOBALDEF_HPP_
