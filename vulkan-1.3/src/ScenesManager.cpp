#include <VulkanEngine.hpp>
#include <nodes/ScenesManager.hpp>

namespace engine {

ScenesManager::ScenesManager(VulkanEngine *eng) : engine_(eng) {}

ScenesManager::~ScenesManager() { destroy(); }

void ScenesManager::init() {
  if (isinit)
    return;
  isinit = true;
}

void ScenesManager::destroy() {
  if (isinit) {
    destroy_scene();
    isinit = false;
  }
}

void ScenesManager::draw(const glm::mat4 &parentMatrix, DrawContext &ctx) {
  for (auto &[scene_name, scene] : loadedScenes_) {
    scene->Draw(parentMatrix, ctx);
  }
}

void ScenesManager::destroy_scene() {
  vkDeviceWaitIdle(engine_->device_);
  loadedScenes_.clear();
}
} // namespace engine
