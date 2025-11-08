#include <Tools.hpp>
#include <builder/ComputePipelineBuilder.hpp>

namespace engine {

          void ComputePipelineBuilder::clear(){

                    if (computeShader.module != VK_NULL_HANDLE) {
                              vkDestroyShaderModule(device_, computeShader.module, nullptr);
                    }

                    specializationEntries_.clear();
                    specializationData_.clear();
                    specializationInfo_ = {};
                    hasSpecialization_ = false;
          }

          VkPipeline ComputePipelineBuilder::build(){
                    if (pipelineLayout_ == VK_NULL_HANDLE)
                              throw std::runtime_error("ComputePipelineBuilder: layout not set.");

                    if (hasSpecialization_) {
                              specializationInfo_.mapEntryCount = static_cast<uint32_t>(specializationEntries_.size());
                              specializationInfo_.pMapEntries = specializationEntries_.data();
                              specializationInfo_.dataSize = specializationData_.size();
                              specializationInfo_.pData = specializationData_.data();
                              computeShader.pSpecializationInfo = &specializationInfo_;
                    }

                    VkComputePipelineCreateInfo computePipelineCreateInfo{};
                    computePipelineCreateInfo.sType =
                              VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
                    computePipelineCreateInfo.layout = pipelineLayout_;
                    computePipelineCreateInfo.stage = computeShader;

                    if (vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, 
                              &computePipelineCreateInfo, nullptr, &pipeline_) != VK_SUCCESS)
                              throw std::runtime_error("Failed to create compute pipeline.");

                    return pipeline_;
          }

          ComputePipelineBuilder::ComputePipelineBuilder(VkDevice device)
                    :device_(device)
          { }

          ComputePipelineBuilder:: ~ComputePipelineBuilder() {
                    clear();
          }

          ComputePipelineBuilder& ComputePipelineBuilder::set_pipeline_layout(VkPipelineLayout layout) {
                    pipelineLayout_ = layout;
                    return *this;
          }

          ComputePipelineBuilder& ComputePipelineBuilder::set_shaders_stage(const ComputePipelineBuilder::ShaderStageDesc& stage) {

                    computeShader = tools::shader_stage_create_info(
                              device_, stage.path, stage.stage, stage.entry);

                    return *this;
          }

          ComputePipelineBuilder& ComputePipelineBuilder::set_specialization_constant(uint32_t constantID, const void* data, size_t size) {
                    if (!data || size == 0)
                              throw std::runtime_error("ComputePipelineBuilder: invalid specialization constant data.");

                    VkSpecializationMapEntry entry{};
                    entry.constantID = constantID;
                    entry.offset = static_cast<uint32_t>(specializationData_.size());
                    entry.size = size;

                    specializationEntries_.push_back(entry);

                    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data);
                    specializationData_.insert(specializationData_.end(), bytes, bytes + size);

                    hasSpecialization_ = true;
                    return *this;
          }
}