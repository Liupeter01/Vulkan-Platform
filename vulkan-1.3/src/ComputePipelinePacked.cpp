#include <Util.hpp>
#include <pipeline/ComputePipelinePacked.hpp>

namespace engine {
namespace compute {

ComputePipelinePacked::ComputePipelinePacked(VkDevice device,
                                             VmaAllocator allocator)
    :device_(device), allocator_(allocator), imageAttachmentCompute_(device) 
{}

ComputePipelinePacked::~ComputePipelinePacked() { destroy(); }

void ComputePipelinePacked::init() { 
          if (isInit_) return;
          imageAttachmentCompute_.init();
          isInit_ = true;
}

void ComputePipelinePacked::destroy() {
  if (isInit_) {
            imageAttachmentCompute_.destory();
    isInit_ = false;
  }
}

void ComputePipelinePacked::draw(VkExtent2D drawExtent,
                                 AllocatedImage &offscreen_draw,
                                 AllocatedImage &offscreen_depth,
                                 FrameData &curr_frame) {

  // now that we are sure that the commands finished executing, we can safely
  VkCommandBuffer cmd = curr_frame._mainCommandBuffer;

  ComputeResources res{ offscreen_draw.imageView };

  auto ins = 
            imageAttachmentCompute_.generate_instance(
                      res, 
                      curr_frame._frameDescriptor);

  vkCmdBindPipeline(cmd, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE,
                    ins.pipeline->getPipeline());

  vkCmdPushConstants(cmd, ins.pipeline->getPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                     sizeof(ComputeShaderPushConstants), &data);

  // bind the descriptor set containing the draw image for the compute pipeline
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, ins.pipeline->getPipelineLayout(),
                          0, 1, &ins.computeSet, 0, nullptr);

  // execute the compute pipeline dispatch. We are using 16x16 workgroup size so
  // we need to divide by it
  vkCmdDispatch(cmd, static_cast<uint32_t>(std::ceil(drawExtent.width / 16.0f)),
                static_cast<uint32_t>(std::ceil(drawExtent.height / 16.0f)), 1);
}

} // namespace compute
} // namespace engine
