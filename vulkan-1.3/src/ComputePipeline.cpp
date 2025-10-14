#include <Util.hpp>
#include <pipeline/ComputePipeline.hpp>

namespace engine {
namespace compute {

ComputePipelinePacked::ComputePipelinePacked(VkDevice device,
                                             VmaAllocator allocator)
    : PipelineBasic{device, allocator, PipelineType::COMPUTE} {
  set_layout();
}

ComputePipelinePacked::~ComputePipelinePacked() { destroy(); }

void ComputePipelinePacked::init() { init_pipeline(); }

void ComputePipelinePacked::destroy() {
  if (isInit_) {
    vkDestroyDescriptorSetLayout(device_, descriptorLayout_, nullptr);
    vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
    vkDestroyPipeline(device_, pipeline_, nullptr);
    reset_init();
  }
}

void ComputePipelinePacked::set_layout() {

  DescriptorLayoutBuilder builder{device_};
  descriptorLayout_ = builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
                          .build(VK_SHADER_STAGE_COMPUTE_BIT); // add bindings
}

void ComputePipelinePacked::init_pipeline() {
  if (isInit_)
    return;

  VkPushConstantRange pushConstant{};
  pushConstant.offset = 0;
  pushConstant.size = sizeof(ComputeShaderPushConstants);
  pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  VkPipelineLayoutCreateInfo computeLayout{};
  computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  computeLayout.pNext = nullptr;
  computeLayout.pSetLayouts = &descriptorLayout_;
  computeLayout.setLayoutCount = 1;

  computeLayout.pushConstantRangeCount = 1;
  computeLayout.pPushConstantRanges = &pushConstant;

  vkCreatePipelineLayout(device_, &computeLayout, nullptr, &pipelineLayout_);

  // load shader
  VkShaderModule computeDrawShader;
  util::load_shader(CONFIG_HOME "shaders/gradient.comp.spv", device_,
                    &computeDrawShader);

  VkPipelineShaderStageCreateInfo stageinfo{};
  stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stageinfo.pNext = nullptr;
  stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  stageinfo.module = computeDrawShader;
  stageinfo.pName = "main";

  VkComputePipelineCreateInfo computePipelineCreateInfo{};
  computePipelineCreateInfo.sType =
      VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  computePipelineCreateInfo.layout = pipelineLayout_;
  computePipelineCreateInfo.stage = stageinfo;

  vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1,
                           &computePipelineCreateInfo, nullptr, &pipeline_);

  vkDestroyShaderModule(device_, computeDrawShader, nullptr);

  init_finished(); // set isinit flag = true
}

void ComputePipelinePacked::draw(VkExtent2D drawExtent,
                                 AllocatedImage &offscreen_draw,
                                 AllocatedImage &offscreen_depth,
                                 FrameData &curr_frame) {

  // now that we are sure that the commands finished executing, we can safely
  VkCommandBuffer cmd = curr_frame._mainCommandBuffer;

  vkCmdBindPipeline(cmd, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE,
                    pipeline_);

  vkCmdPushConstants(cmd, pipelineLayout_, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                     sizeof(ComputeShaderPushConstants), &data);

  VkDescriptorSet sceneSet = curr_frame.allocate(descriptorLayout_);

  DescriptorWriter writer{device_};
  writer.write_image(0, offscreen_draw.imageView, VK_NULL_HANDLE,
                     VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
  writer.update_set(sceneSet);

  // bind the descriptor set containing the draw image for the compute pipeline
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout_,
                          0, 1, &sceneSet, 0, nullptr);

  // execute the compute pipeline dispatch. We are using 16x16 workgroup size so
  // we need to divide by it
  vkCmdDispatch(cmd, static_cast<uint32_t>(std::ceil(drawExtent.width / 16.0f)),
                static_cast<uint32_t>(std::ceil(drawExtent.height / 16.0f)), 1);
}

} // namespace compute
} // namespace engine
