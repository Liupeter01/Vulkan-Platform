#pragma once
#ifndef _SCENES_NODES_MANAGER_HPP_
#define _SCENES_NODES_MANAGER_HPP_
#include <memory>
#include <nodes/scene/SceneNode.hpp>
#include <optional>
#include <string>
#include <unordered_map>

namespace engine {
class ScenesManager {
public:
  ScenesManager(VulkanEngine *eng);
  virtual ~ScenesManager();

public:
  void init();
  void destroy();
  void draw(const glm::mat4 &parentMatrix, DrawContext &ctx);

protected:
  void destroy_scene();

private:
  bool isinit = false;
  VulkanEngine *engine_{};
  std::unordered_map<
      /*scene name = */ std::string,
      /*scene nodes = */ std::shared_ptr<NodeManager>>
      loadedScenes_;
};

} // namespace engine

#endif //_SCENES_NODES_MANAGER_HPP_
