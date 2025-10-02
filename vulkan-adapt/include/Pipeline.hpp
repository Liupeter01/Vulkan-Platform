#pragma once
#ifndef _PIPELINE_HPP_
#define _PIPELINE_HPP_
#include <Device.hpp>
#include <optional>
#include <vector>

namespace engine {
struct PipelineConf {
  PipelineConf();

  static void enableAlphaBlending(PipelineConf &conf);

  // PipelineConf(const PipelineConf&) = delete;
  // PipelineConf& operator=(const PipelineConf&) = delete;

  std::vector<VkDynamicState> dynamicStateEnables;
  VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
  /*VkPipelineViewportStateCreateInfo viewportStateInfo{};*/
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
  VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
  VkPipelineMultisampleStateCreateInfo multisampleInfo{};

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  VkPipelineColorBlendStateCreateInfo colorBlendInfo{};

  VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};

  VkPipelineLayout pipelineLayout{};
  VkRenderPass renderPass{};
  uint32_t subpass{};
  VkPipeline basePipelineHandle{};
  int32_t basePipelineIndex{};
};

struct Pipeline {
  Pipeline(Pipeline &&) = delete;
  Pipeline &operator=(Pipeline &&) = delete;

  Pipeline(MyEngineDevice &device, const std::string &vertPath,
           const std::string &fragPath, PipelineConf &&conf);

  virtual ~Pipeline();

  [[nodiscard]] std::vector<uint32_t> readSpv(const std::string &path);
  void bind(VkCommandBuffer commandBuffer);

protected:
  void createPipeline(const std::string &vertPath, const std::string &fragPath);
  void loadShader(const std::string &shaderPath, VkShaderModule *shaderModule);

private:
  PipelineConf conf_;
  MyEngineDevice &device_;
  VkPipeline graphicPipeline_;
  VkShaderModule vertShaderModule_;
  VkShaderModule fragShaderModule_;
};
} // namespace engine

#endif //_PIPELINE_HPP_
