#pragma once
#ifndef _PARTICLESYS2D_HPP_
#define _PARTICLESYS2D_HPP_
#include <builder/BarrierBuilder.hpp>
#include <compute/Compute_EffectBase.hpp>
#include <particle/ParticleDataBuffer.hpp>

// IMGUI Support
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

namespace engine {

class VulkanEngine;

namespace particle2d {

struct alignas(16) ParticlePushConstant {
  glm::vec4 speedup{1.f, 1.f, 0.f, 0.f};
  float deltaTime{}; //
  // uint32_t frameIndex{}; //
};

struct alignas(16) GPUParticle {
  glm::vec4 position;
  glm::vec4 velocity;
  glm::vec4 color;
};

struct ParticleResources {
  VkImageView colorImage{};

  // Ping Pong Swapping structure
  VkBuffer particlesIn;
  VkBuffer particlesOut;
  std::size_t bufferSize{};
};

template <typename PushConstantType = ParticlePushConstant,
          typename ResourcesType = ParticleResources>
class Compute_ParticleSys2D
    : public Compute_EffectBase<PushConstantType, ResourcesType> {

public:
  Compute_ParticleSys2D(VkDevice device)
      : Compute_EffectBase<PushConstantType, ResourcesType>(device) {}

  void init() override {
    if (this->isinit_)
      return;

    DescriptorLayoutBuilder builder{this->device_};
    this->computeLayout_ =
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
            .add_binding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
            .add_binding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
            .build(VK_SHADER_STAGE_COMPUTE_BIT); // add bindings

    auto layout = {this->computeLayout_};

    this->defaultComputePipeline_.set_compute_shader(
        SLANG_SHADER_PATH "particle2D.slang.spv", "compMain");
    this->defaultComputePipeline_.template create<PushConstantType>(
        ComputePass::DEFAULT, layout);

    this->specialConstantPipeline_.set_compute_shader(
        SLANG_SHADER_PATH "particle2D.slang.spv", "compMain");
    this->specialConstantPipeline_.template create<PushConstantType>(
        ComputePass::SPECIALCONSTANT, layout);
    this->isinit_ = true;
  }

  void on_gui() {

    if (ImGui::CollapsingHeader("Compute_ParticleSys2D",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Text("Speed Multiplier (0.1x ~ 5x)");

      ImGui::SliderFloat("Speed X", &this->pushConstantData_.speedup.x, 0.1f,
                         5.0f, "%.2fx");
      ImGui::SliderFloat("Speed Y", &this->pushConstantData_.speedup.y, 0.1f,
                         5.0f, "%.2fx");
      ImGui::SliderFloat("Speed Z", &this->pushConstantData_.speedup.z, 0.1f,
                         5.0f, "%.2fx");
      ImGui::SliderFloat("Speed W", &this->pushConstantData_.speedup.w, 0.1f,
                         5.0f, "%.2fx");

      ImGui::Separator();
    }
  }

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

    VkMemoryBarrier2 memBarrier{};
    memBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
    memBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    memBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    memBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    memBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

    BarrierBuilder{}.add(memBarrier).createBarrier(cmd);
  }

  ComputeInstance generate_instance(
      ResourcesType &resources,
      DescriptorPoolAllocator &globalDescriptorAllocator) override {

    this->writer_.clear();

    ComputeInstance ins{};
    ins.pipeline = &this->defaultComputePipeline_;
    ins.computeSet = globalDescriptorAllocator.allocate(this->computeLayout_);

    this->writer_.write_buffer(0, resources.particlesIn, resources.bufferSize,
                               0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    this->writer_.write_buffer(1, resources.particlesOut, resources.bufferSize,
                               0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    this->writer_.write_image(2, resources.colorImage, VK_NULL_HANDLE,
                              VK_IMAGE_LAYOUT_GENERAL,
                              VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

    this->writer_.update_set(ins.computeSet);
    return ins;
  }
};
} // namespace particle2d
} // namespace engine

#endif //_COMPUTE_PARTICLESYS_HPP_
