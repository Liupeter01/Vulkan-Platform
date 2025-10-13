#pragma once
#ifndef _PIPELINE_BASIC_HPP_
#define _PIPELINE_BASIC_HPP_
#include <GlobalDef.hpp>

namespace engine {
class PipelineBasic {
public:
  enum class PipelineType { UNDEFINED, COMPUTE, GRAPHIC };

  PipelineBasic(VkDevice device, VmaAllocator allocator, PipelineType type)
      : device_(device), allocator_(allocator), type_(type), isInit_(false) {}

  virtual ~PipelineBasic() {
    if (isInit_) {
      vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
      vkDestroyPipeline(device_, pipeline_, nullptr);
      reset_init();
    }
  }

  PipelineBasic(const PipelineBasic &) = delete;
  PipelineBasic &operator=(const PipelineBasic &) = delete;

  virtual void draw(VkExtent2D drawExtent,
            AllocatedImage& offscreen_draw,
            AllocatedImage& offscreen_depth,
            FrameData& curr_frame) = 0;
  bool isCompute() const { return type_ == PipelineType::COMPUTE; }
  bool isGraphics() const { return type_ == PipelineType::GRAPHIC; }
  virtual void init() = 0;
  virtual void destroy() = 0;

protected:
  void reset_init() { isInit_ = false; }
  void init_finished() { isInit_ = true; }

protected:
  bool isInit_ = false;
  PipelineType type_ = PipelineType::UNDEFINED;
  VkDevice device_ = VK_NULL_HANDLE;
  VmaAllocator allocator_{};
  VkPipeline pipeline_ = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
};

} // namespace engine

#endif //_PIPELINE_BASIC_HPP_
