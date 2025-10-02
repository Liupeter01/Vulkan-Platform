#pragma once
#ifndef _WINDOW_HPP_
#define _WINDOW_HPP_

#include <memory>
#include <stdexcept>
#include <string>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace engine {
struct Window {
public:
  Window(Window &&) = delete;
  Window &operator=(Window &&) = delete;

  Window(const std::string &name, const int width, const int height);
  virtual ~Window();

public:
  bool shouldClose();
  VkExtent2D getExtent() const;
  void resetWindowResizedFlag();
  bool wasWindowResized() const;
  void createWindowSurface(VkInstance ins, VkSurfaceKHR *surface);
  GLFWwindow *getGLFWWindow() const { return window_; }

protected:
  void initWindow();
  static void resizeFrameBufferCallback(GLFWwindow *window, int width,
                                        int height);

private:
  const std::string name_;
  bool isWindowResized_ = false;
  int width_;
  int height_;
  GLFWwindow *window_{nullptr};
};
} // namespace engine

#endif //_WINDOW_HPP_
