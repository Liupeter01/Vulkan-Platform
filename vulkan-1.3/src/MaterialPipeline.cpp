#include <material/GLTFPBR_Material.hpp>
#include <material/MaterialPipeline.hpp>
#include <mesh/MeshBuffers.hpp>
#include <spdlog/spdlog.h>

namespace engine {
          MaterialPipeline::MaterialPipeline(VkDevice device) 
                    : device_(device), isUsingDefaultShader(true)
          { }

MaterialPipeline::~MaterialPipeline() { destroy(); }

void MaterialPipeline::set_vertex_shader(const std::string& path, const std::string& entry) {
          if (path.empty() || entry.empty()) {
                    spdlog::warn("[MaterialPipeline Warn]: Invalid Vertex Shader path or entry, using default settings!");
                    return;
          }

          stages[0].path = path;
          stages[0].entry = entry;
          isUsingDefaultShader = false;
}

void MaterialPipeline::set_fragment_shader(const std::string& path, const std::string& entry){

          if (path.empty() || entry.empty()) {
                    spdlog::warn("[MaterialPipeline Warn]: Invalid Fragment Shader path or entry, using default settings!");
                    return;
          }

          stages[1].path = path;
          stages[1].entry = entry;
          isUsingDefaultShader = false;
}

void MaterialPipeline::create(
    MaterialPass pass, const std::vector<VkDescriptorSetLayout> &layouts) {
  if (isinit_)
    return;
  if (pass == MaterialPass::UNDEFINED) {
    throw std::runtime_error("Undefined MaterialPass!");
  }

  if (isUsingDefaultShader) {
            spdlog::info("[MaterialPipeline info]: MaterialPass {}, Using default Vertex Shader settings.", static_cast<int>(pass));
            spdlog::info("[MaterialPipeline info]: MaterialPass {}, Using default Fragment Shader settings.", static_cast<int>(pass));
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
      .set_shaders_stages(stages)
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
