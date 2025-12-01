#pragma once
#ifndef _COMPUTE_IMAGEATTACHMENT_HPP_
#define _COMPUTE_IMAGEATTACHMENT_HPP_
#include <compute/Compute_EffectBase.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

namespace engine {

struct ComputeShaderPushConstants {
  glm::vec4 topLeft = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);     // Red
  glm::vec4 topRight = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);    // Yellow
  glm::vec4 bottomLeft = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);  // Blue
  glm::vec4 bottomRight = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f); // Cyan
};

struct ImageAttachmentResources {
  VkImageView colorImageView{};
};

inline namespace compute {

template <typename PushConstantType = ComputeShaderPushConstants,
          typename ResourcesType = ImageAttachmentResources>

class Compute_ImageAttachment
    : public Compute_EffectBase<PushConstantType, ResourcesType> {

public:
  Compute_ImageAttachment(VkDevice device)
      : Compute_EffectBase<PushConstantType, ResourcesType>(device) {}

  void init() override {
    if (this->isinit_)
      return;

    DescriptorLayoutBuilder builder{this->device_};
    this->computeLayout_ =
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
            .build(VK_SHADER_STAGE_COMPUTE_BIT); // add bindings

    auto layout = {this->computeLayout_};

    this->defaultComputePipeline_.set_compute_shader(
        GLSL_SHADER_PATH "gradient.comp.spv", "main");
    this->defaultComputePipeline_.template create<PushConstantType>(
        ComputePass::DEFAULT, layout);

    this->specialConstantPipeline_.set_compute_shader(
        GLSL_SHADER_PATH "gradient.comp.spv", "main");
    this->specialConstantPipeline_.template create<PushConstantType>(
        ComputePass::SPECIALCONSTANT, layout);
    this->isinit_ = true;
  }

  bool has_graphic() const override { return false; }
  bool has_compute() const override { return true; }

  void render(VkCommandBuffer cmd, const ComputeInstance &ins) override {}

  void dispatch(VkCommandBuffer cmd, const ComputeInstance &ins) override {

    vkCmdBindPipeline(cmd, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE,
                      ins.pipeline->getPipeline());

    vkCmdPushConstants(cmd, ins.pipeline->getPipelineLayout(),
                       VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstantType),
                       &this->pushConstantData_);

    // bind the descriptor set containing the draw image for the compute
    // pipeline
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                            ins.pipeline->getPipelineLayout(), 0, 1,
                            &ins.computeSet, 0, nullptr);

    // execute the compute pipeline dispatch. We are using 16x16 workgroup size
    // so we need to divide by it

    vkCmdDispatch(cmd, this->dispatchGroups_.x, this->dispatchGroups_.y,
                  this->dispatchGroups_.z);
  }

  void on_gui() {

    if (ImGui::CollapsingHeader("Compute_ImageAttachment",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::InputFloat4("Top Left", (float *)&this->pushConstantData_.topLeft,
                         "%.3f", ImGuiInputTextFlags_ElideLeft);
      ImGui::InputFloat4("Top Right",
                         (float *)&this->pushConstantData_.topRight, "%.3f",
                         ImGuiInputTextFlags_ElideLeft);
      ImGui::InputFloat4("Bottom Left",
                         (float *)&this->pushConstantData_.bottomLeft, "%.3f",
                         ImGuiInputTextFlags_ElideLeft);
      ImGui::InputFloat4("Bottom Right",
                         (float *)&this->pushConstantData_.bottomRight, "%.3f",
                         ImGuiInputTextFlags_ElideLeft);
      ImGui::Separator();
    }
  }

  ComputeInstance generate_instance(
      ResourcesType &resources,
      DescriptorPoolAllocator &globalDescriptorAllocator) override {

    this->writer_.clear();

    ComputeInstance ins{};
    ins.pipeline = &this->defaultComputePipeline_;
    ins.computeSet = globalDescriptorAllocator.allocate(this->computeLayout_);

    this->writer_.write_image(0, resources.colorImageView, VK_NULL_HANDLE,
                              VK_IMAGE_LAYOUT_GENERAL,
                              VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    this->writer_.update_set(ins.computeSet);
    return ins;
  }
};
} // namespace compute
} // namespace engine

#endif //_COMPUTE_IMAGEATTACHMENT_HPP_
