#include <Util.hpp>
#include <pipeline/ComputePipeline.hpp>

namespace engine {
namespace compute {

ComputePipelinePacked::ComputePipelinePacked(VkDevice device)
    : device_(device), isInit_(false), descriptorAllocator_{device, 10, sizes},
      builder{device} {}

ComputePipelinePacked::~ComputePipelinePacked() { destroy(); }

void ComputePipelinePacked::init(VkImageView imageView) {
  init_descriptors(imageView);
  init_pipeline();

  isInit_ = true;
}

void ComputePipelinePacked::destroy() {
  destroy_pipeline();
  destroy_descriptors();

  isInit_ = false;
}

void ComputePipelinePacked::destroy_descriptors() {
  if (isInit_) {
    descriptorAllocator_.destroy_pool();
    vkDestroyDescriptorSetLayout(device_, drawCompDescriptorLayout_, nullptr);
  }
}

void ComputePipelinePacked::destroy_pipeline() {
  if (isInit_) {
    vkDestroyPipelineLayout(device_, gradientComputePipelineLayout_, nullptr);
    vkDestroyPipeline(device_, gradientComputePipeline_, nullptr);
  }
}

void ComputePipelinePacked::init_descriptors(VkImageView imageView) {

  drawCompDescriptorLayout_ =
      builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
          .build(VK_SHADER_STAGE_COMPUTE_BIT); // add bindings

  // allocate a descriptor set for our draw image
  drawCompDescriptor_ =
      descriptorAllocator_.allocate(drawCompDescriptorLayout_);

  VkDescriptorImageInfo imgInfo{};
  imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  imgInfo.imageView = imageView;

  VkWriteDescriptorSet drawImageWrite = {};
  drawImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

  drawImageWrite.dstBinding = 0;
  drawImageWrite.dstSet = drawCompDescriptor_;
  drawImageWrite.descriptorCount = 1;
  drawImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  drawImageWrite.pImageInfo = &imgInfo;

  vkUpdateDescriptorSets(device_, 1, &drawImageWrite, 0, nullptr);
}

void ComputePipelinePacked::init_pipeline() {

  VkPushConstantRange pushConstant{};
  pushConstant.offset = 0;
  pushConstant.size = sizeof(ComputeShaderPushConstants);
  pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  VkPipelineLayoutCreateInfo computeLayout{};
  computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  computeLayout.pNext = nullptr;
  computeLayout.pSetLayouts = &drawCompDescriptorLayout_;
  computeLayout.setLayoutCount = 1;

  computeLayout.pushConstantRangeCount = 1;
  computeLayout.pPushConstantRanges = &pushConstant;

  vkCreatePipelineLayout(device_, &computeLayout, nullptr,
                         &gradientComputePipelineLayout_);

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
  computePipelineCreateInfo.layout = gradientComputePipelineLayout_;
  computePipelineCreateInfo.stage = stageinfo;

  vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1,
                           &computePipelineCreateInfo, nullptr,
                           &gradientComputePipeline_);

  vkDestroyShaderModule(device_, computeDrawShader, nullptr);
}

void ComputePipelinePacked::draw_compute(VkCommandBuffer &cmd,
                                         VkExtent2D drawExtent) {

  vkCmdBindPipeline(cmd, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE,
                    gradientComputePipeline_);

  vkCmdPushConstants(cmd, gradientComputePipelineLayout_,
                     VK_SHADER_STAGE_COMPUTE_BIT, 0,
                     sizeof(ComputeShaderPushConstants), &data);

  // bind the descriptor set containing the draw image for the compute pipeline
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                          gradientComputePipelineLayout_, 0, 1,
                          &drawCompDescriptor_, 0, nullptr);

  // execute the compute pipeline dispatch. We are using 16x16 workgroup size so
  // we need to divide by it
  vkCmdDispatch(cmd, static_cast<uint32_t>(std::ceil(drawExtent.width / 16.0f)),
                static_cast<uint32_t>(std::ceil(drawExtent.height / 16.0f)), 1);
}

} // namespace compute
} // namespace engine
