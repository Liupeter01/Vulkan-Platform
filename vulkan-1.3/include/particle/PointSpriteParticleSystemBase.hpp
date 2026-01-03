#pragma once
#ifndef _POINT_SPRITE_PARTICLE_SYSTEM_HPP_
#define _POINT_SPRITE_PARTICLE_SYSTEM_HPP_
#include <array>
#include <builder/BarrierBuilder.hpp>
#include <compute/Compute_EffectBase.hpp>
#include <material/Graphic_EffectBase.hpp>
#include <particle/ParticleDataBuffer.hpp>

// IMGUI Support
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

namespace engine {
namespace particle {
struct alignas(16) ParticlePushConstant {
  glm::vec4 speedup{1.f, 1.f, 1.f, 0.f};
  float deltaTime{}; //
};

struct GPUParticle {
  glm::vec4 position{0.f, 0.f, 0.f, 1.f};
  glm::vec4 velocity{0.f};
  glm::vec4 color{1.f};
};

struct ParticleComputeResourcesAOS {
  // Ping Pong Swapping structure
  VkBuffer particlesIn;
  VkBuffer particlesOut;
  std::size_t bufferSize{};
};

struct ParticleGraphicResourcesAOS {
  // Ping Pong Swapping structure
  VkBuffer particlesIn;
  std::size_t bufferSize{};
};

struct ParticleCompResourcesSOA {
  // Ping Pong Swapping structure
  VkBuffer positionIn;
  VkBuffer positionOut;
  std::size_t positionBufferSize{};
  VkBuffer velocityIn;
  VkBuffer velocityOut;
  std::size_t velocityBufferSize{};
  VkBuffer colorIn;
  VkBuffer colorOut;
  std::size_t colorBufferSize{};
};

struct ParticleGraphicResourcesSOA {
  // Ping Pong Swapping structure
  VkBuffer positionIn;
  std::size_t positionBufferSize{};
  VkBuffer colorIn;
  std::size_t colorBufferSize{};
};

struct alignas(16) ParticleDrawPushConstant {
  glm::vec4 size{2.0f, 0.f, 0.f, 0.f}; //
};

namespace details {

template <typename ComputePushConstantType, typename ComputeResourcesType,
          typename GraphicPushConstantType, typename GraphicResourcesType>
class PointSpriteParticleSystemBase
    : public Compute_EffectBase<ComputePushConstantType, ComputeResourcesType>,
      public Graphic_EffectBase<GraphicPushConstantType, GraphicResourcesType> {

public:
  PointSpriteParticleSystemBase(VkDevice device)
      : Compute_EffectBase<ComputePushConstantType, ComputeResourcesType>(
            device),
        Graphic_EffectBase<GraphicPushConstantType, GraphicResourcesType>(
            device),
        device_(device) {}

public:
  virtual void init() {
    if (this->isInit_)
      return;

    this->ensure_compute_initialized();
    this->ensure_graphic_initialized();

    this->isInit_ = true;
  }

  void setGlobalLayout(VkDescriptorSetLayout globalSceneLayout,
                       VkDescriptorSet globalSceneSet) {
    globalSceneSet_ = globalSceneSet;
    globalSceneLayout_ = globalSceneLayout;
  }

  void set_vertex_shader(const std::string &path, const std::string &entry) {
    stages[0].path = path;
    stages[0].entry = entry;
  }

  void set_fragment_shader(const std::string &path, const std::string &entry) {
    stages[1].path = path;
    stages[1].entry = entry;
  }

  virtual void on_gui() {

    if (ImGui::CollapsingHeader("Compute_ParticleSys",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Text("Speed Multiplier (0.1x ~ 5x)");

      ImGui::SliderFloat("Speed X", &this->getCompPushConstantData().speedup.x,
                         0.1f, 5.0f, "%.2fx");
      ImGui::SliderFloat("Speed Y", &this->getCompPushConstantData().speedup.y,
                         0.1f, 5.0f, "%.2fx");
      ImGui::SliderFloat("Speed Z", &this->getCompPushConstantData().speedup.z,
                         0.1f, 5.0f, "%.2fx");
      ImGui::SliderFloat("Speed W", &this->getCompPushConstantData().speedup.w,
                         0.1f, 5.0f, "%.2fx");

      ImGui::Separator();
    }
  }

  bool has_graphic() const override { return true; }
  bool has_compute() const override { return true; }

  void render(VkCommandBuffer cmd, const MaterialInstance &ins) override {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      ins.pipeline->getPipeline());

    vkCmdPushConstants(
        cmd, ins.pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0,
        sizeof(GraphicPushConstantType), &this->getGraphicPushConstantData());

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            ins.pipeline->getPipelineLayout(), 0, 1,
                            &globalSceneSet_, 0, nullptr);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            ins.pipeline->getPipelineLayout(), 1, 1,
                            &ins.materialSet, 0, nullptr);

    vkCmdDraw(cmd, this->dispatchGroups_.x * 256, 1, 0, 0);
  }

  void dispatch(VkCommandBuffer cmd, const ComputeInstance &ins) override {

    vkCmdBindPipeline(cmd, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE,
                      ins.pipeline->getPipeline());

    vkCmdPushConstants(
        cmd, ins.pipeline->getPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
        sizeof(ComputePushConstantType), &this->getCompPushConstantData());

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

protected:
  bool isInit_ = false;
  VkDevice device_;
  VkDescriptorSet globalSceneSet_{};
  VkDescriptorSetLayout globalSceneLayout_{};

  std::vector<GraphicPipelineBuilder::ShaderStageDesc> stages{
      GraphicPipelineBuilder::ShaderStageDesc{
          SLANG_SHADER_PATH "SpriteParticleDraw3D.slang.spv", "vertMain",
          VK_SHADER_STAGE_VERTEX_BIT},
      GraphicPipelineBuilder::ShaderStageDesc{
          SLANG_SHADER_PATH "SpriteParticleDraw3D.slang.spv", "fragMain",
          VK_SHADER_STAGE_FRAGMENT_BIT}};
};

} // namespace details

template <typename ComputePushConstantType = ParticlePushConstant,
          typename ComputeResourcesType = ParticleComputeResourcesAOS,
          typename GraphicPushConstantType = ParticleDrawPushConstant,
          typename GraphicResourcesType = ParticleGraphicResourcesAOS>

class PointSpriteParticleSystemBaseAOS
    : public details::PointSpriteParticleSystemBase<
          ComputePushConstantType, ComputeResourcesType,
          GraphicPushConstantType, GraphicResourcesType> {

  using BaseClass = details::PointSpriteParticleSystemBase<
      ComputePushConstantType, ComputeResourcesType, GraphicPushConstantType,
      GraphicResourcesType>;

public:
  PointSpriteParticleSystemBaseAOS(VkDevice device) : BaseClass(device) {}

  ComputeInstance generate_comp_instance(
      ComputeResourcesType &resources,
      DescriptorPoolAllocator &globalDescriptorAllocator) override {

    this->computeWriter_.clear();

    ComputeInstance ins{};
    ins.pipeline = &this->defaultComputePipeline_;
    ins.computeSet = globalDescriptorAllocator.allocate(this->computeLayout_);

    this->computeWriter_.write_buffer(0, resources.particlesIn,
                                      resources.bufferSize, 0,
                                      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    this->computeWriter_.write_buffer(1, resources.particlesOut,
                                      resources.bufferSize, 0,
                                      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    this->computeWriter_.update_set(ins.computeSet);
    return ins;
  }

  MaterialInstance generate_graphic_instance(
      GraphicResourcesType &resources,
      DescriptorPoolAllocator &globalDescriptorAllocator) override {

    this->graphicWriter_.clear();

    MaterialInstance ins{};
    ins.pipeline = &this->defaultOpaquePointPipeline_;
    ins.materialSet = globalDescriptorAllocator.allocate(this->graphicLayout_);

    this->graphicWriter_.write_buffer(0, resources.particlesIn,
                                      resources.bufferSize, 0,
                                      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    this->graphicWriter_.update_set(ins.materialSet);
    return ins;
  }

protected:
  void init_compute() override {

    if (this->isComputeInit_)
      return;

    DescriptorLayoutBuilder builder{this->device_};
    this->computeLayout_ =
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
            .add_binding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
            .build(VK_SHADER_STAGE_COMPUTE_BIT |
                   VK_SHADER_STAGE_VERTEX_BIT); // add bindings

    this->defaultComputePipeline_.set_compute_shader(
        SLANG_SHADER_PATH "SpriteParticleComputeAOS.slang.spv", "compMain");
    this->defaultComputePipeline_.template create<ComputePushConstantType>(
        ComputePass::DEFAULT, {this->computeLayout_});

    this->specialConstantPipeline_.set_compute_shader(
        SLANG_SHADER_PATH "SpriteParticleComputeAOS.slang.spv", "compMain");
    this->specialConstantPipeline_.template create<ComputePushConstantType>(
        ComputePass::SPECIALCONSTANT, {this->computeLayout_});

    this->isComputeInit_ = true;
  }

  void init_graphic() override {

    if (!this->globalSceneLayout_ || !this->globalSceneSet_) {
      throw std::runtime_error(
          "Invalid Global Scene Layout! You must setGlobalLayout At First");
    }

    if (this->isGraphicInit_)
      return;

    DescriptorLayoutBuilder builder{this->device_};
    this->graphicLayout_ =
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
            .build(VK_SHADER_STAGE_COMPUTE_BIT |
                   VK_SHADER_STAGE_VERTEX_BIT); // add bindings

    /*Graphic Pipeline*/
    auto layout = {this->globalSceneLayout_, this->graphicLayout_};

    this->defaultOpaquePipeline_.set_vertex_shader(this->stages[0].path,
                                                   this->stages[0].entry);
    this->defaultOpaquePipeline_.set_fragment_shader(this->stages[1].path,
                                                     this->stages[1].entry);
    this->defaultOpaquePipeline_.template create<GraphicPushConstantType>(
        MaterialPass::OPAQUE, layout);

    this->transparentPipeline_.set_vertex_shader(this->stages[0].path,
                                                 this->stages[0].entry);
    this->transparentPipeline_.set_fragment_shader(this->stages[1].path,
                                                   this->stages[1].entry);
    this->transparentPipeline_.template create<GraphicPushConstantType>(
        MaterialPass::TRANSPARENT, layout);

    this->defaultOpaquePointPipeline_.set_vertex_shader(this->stages[0].path,
                                                        this->stages[0].entry);
    this->defaultOpaquePointPipeline_.set_fragment_shader(
        this->stages[1].path, this->stages[1].entry);
    this->defaultOpaquePointPipeline_.template create<GraphicPushConstantType>(
        MaterialPass::OPAQUE_POINT, layout);

    this->isGraphicInit_ = true;
  }
};

template <typename ComputePushConstantType = ParticlePushConstant,
          typename ComputeResourcesType = ParticleCompResourcesSOA,
          typename GraphicPushConstantType = ParticleDrawPushConstant,
          typename GraphicResourcesType = ParticleGraphicResourcesSOA>
class PointSpriteParticleSystemBaseSOA
    : public details::PointSpriteParticleSystemBase<
          ComputePushConstantType, ComputeResourcesType,
          GraphicPushConstantType, GraphicResourcesType> {

  using BaseClass = details::PointSpriteParticleSystemBase<
      ComputePushConstantType, ComputeResourcesType, GraphicPushConstantType,
      GraphicResourcesType>;

public:
  PointSpriteParticleSystemBaseSOA(VkDevice device) : BaseClass(device) {}

  ComputeInstance generate_comp_instance(
      ComputeResourcesType &resources,
      DescriptorPoolAllocator &globalDescriptorAllocator) override {

    this->computeWriter_.clear();

    ComputeInstance ins{};
    ins.pipeline = &this->defaultComputePipeline_;
    ins.computeSet = globalDescriptorAllocator.allocate(this->computeLayout_);

    this->computeWriter_.write_buffer(0, resources.positionIn,
                                      resources.positionBufferSize, 0,
                                      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    this->computeWriter_.write_buffer(1, resources.positionOut,
                                      resources.positionBufferSize, 0,
                                      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    this->computeWriter_.write_buffer(2, resources.velocityIn,
                                      resources.velocityBufferSize, 0,
                                      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    this->computeWriter_.write_buffer(3, resources.velocityOut,
                                      resources.velocityBufferSize, 0,
                                      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    this->computeWriter_.write_buffer(4, resources.colorIn,
                                      resources.colorBufferSize, 0,
                                      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    this->computeWriter_.write_buffer(5, resources.colorOut,
                                      resources.colorBufferSize, 0,
                                      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    this->computeWriter_.update_set(ins.computeSet);
    return ins;
  }

  MaterialInstance generate_graphic_instance(
      GraphicResourcesType &resources,
      DescriptorPoolAllocator &globalDescriptorAllocator) override {

    this->graphicWriter_.clear();

    MaterialInstance ins{};
    ins.pipeline = &this->defaultOpaquePointPipeline_;
    ins.materialSet = globalDescriptorAllocator.allocate(this->graphicLayout_);

    this->graphicWriter_.write_buffer(0, resources.positionIn,
                                      resources.positionBufferSize, 0,
                                      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    this->graphicWriter_.write_buffer(1, resources.colorIn,
                                      resources.colorBufferSize, 0,
                                      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    this->graphicWriter_.update_set(ins.materialSet);
    return ins;
  }

protected:
  void init_compute() override {

    if (this->isComputeInit_)
      return;

    DescriptorLayoutBuilder builder{this->device_};
    this->computeLayout_ =
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
            .add_binding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
            .add_binding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
            .add_binding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
            .add_binding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
            .add_binding(5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
            .build(VK_SHADER_STAGE_COMPUTE_BIT |
                   VK_SHADER_STAGE_VERTEX_BIT); // add bindings

    this->defaultComputePipeline_.set_compute_shader(
        SLANG_SHADER_PATH "SpriteParticleComputeSOA.slang.spv", "compMain");
    this->defaultComputePipeline_.template create<ComputePushConstantType>(
        ComputePass::DEFAULT, {this->computeLayout_});

    this->specialConstantPipeline_.set_compute_shader(
        SLANG_SHADER_PATH "SpriteParticleComputeSOA.slang.spv", "compMain");
    this->specialConstantPipeline_.template create<ComputePushConstantType>(
        ComputePass::SPECIALCONSTANT, {this->computeLayout_});

    this->isComputeInit_ = true;
  }

  void init_graphic() override {

    if (!this->globalSceneLayout_ || !this->globalSceneSet_) {
      throw std::runtime_error(
          "Invalid Global Scene Layout! You must setGlobalLayout At First");
    }

    if (this->isGraphicInit_)
      return;

    DescriptorLayoutBuilder builder{this->device_};
    this->graphicLayout_ =
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
            .add_binding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
            .build(VK_SHADER_STAGE_COMPUTE_BIT |
                   VK_SHADER_STAGE_VERTEX_BIT); // add bindings

    /*Graphic Pipeline*/
    auto layout = {this->globalSceneLayout_, this->graphicLayout_};

    this->defaultOpaquePipeline_.set_vertex_shader(this->stages[0].path,
                                                   this->stages[0].entry);
    this->defaultOpaquePipeline_.set_fragment_shader(this->stages[1].path,
                                                     this->stages[1].entry);
    this->defaultOpaquePipeline_.template create<GraphicPushConstantType>(
        MaterialPass::OPAQUE, layout);

    this->transparentPipeline_.set_vertex_shader(this->stages[0].path,
                                                 this->stages[0].entry);
    this->transparentPipeline_.set_fragment_shader(this->stages[1].path,
                                                   this->stages[1].entry);
    this->transparentPipeline_.template create<GraphicPushConstantType>(
        MaterialPass::TRANSPARENT, layout);

    this->defaultOpaquePointPipeline_.set_vertex_shader(this->stages[0].path,
                                                        this->stages[0].entry);
    this->defaultOpaquePointPipeline_.set_fragment_shader(
        this->stages[1].path, this->stages[1].entry);
    this->defaultOpaquePointPipeline_.template create<GraphicPushConstantType>(
        MaterialPass::OPAQUE_POINT, layout);

    this->isGraphicInit_ = true;
  }
};

} // namespace particle
} // namespace engine

#endif // _POINT_SPRITE_PARTICLE_SYSTEM_HPP_
