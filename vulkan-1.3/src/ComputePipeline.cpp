#include <compute/ComputePipeline.hpp>
#include <spdlog/spdlog.h>

namespace engine {
ComputePipeline::ComputePipeline(VkDevice device) : device_(device) {}

ComputePipeline::~ComputePipeline() { destroy(); }

void ComputePipeline::set_compute_shader(const std::string &path,
                                         const std::string &entry) {
  if (path.empty() || entry.empty()) {
    spdlog::warn("[ComputePipeline Warn]: Invalid Compute Shader path or "
                 "entry, using default settings!");
    return;
  }

  compute_stage.path = path;
  compute_stage.entry = entry;
  isUsingDefaultShader = false;
}

void ComputePipeline::destroy() {
  if (isinit_) {
    vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
    vkDestroyPipeline(device_, pipeline_, nullptr);
    isinit_ = false;
  }
}

void ComputePipeline::create_default_pipeline(ComputePipelineBuilder &builder) {
  pipeline_ = builder.build();
}

void ComputePipeline::create_specialize_constant_pipeline(
    ComputePipelineBuilder &builder) {
  // pipeline_ = builder.set_specialization_constant().build();
  if (specializationConstants_.empty()) {
    spdlog::warn("[ComputePipeline Warn]: No specialization constants set, "
                 "falling back to default pipeline.");
    create_default_pipeline(builder);
    return;
  }

  for (auto &spec : specializationConstants_) {
    builder.set_specialization_constant(spec.id, spec.data.data(),
                                        spec.data.size());
  }

  spdlog::info("[ComputePipeline info]: Building compute pipeline with {} "
               "specialization constants.",
               specializationConstants_.size());

  pipeline_ = builder.build();
}
} // namespace engine
