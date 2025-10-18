#include <material/GLTFPBR_Material.hpp>
#include <material/MaterialPipeline.hpp>
#include <mesh/MeshBuffers.hpp>

namespace engine {
MaterialPipeline::MaterialPipeline(VkDevice device) : device_(device) {}

MaterialPipeline::~MaterialPipeline() { destroy(); }

void MaterialPipeline::create(
    MaterialPass pass, const std::vector<VkDescriptorSetLayout> &layouts) {
  if (isinit_)
    return;
  if (pass == MaterialPass::UNDEFINED) {
    throw std::runtime_error("Undefined MaterialPass!");
  }

  VkPushConstantRange matrixRange{};
  matrixRange.offset = 0;
  matrixRange.size = sizeof(GPUGeoPushConstants);
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
  builder
      .set_shaders(CONFIG_HOME "shaders/pbr.vert.spv",
                   CONFIG_HOME "shaders/pbr.frag.spv")
      .set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
      .set_polygon_mode(VK_POLYGON_MODE_FILL)
      .set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
      .set_multisampling()
      .set_depthtest(VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL)
      .set_depth_format(VK_FORMAT_D32_SFLOAT)
      .set_color_attachment_format(VK_FORMAT_R16G16B16A16_SFLOAT);

  if (pass == MaterialPass::OPAQUE) {
    create_opaque_pipeline(builder);
  } else if (pass == MaterialPass::TRANSPARENT) {
    create_transparent_pipeline(builder);
  }

  vkDestroyShaderModule(device_, builder.shaderStages_[0].module, nullptr);
  vkDestroyShaderModule(device_, builder.shaderStages_[1].module, nullptr);

  isinit_ = true;
}

void MaterialPipeline::destroy() {
  if (isinit_) {
    vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
    vkDestroyPipeline(device_, pipeline_, nullptr);
    isinit_ = false;
  }
}

void MaterialPipeline::create_opaque_pipeline(GraphicPipelineBuilder &builder) {
  pipeline_ = builder.disable_blending().build();
}

void MaterialPipeline::create_transparent_pipeline(
    GraphicPipelineBuilder &builder) {
  pipeline_ = builder.set_blending_alphablend(true).build();
}
} // namespace engine
