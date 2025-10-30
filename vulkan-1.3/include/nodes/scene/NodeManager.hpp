#pragma once
#ifndef _NODE_MANAGER_HPP_
#define _NODE_MANAGER_HPP_
#include <memory>
#include <mesh/MeshLoader.hpp>
#include <nodes/core/BaseNode.hpp>
#include <optional>
#include <string>
#include <unordered_map>

namespace engine {

struct NodeManager : public node::IRenderable {
          std::string name;
  virtual ~NodeManager();

  void init(const std::string &root_name = "/root");
  void destroy();
  void Draw(const glm::mat4 &topMatrix, DrawContext &ctx) override;

  // CRUD
  bool attachChildren(const std::string &parentName,
                      std::shared_ptr<MeshAsset> asset);
  bool attachChildren(const std::string &parentName,
                      std::shared_ptr<node::BaseNode> child);
  bool attachChildrens(
      const std::string &parentName,
      const std::vector<std::shared_ptr<node::BaseNode>> &childrens);
  bool
  attachChildrens(const std::string &parentName,
                  const std::vector<std::shared_ptr<MeshAsset>> &childrens);

  [[nodiscard]] std::optional<std::shared_ptr<node::BaseNode>>
  findNode(const std::string &name) const;
  [[nodiscard]] std::optional<std::shared_ptr<MeshAsset>>
  findMesh(const std::string &name) const;

protected:
  // Node Query System(automatically recurse all the children nodes)
  bool insert(std::shared_ptr<node::BaseNode> parent,
              std::shared_ptr<node::BaseNode> child); // push_back +

  bool insert_to_lookup_table(std::shared_ptr<node::BaseNode> &node,
                              std::string_view parentPath);

  void create_root(const std::string &root_name = "/root");

  // remove all tree recursively!
  void remove_nodes(std::shared_ptr<node::BaseNode> root);

protected:
  std::shared_ptr<node::BaseNode> sceneRoot_ = nullptr;
  std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes_;
  std::unordered_map<std::string, std::shared_ptr<node::BaseNode>> nodes_;

private:
  bool isinit = false;
};
} // namespace engine

#endif //_NODE_MANAGER_HPP_
