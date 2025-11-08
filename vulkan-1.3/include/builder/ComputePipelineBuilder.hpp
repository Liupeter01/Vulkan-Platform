#pragma once
#ifndef _COMPUTE_PIPELINE_BUILDER_HPP_
#define _COMPUTE_PIPELINE_BUILDER_HPP_
#include <vector>
#include <vulkan/vulkan.hpp>

namespace engine {

          struct ComputePipelineBuilder {

                    struct ShaderStageDesc {
                              std::string path;
                              std::string entry;
                              VkShaderStageFlagBits stage;
                    };

                    ComputePipelineBuilder(VkDevice device);
                    virtual ~ComputePipelineBuilder();

                    void clear();
                    VkPipeline build();

                    ComputePipelineBuilder& set_shaders_stage(const ShaderStageDesc& stage);
                    ComputePipelineBuilder& set_pipeline_layout(VkPipelineLayout layout);
                    ComputePipelineBuilder& set_specialization_constant(uint32_t constantID, const void* data, size_t size);

          private:
                    VkDevice device_{};
                    VkPipeline pipeline_{};
                    VkPipelineLayout pipelineLayout_{};
                    VkPipelineShaderStageCreateInfo computeShader;

                    std::vector<VkSpecializationMapEntry> specializationEntries_;
                    std::vector<uint8_t> specializationData_;
                    VkSpecializationInfo specializationInfo_{};
                    bool hasSpecialization_ = false;
          };
}

#endif //_COMPUTE_PIPELINE_BUILDER_HPP_