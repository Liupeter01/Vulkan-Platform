#pragma once
#ifndef _MATERIAL_PIPELINE_HPP_
#define _MATERIAL_PIPELINE_HPP_
#include <builder/GraphicPipelineBuilder.hpp>
#include <material/GLTFPBR_Material.hpp>
#include <mesh/MeshBuffers.hpp>
#include <spdlog/spdlog.h>

namespace engine {
enum class MaterialPass;

struct MaterialPipeline {
  MaterialPipeline(VkDevice device);
  virtual ~MaterialPipeline();

  void set_vertex_shader(const std::string &path, const std::string &entry);
  void set_fragment_shader(const std::string &path, const std::string &entry);

  template <typename _Ty = GPUGeoPushConstants>
  void create(MaterialPass pass,
              const std::vector<VkDescriptorSetLayout> &layouts) {
    if (isinit_)
      return;
    if (pass == MaterialPass::UNDEFINED) {
      throw std::runtime_error("Undefined MaterialPass!");
    }

    static_assert(sizeof(_Ty) <= 128,
                  "Push constant size exceeds Vulkan 128-byte limit!");

    if (isUsingDefaultShader) {
      spdlog::info("[MaterialPipeline info]: MaterialPass {}, Using default "
                   "Vertex Shader settings.",
                   static_cast<int>(pass));
      spdlog::info("[MaterialPipeline info]: MaterialPass {}, Using default "
                   "Fragment Shader settings.",
                   static_cast<int>(pass));
    }

    VkPushConstantRange matrixRange{};
    matrixRange.offset = 0;
    matrixRange.size = sizeof(_Ty);
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo graphicLayout{};
    graphicLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    graphicLayout.pushConstantRangeCount = 1;
    graphicLayout.pPushConstantRanges = &matrixRange;
    graphicLayout.setLayoutCount = static_cast<uint32_t>(layouts.size());
    graphicLayout.pSetLayouts = layouts.data();
    vkCreatePipelineLayout(device_, &graphicLayout, nullptr, &pipelineLayout_);

    GraphicPipelineBuilder builder{device_};
    builder.pipelineLayout_ = pipelineLayout_;
    builder.set_shaders_stages(stages)
        .set_polygon_mode(VK_POLYGON_MODE_FILL)
        .set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
        .set_multisampling()
        .set_depthtest(VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL)
        .set_depth_format(VK_FORMAT_D32_SFLOAT)
        .set_color_attachment_format(VK_FORMAT_R16G16B16A16_SFLOAT);

    if (pass == MaterialPass::OPAQUE) {
      builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
      create_opaque_pipeline(builder);
    } else if (pass == MaterialPass::TRANSPARENT) {
      builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
      create_transparent_pipeline(builder);
    } else if (pass == MaterialPass::OPAQUE_POINT) {
      builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
      create_opaque_point_pipeline(builder);
    }

    isinit_ = true;
  }

  void destroy();

  VkPipeline getPipeline() { return pipeline_; }
  VkPipelineLayout getPipelineLayout() { return pipelineLayout_; }

protected:
  std::vector<GraphicPipelineBuilder::ShaderStageDesc> stages{
      GraphicPipelineBuilder::ShaderStageDesc{
          GLSL_SHADER_PATH "pbr.vert.spv", "main", VK_SHADER_STAGE_VERTEX_BIT},
      GraphicPipelineBuilder::ShaderStageDesc{GLSL_SHADER_PATH "pbr.frag.spv",
                                              "main",
                                              VK_SHADER_STAGE_FRAGMENT_BIT}};

  void create_opaque_pipeline(GraphicPipelineBuilder &builder);
  void create_transparent_pipeline(GraphicPipelineBuilder &builder);
  void create_opaque_point_pipeline(GraphicPipelineBuilder &builder);

private:
  bool isinit_ = false;
  bool isUsingDefaultShader = true;

  VkDevice device_ = VK_NULL_HANDLE;
  VkPipeline pipeline_ = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
};
} // namespace engine

#endif //_MATERIAL_PIPELINE_HPP_
