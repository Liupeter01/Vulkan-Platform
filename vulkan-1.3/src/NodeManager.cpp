#include <nodes/mesh/MeshNode.hpp>
#include <nodes/scene/NodeManager.hpp>
#include <spdlog/spdlog.h>

namespace engine {

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
    remove_nodes(sceneRoot_);
    sceneRoot_.reset();
    meshes_.clear();
    nodes_.clear();
    isinit = false;
  }
}

void NodeManager::create_root(const std::string &root_name) {
  if (sceneRoot_) {
    spdlog::warn("[NodeManager Info]: Replacing existing root node '{}'",
                 sceneRoot_->node_name);
    remove_nodes(sceneRoot_);
  }

  sceneRoot_ = std::make_shared<node::BaseNode>();
  sceneRoot_->node_name = root_name;
  sceneRoot_->node_path = root_name;

  nodes_.clear();
  auto [_, status] = nodes_.try_emplace(root_name, sceneRoot_);
  if (status) {
    spdlog::info("[NodeManager Info]: Scene root node created as '{}'",
                 root_name);
  } else {
    spdlog::error("[NodeManager Error]: Scene root node created FAILED!");
    throw std::runtime_error("Create Scene Root Failed!");
  }
}

bool NodeManager::insert_to_lookup_table(std::shared_ptr<node::BaseNode> &node,
                                         std::string_view parentPath) {

  if (node != sceneRoot_) {
    std::string fullPath = fmt::format("{}/{}", parentPath, node->node_name);
    node->node_path = fullPath;

    if (!nodes_.try_emplace(fullPath, node).second)
      return false;
  }

  for (auto &child : node->children) {
    if (!insert_to_lookup_table(child, node->node_path)) {
      spdlog::warn("[NodePackedCreator Warn]: Emplace "
                   "std::shared_ptr<BaseNode> to unordered_map Already Exist!");
    }
  }
  return true;
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

void NodeManager::Draw(const glm::mat4 &topMatrix, DrawContext &ctx) {
  sceneRoot_->Draw(topMatrix, ctx);
}

bool NodeManager::attachChildren(const std::string &parentName,
                                 std::shared_ptr<mesh::MeshAsset> asset) {
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
    const std::vector<std::shared_ptr<mesh::MeshAsset>> &childrens) {

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

    auto [_, inserted] = meshes_.try_emplace(node->meshName, node);
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
  auto it = nodes_.find(name);
  if (it != nodes_.end())
    return it->second;
  return std::nullopt;
}

std::optional<std::shared_ptr<mesh::MeshAsset>>
NodeManager::findMesh(const std::string &name) const {
  auto it = meshes_.find(name);
  if (it != meshes_.end())
    return it->second;
  return std::nullopt;
}

bool NodeManager::insert(std::shared_ptr<node::BaseNode> parent,
                         std::shared_ptr<node::BaseNode> child) {
  if (!parent || !child) {
    spdlog::warn("[WARN]: Invalid Parent & Child Node!");
    return false;
  }

  parent->children.push_back(child);
  child->parent_ = parent;

  if (!insert_to_lookup_table(child, parent->node_path)) {
    spdlog::warn("[Scene Warn]: Node {} already exists in lookup.",
                 child->node_path);
    return false;
  }
  return true;
}
} // namespace engine
