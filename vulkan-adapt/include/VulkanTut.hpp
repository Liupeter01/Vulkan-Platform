#pragma once
#ifndef _VULKAN_TUT_HPP_
#define _VULKAN_TUT_HPP_
#include <Descriptors.hpp>
#include <GameObject.hpp>
#include <Model.hpp>
#include <Render.hpp>
#include <Window.hpp>

namespace engine {

struct VulkanTut {
  VulkanTut(VulkanTut &&) = delete;
  VulkanTut &operator=(VulkanTut &&) = delete;

  VulkanTut();
  virtual ~VulkanTut();

public:
  void run();
  static constexpr std::size_t width = 800;
  static constexpr std::size_t height = 600;

protected:
  void loadGameObjects();

private:
  Window window_ = Window("VulkanTut", width, height);
  MyEngineDevice device_{window_};
  Render render_{window_, device_};
  std::unique_ptr<DescriptorPool> globalPool_{}; // must after device_
  std::unordered_map<GameObject::id_t, GameObject> gameObject_;
};
} // namespace engine

#endif //_VULKAN_TUT_HPP_
