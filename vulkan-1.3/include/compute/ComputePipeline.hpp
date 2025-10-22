#pragma once
#ifndef _COMPUTE_COMPUTE_PIPELINE_HPP_
#define _COMPUTE_COMPUTE_PIPELINE_HPP_
#include <Util.hpp>
#include <compute/Compute_Material.hpp>
#include <vector>

namespace engine {

struct ComputePipeline {
  ComputePipeline(VkDevice device) : device_(device) {}

  virtual ~ComputePipeline() { destroy(); }

  void create(const std::vector<VkDescriptorSetLayout> &layouts) {

            if (isinit_) return;
    VkPushConstantRange pushConstant{};
    pushConstant.offset = 0;
    pushConstant.size = sizeof(ComputeShaderPushConstants);
    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkPipelineLayoutCreateInfo computeLayout{};
    computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    computeLayout.pNext = nullptr;
    computeLayout.setLayoutCount = static_cast<uint32_t>(layouts.size());
    computeLayout.pSetLayouts = layouts.data();
    computeLayout.pushConstantRangeCount = 1;
    computeLayout.pPushConstantRanges = &pushConstant;

    vkCreatePipelineLayout(device_, &computeLayout, nullptr, &pipelineLayout_);
    create_default_pipeline();

    isinit_ = true;
  }
  void destroy() {
    if (isinit_) {
      vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
      vkDestroyPipeline(device_, pipeline_, nullptr);
      isinit_ = false;
    }
  }

  VkPipeline getPipeline() { return pipeline_; }
  VkPipelineLayout getPipelineLayout() { return pipelineLayout_; }

protected:
  void create_default_pipeline() {
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
  }

private:
  bool isinit_ = false;
  VkDevice device_ = VK_NULL_HANDLE;

  VkPipeline pipeline_ = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
};

} // namespace engine

#endif //_COMPUTE_COMPUTE_PIPELINE_HPP_
