#pragma once
#ifndef _COMPUTE_COMPUTE_PIPELINE_HPP_
#define _COMPUTE_COMPUTE_PIPELINE_HPP_
#include <Util.hpp>
#include <builder/ComputePipelineBuilder.hpp>
#include <compute/Compute_Material.hpp>
#include <config.h>
#include <spdlog/spdlog.h>
#include <vector>

namespace engine {

struct ComputePipeline {
  struct SpecializationEntry {
    uint32_t id;
    std::vector<uint8_t> data;
  };

  ComputePipeline(VkDevice device);
  virtual ~ComputePipeline();

  void set_compute_shader(const std::string &path, const std::string &entry);

  template <typename T>
  void set_specialization_constant(uint32_t constantID, const T &value) {
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&value);
    specializationConstants_.push_back(
        {constantID, std::vector<uint8_t>(bytes, bytes + sizeof(T))});
  }

  template <typename _Ty>
  void create(ComputePass pass,
              const std::vector<VkDescriptorSetLayout> &layouts) {

    if (isinit_)
      return;

    static_assert(sizeof(_Ty) <= 128,
                  "Push constant size exceeds Vulkan 128-byte limit!");

    if (pass == ComputePass::UNDEFINED) {
      throw std::runtime_error("Undefined ComputePass!");
    }

    if (isUsingDefaultShader) {
      spdlog::info("[ComputePipeline info]: ComputePass {}, Using default "
                   "Compute Shader settings.",
                   static_cast<int>(pass));
    }

    VkPushConstantRange pushConstant{};
    pushConstant.offset = 0;
    pushConstant.size = sizeof(_Ty);
    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkPipelineLayoutCreateInfo computeLayout{};
    computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    computeLayout.pNext = nullptr;
    computeLayout.setLayoutCount = static_cast<uint32_t>(layouts.size());
    computeLayout.pSetLayouts = layouts.data();
    computeLayout.pushConstantRangeCount = 1;
    computeLayout.pPushConstantRanges = &pushConstant;

    vkCreatePipelineLayout(device_, &computeLayout, nullptr, &pipelineLayout_);

    ComputePipelineBuilder builder{device_};
    builder.set_pipeline_layout(pipelineLayout_)
        .set_shaders_stage(compute_stage);

    if (pass == ComputePass::DEFAULT) {
      create_default_pipeline(builder);
    } else if (pass == ComputePass::SPECIALCONSTANT) {
      create_specialize_constant_pipeline(builder);
    }

    isinit_ = true;
  }

  void destroy();

  VkPipeline getPipeline() { return pipeline_; }
  VkPipelineLayout getPipelineLayout() { return pipelineLayout_; }

protected:
  void create_default_pipeline(ComputePipelineBuilder &builder);
  void create_specialize_constant_pipeline(ComputePipelineBuilder &builder);

  ComputePipelineBuilder::ShaderStageDesc compute_stage{
      GLSL_SHADER_PATH "gradient.comp.spv", "main",
      VK_SHADER_STAGE_COMPUTE_BIT};

private:
  bool isinit_ = false;
  bool isUsingDefaultShader = true;
  VkDevice device_ = VK_NULL_HANDLE;

  VkPipeline pipeline_ = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;

  std::vector<SpecializationEntry> specializationConstants_;
};

} // namespace engine

#endif //_COMPUTE_COMPUTE_PIPELINE_HPP_
