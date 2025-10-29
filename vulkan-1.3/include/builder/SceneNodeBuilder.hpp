#pragma once
#ifndef _SCENE_NODE_BUILDER_HPP_
#define _SCENE_NODE_BUILDER_HPP_
#include <filesystem>
#include <nodes/scene/SceneNode.hpp>

namespace engine {

struct SceneNodeBuilder {

  SceneNodeBuilder(VulkanEngine *eng);
  SceneNodeBuilder &
  set_options(fastgltf::Options option =
                  fastgltf::Options::DontRequireValidAssetMember |
                  fastgltf::Options::AllowDouble |
                  fastgltf::Options::LoadExternalBuffers);

  SceneNodeBuilder &set_filepath(std::filesystem::path filePath);
  SceneNodeBuilder &set_config(const node::SceneNodeConf &conf);
  std::optional<std::shared_ptr<node::SceneNode>> build();

protected:
  static VkFilter extract_filter(fastgltf::Filter filter);
  static VkSamplerMipmapMode extract_mipmap_mode(fastgltf::Filter filter);

  void processSamplers(fastgltf::Asset &gltf);
  void processImages(fastgltf::Asset &gltf);
  void processMaterials(fastgltf::Asset &gltf);
  void processMeshes(fastgltf::Asset &gltf);
  void processNodes(fastgltf::Asset &gltf);
  void processRelation(fastgltf::Asset &gltf);

protected:
  fastgltf::Options options_{};
  std::filesystem::path filepath_{};
  std::optional<node::SceneNodeConf> conf_{};

  // temporal arrays for all the objects to use while creating the GLTF data
  std::vector<std::shared_ptr<MeshAsset>> meshes_;
  std::vector<std::shared_ptr<node::BaseNode>> nodes_;
  std::vector<std::shared_ptr<AllocatedTexture>> images_;
  std::vector<std::shared_ptr<MaterialInstance>> materials_;

private:
  VulkanEngine *engine_{};
  std::shared_ptr<node::SceneNode> scene{};
};
} // namespace engine

#endif //_SCENE_NODE_BUILDER_HPP_
