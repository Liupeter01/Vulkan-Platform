#include <mesh/MeshLoader.hpp>
#include <nodes/MeshNode.hpp>
#include <nodes/NodeManager.hpp>
#include <spdlog/spdlog.h>

namespace engine {
namespace node {
NodeManager::~NodeManager() {}

void NodeManager::init(const std::string &root_name) {
  if (isinit)
    return;

  // Allocate root memory
  create_root(root_name);

  isinit = true;
}

void NodeManager::destroy() {
  if (isinit) {
    remove_nodes(scene_root_);
    scene_root_.reset();
    lookup_mesh.clear();
    lookup_path.clear();
    isinit = false;
  }
}

void NodeManager::create_root(const std::string &root_name) {
  if (scene_root_) {
    spdlog::warn("[Scene Warn]: Replacing existing root node '{}'",
                 scene_root_->node_name);
    remove_nodes(scene_root_);
  }

  scene_root_ = std::make_shared<node::BaseNode>();
  scene_root_->node_name = root_name;
  scene_root_->node_path = root_name;

  lookup_path.clear();
  lookup_path.try_emplace(root_name, scene_root_);

  spdlog::info("[Scene Info]: Scene root node created as '{}'", root_name);
}

// remove all tree recursively!
void NodeManager::remove_nodes(std::shared_ptr<node::BaseNode> root) {
  if (!root)
    return;
  for (auto &child : root->children) {
    remove_nodes(child);
  }
  root->children.clear();
  root.reset();
}

void NodeManager::draw(const glm::mat4 &parentMatrix, DrawContext &ctx) {
  scene_root_->Draw(parentMatrix, ctx);
}

bool NodeManager::attachChildren(const std::string &parentName,
                                 std::shared_ptr<MeshAsset> asset) {
  spdlog::debug(
      "[Scene Info] Attaching single MeshAsset child '{}' to parent '{}'",
      asset->meshName, parentName);
  return attachChildrens(parentName, {asset});
}

bool NodeManager::attachChildren(const std::string &parentName,
                                 std::shared_ptr<node::BaseNode> child) {
  spdlog::debug(
      "[Scene Info] Attaching single BaseNode child '{}' to parent '{}'",
      child->node_name, parentName);
  return attachChildrens(parentName, {child});
}

bool NodeManager::attachChildrens(
    const std::string &parentName,
    const std::vector<std::shared_ptr<node::BaseNode>> &childrens) {

  auto parentNode = findNode(parentName);
  if (!parentNode.has_value()) {
    spdlog::error("[Scene Error]: ParentNode Not Found!");
    return false;
  }

  bool status = true;
  for (auto &node : childrens) {
    if (!node) {
      spdlog::warn("[Scene Warn]: Nullptr child detected under parent {}",
                   parentName);
      continue;
    }

    // it's inside insert function: parentNode->children.push_back(child);
    if (!insert(parentNode.value(), node)) {
      spdlog::warn("[Scene Warn]: Failed to attach child node: {}",
                   node->node_path);
      status = false;
    }
  }
  return status;
}

bool NodeManager::attachChildrens(
    const std::string &parentName,
    const std::vector<std::shared_ptr<MeshAsset>> &childrens) {

  auto parentNode = findNode(parentName);
  if (!parentNode.has_value()) {
    spdlog::error("[Scene Error]: ParentNode Not Found!");
    return false;
  }

  bool status = true;
  for (auto &node : childrens) {
    if (!node) {
      spdlog::warn("[Scene Warn]: Nullptr child detected under parent {}",
                   parentName);
      continue;
    }

    auto [_, inserted] = lookup_mesh.try_emplace(node->meshName, node);
    if (!inserted)
      spdlog::warn("[Scene Warn]: Mesh: {} Already Exist!", node->meshName);

    auto child = std::make_shared<node::MeshNode>(parentNode.value());
    child->mesh_ = node;
    child->node_name = node->meshName;

    // it's inside insert function: parentNode->children.push_back(child);
    if (!insert(parentNode.value(), child)) {
      spdlog::warn("[Scene Warn]: Failed to attach child node: {}",
                   child->node_path);
      status = false;
    }
  }
  return status;
}

std::optional<std::shared_ptr<node::BaseNode>>
NodeManager::findNode(const std::string &name) const {
  auto it = lookup_path.find(name);
  if (it != lookup_path.end())
    return it->second;
  return std::nullopt;
}

std::optional<std::shared_ptr<MeshAsset>>
NodeManager::findMesh(const std::string &name) const {
  auto it = lookup_mesh.find(name);
  if (it != lookup_mesh.end())
    return it->second;
  return std::nullopt;
}

bool NodeManager::insert_to_lookup(std::shared_ptr<node::BaseNode> node,
                                   const std::string &parentPath) {
  std::string fullPath = fmt::format("{}/{}", parentPath, node->node_name);
  node->node_path = fullPath;

  auto [_, inserted] = lookup_path.try_emplace(fullPath, node);
  if (!inserted)
    return false;

  for (auto &child : node->children)
    insert_to_lookup(child, fullPath);
  return true;
}

bool NodeManager::insert(std::shared_ptr<node::BaseNode> parent,
                         std::shared_ptr<node::BaseNode> child) {
  if (!parent || !child) {
    spdlog::warn("[WARN]: Invalid Parent & Child Node!");
    return false;
  }

  parent->children.push_back(child);
  child->parent_ = parent;

  if (!insert_to_lookup(child, parent->node_path)) {
    spdlog::warn("[Scene Warn]: Node {} already exists in lookup.",
                 child->node_path);
    return false;
  }
  return true;
}
} // namespace node
} // namespace engine
