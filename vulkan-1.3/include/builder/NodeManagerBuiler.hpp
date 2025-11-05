#pragma once
#ifndef _NODE_MANAGER_BUILDER_HPP_
#define _NODE_MANAGER_BUILDER_HPP_
#include <fastgltf/core.hpp>
#include <filesystem>
#include <memory>
#include <nodes/scene/NodeManager.hpp>
#include <optional>
#include <string>
// #include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

namespace engine {
class VulkanEngine;

struct NodeManagerBuilder {
  NodeManagerBuilder(VulkanEngine *eng);
  NodeManagerBuilder &set_options(
      fastgltf::Options option = fastgltf::Options::LoadExternalBuffers);
  NodeManagerBuilder &set_filepath(std::filesystem::path filePath);
  NodeManagerBuilder &set_root(const std::string &root = "/root");
  NodeManagerBuilder &set_debug_color(bool status = true);
  std::optional<std::shared_ptr<NodeManager>> build();

protected:
  void processMeshes(fastgltf::Asset &gltf);
  bool enableDebugColor{true};
  std::string root_name{};
  fastgltf::Options options_{};
  std::filesystem::path filepath_{};

private:
  std::shared_ptr<NodeManager> nodeMgr_;
  std::vector<std::shared_ptr<MeshAsset>> assets_;
  VulkanEngine *engine_{};
};
} // namespace engine

#endif
