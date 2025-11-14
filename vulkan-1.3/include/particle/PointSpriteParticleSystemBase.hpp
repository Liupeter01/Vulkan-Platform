#pragma once
#ifndef _POINT_SPRITE_PARTICLE_SYSTEM_HPP_
#define _POINT_SPRITE_PARTICLE_SYSTEM_HPP_
#include <array>
#include <material/MaterialPipeline.hpp>
#include <particle/ParticleDataBuffer.hpp>
#include <builder/BarrierBuilder.hpp>
#include <compute/Compute_EffectBase.hpp>

// IMGUI Support
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

namespace engine {
          namespace particle {
                    struct alignas(16) ParticlePushConstant {
                              glm::vec4 speedup{ 1.f, 1.f, 1.f, 0.f };
                              float deltaTime{}; //
                    };

                    struct GPUParticle {
                              glm::vec4 position{ 0.f,0.f,0.f,1.f };
                              glm::vec4 velocity{ 0.f };
                              glm::vec4 color{ 1.f };
                    };

                    struct ParticleResources {
                              // Ping Pong Swapping structure
                              VkBuffer particlesIn;
                              VkBuffer particlesOut;
                              std::size_t bufferSize{};
                    };

                    template <typename PushConstantType = ParticlePushConstant,
                              typename ResourcesType = ParticleResources>
                    class PointSpriteParticleSystemBase
                              : public Compute_EffectBase<PushConstantType, ResourcesType> {

                    public:
                              PointSpriteParticleSystemBase(VkDevice device)
                                        : Compute_EffectBase<PushConstantType, ResourcesType>(device)
                                        , pointOpaquePipeline_(device)
                                        , device_(device)
                              {

                              }

                              void setGlobalLayout(VkDescriptorSetLayout globalSceneLayout, VkDescriptorSet globalSceneSet) {
                                        globalSceneSet_ = globalSceneSet;
                                        globalSceneLayout_ = globalSceneLayout;
                              }

                              void set_vertex_shader(const std::string& path, const std::string& entry) {
                                        stages[0].path = path;
                                        stages[0].entry = entry;
                              }

                              void set_fragment_shader(const std::string& path,
                                        const std::string& entry) {
                                        stages[1].path = path;
                                        stages[1].entry = entry;
                              }

                              void init() override {
                                        if (this->isinit_)
                                                  return;

                                        if (!globalSceneLayout_ || !globalSceneSet_) {
                                                  throw std::runtime_error("Invalid Global Scene Layout! You must setGlobalLayout At First");
                                        }

                                        DescriptorLayoutBuilder builder{ this->device_ };
                                        this->computeLayout_ =
                                                  builder
                                                  .add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                                                  .add_binding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                                                  .build(VK_SHADER_STAGE_COMPUTE_BIT |
                                                            VK_SHADER_STAGE_VERTEX_BIT); // add bindings

                                        this->defaultComputePipeline_.set_compute_shader(SLANG_SHADER_PATH"SpriteParticleCompute.slang.spv", "compMain");
                                        this->defaultComputePipeline_.template create<PushConstantType>(
                                                  ComputePass::DEFAULT, { this->computeLayout_ });

                                        this->specialConstantPipeline_.set_compute_shader(SLANG_SHADER_PATH"SpriteParticleCompute.slang.spv", "compMain");
                                        this->specialConstantPipeline_.template create<PushConstantType>(
                                                  ComputePass::SPECIALCONSTANT, { this->computeLayout_ });

                                        /*Graphic Pipeline*/
                                        auto layout = { globalSceneLayout_, this->computeLayout_ };

                                        pointOpaquePipeline_.set_vertex_shader(stages[0].path, stages[0].entry);
                                        pointOpaquePipeline_.set_fragment_shader(stages[1].path, stages[1].entry);
                                        pointOpaquePipeline_.template create<>(MaterialPass::OPAQUE_POINT, layout);

                                        this->isinit_ = true;
                              }

                              virtual void on_gui() {

                                        if (ImGui::CollapsingHeader("Compute_ParticleSys3D",
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

                              bool has_graphic() const override { return true; }
                              bool has_compute() const override { return true; }

                              void render(VkCommandBuffer cmd, const ComputeInstance& ins) override {

                                        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                  pointOpaquePipeline_.getPipeline());

                                        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                  pointOpaquePipeline_.getPipelineLayout(), 0, 1,
                                                  &globalSceneSet_, 0, nullptr);

                                        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                  pointOpaquePipeline_.getPipelineLayout(), 1, 1,
                                                  &ins.computeSet, 0, nullptr);

                                        vkCmdDraw(cmd, this->dispatchGroups_.x * 256, 1, 0, 0);
                              }

                              void dispatch(VkCommandBuffer cmd, const ComputeInstance& ins) override {

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

                                        VkMemoryBarrier2 barrier{};
                                        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;

                                        // Compute shader writes to SSBO
                                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;

                                        // Next stage = vertex shader reads SSBO (not vertex input!)
                                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
                                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

                                        BarrierBuilder{}.add(barrier).createBarrier(cmd);
                              }

                              ComputeInstance generate_instance(
                                        ResourcesType& resources,
                                        DescriptorPoolAllocator& globalDescriptorAllocator) override {

                                        this->writer_.clear();

                                        ComputeInstance ins{};
                                        ins.pipeline = &this->defaultComputePipeline_;
                                        ins.computeSet = globalDescriptorAllocator.allocate(this->computeLayout_);

                                        this->writer_.write_buffer(0, resources.particlesIn, resources.bufferSize,
                                                  0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

                                        this->writer_.write_buffer(1, resources.particlesOut, resources.bufferSize,
                                                  0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

                                        this->writer_.update_set(ins.computeSet);
                                        return ins;
                              }

                    private:
                              VkDevice device_;
                              MaterialPipeline pointOpaquePipeline_;
                              VkDescriptorSet globalSceneSet_{};
                              VkDescriptorSetLayout globalSceneLayout_{};

                              std::vector<GraphicPipelineBuilder::ShaderStageDesc> stages{
    GraphicPipelineBuilder::ShaderStageDesc{
        SLANG_SHADER_PATH "SpriteParticleDraw3D.slang.spv", "vertMain", VK_SHADER_STAGE_VERTEX_BIT},
    GraphicPipelineBuilder::ShaderStageDesc{ SLANG_SHADER_PATH "SpriteParticleDraw3D.slang.spv",
                                            "fragMain",
                                            VK_SHADER_STAGE_FRAGMENT_BIT} };
                    };
          }
}

#endif // _POINT_SPRITE_PARTICLE_SYSTEM_HPP_