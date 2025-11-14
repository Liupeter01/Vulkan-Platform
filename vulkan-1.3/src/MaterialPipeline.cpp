#include <material/MaterialPipeline.hpp>

namespace engine {
MaterialPipeline::MaterialPipeline(VkDevice device)
    : device_(device), isUsingDefaultShader(true) {}

MaterialPipeline::~MaterialPipeline() { destroy(); }

void MaterialPipeline::set_vertex_shader(const std::string &path,
                                         const std::string &entry) {
  if (path.empty() || entry.empty()) {
    spdlog::warn("[MaterialPipeline Warn]: Invalid Vertex Shader path or "
                 "entry, using default settings!");
    return;
  }

  stages[0].path = path;
  stages[0].entry = entry;
  isUsingDefaultShader = false;
}

void MaterialPipeline::set_fragment_shader(const std::string &path,
                                           const std::string &entry) {

  if (path.empty() || entry.empty()) {
    spdlog::warn("[MaterialPipeline Warn]: Invalid Fragment Shader path or "
                 "entry, using default settings!");
    return;
  }

  stages[1].path = path;
  stages[1].entry = entry;
  isUsingDefaultShader = false;
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

void MaterialPipeline::create_opaque_point_pipeline(GraphicPipelineBuilder& builder) {
          pipeline_ = builder.disable_blending().build();
}

} // namespace engine
