#pragma once
#ifndef _COMPUTE_PIPELINE_HPP_
#define _COMPUTE_PIPELINE_HPP_
#include <Descriptors.hpp>
#include <GlobalDef.hpp>
#include <string>

#define GLM_FORCE_RADIANS // no degresss
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace engine {

struct ComputeShaderPushConstants {
  glm::vec4 topLeft = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);     // Red
  glm::vec4 topRight = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);    // Yellow
  glm::vec4 bottomLeft = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);  // Blue
  glm::vec4 bottomRight = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f); // Cyan
};

inline namespace compute {

struct ComputePipelinePacked {

  ComputePipelinePacked(VkDevice device);
  virtual ~ComputePipelinePacked();
  void init(VkImageView imageView);
  void destroy();
  void draw_compute(VkCommandBuffer &cmd, VkExtent2D drawExtent);

  std::string name = " ComputePipelinePacked";
  ComputeShaderPushConstants data{};

private:
  DescriptorLayoutBuilder builder;
  const std::vector<DescriptorAllocator::PoolSizeRatio> sizes{
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}};

  // Initializing the layout and descriptors; store image, or vertex indicies
  DescriptorAllocator descriptorAllocator_;
  VkDescriptorSet drawCompDescriptor_;
  VkDescriptorSetLayout drawCompDescriptorLayout_;

  VkPipeline gradientComputePipeline_;
  VkPipelineLayout gradientComputePipelineLayout_;

protected:
  void init_descriptors(VkImageView imageView);
  void init_pipeline();
  void destroy_descriptors();
  void destroy_pipeline();

private:
  bool isInit_ = false;
  VkDevice device_;
};
} // namespace compute
} // namespace engine

#endif //_COMPUTE_PIPELINE_HPP_
