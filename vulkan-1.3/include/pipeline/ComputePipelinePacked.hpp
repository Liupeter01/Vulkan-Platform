#pragma once
#ifndef _COMPUTE_PIPELINE_PACKED_HPP_
#define _COMPUTE_PIPELINE_PACKED_HPP_

#include <string>
#include <GlobalDef.hpp>
#include <Descriptors.hpp>
#include <compute/Compute_ImageAttachment.hpp>

namespace engine {

inline namespace compute {

class ComputePipelinePacked {
public:
  std::string name = " ComputePipelinePacked";
  ComputePipelinePacked(VkDevice device, VmaAllocator allocator);
  virtual ~ComputePipelinePacked();
  void init() ;
  void destroy() ;
  void draw(VkExtent2D drawExtent, AllocatedImage &offscreen_draw,
            AllocatedImage &offscreen_depth, FrameData &curr_frame);
  ComputeShaderPushConstants &getData() { return data; }

protected:
  ComputeShaderPushConstants data{};

private:
          bool isInit_ = false;
          VkDevice device_;
          VmaAllocator allocator_;

          Compute_ImageAttachment imageAttachmentCompute_;
};
} // namespace compute
} // namespace engine

#endif //_COMPUTE_PIPELINE_PACKED_HPP_
