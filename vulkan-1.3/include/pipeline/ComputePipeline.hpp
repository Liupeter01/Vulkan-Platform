#pragma once
#ifndef _COMPUTE_PIPELINE_HPP_
#define _COMPUTE_PIPELINE_HPP_
#include <Descriptors.hpp>
#include <GlobalDef.hpp>
#include <pipeline/PipelineBasic.hpp>
#include <string>

namespace engine {

struct ComputeShaderPushConstants {
  glm::vec4 topLeft = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);     // Red
  glm::vec4 topRight = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);    // Yellow
  glm::vec4 bottomLeft = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);  // Blue
  glm::vec4 bottomRight = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f); // Cyan
};

inline namespace compute {

class ComputePipelinePacked : public PipelineBasic {
public:
  std::string name = " ComputePipelinePacked";
  ComputePipelinePacked(VkDevice device, VmaAllocator allocator);
  virtual ~ComputePipelinePacked();
  void set_descriptors(VkImageView imageView);
  void init() override;
  void destroy() override;
  void draw(VkExtent2D drawExtent,
            AllocatedImage& offscreen_draw,
            AllocatedImage& offscreen_depth,
            FrameData& curr_frame) override;
  ComputeShaderPushConstants &getData() { return data; }

protected:
          void set_layout();
  ComputeShaderPushConstants data{};

private:
  const std::vector<PoolSizeRatio> sizes{
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}};

  DescriptorPoolAllocator descriptorAllocator_;

  // Initializing the layout and descriptors; store image, or vertex indicies
  VkDescriptorSet descriptor_;
  VkDescriptorSetLayout descriptorLayout_;

private:
  void init_pipeline();
};
} // namespace compute
} // namespace engine

#endif //_COMPUTE_PIPELINE_HPP_
