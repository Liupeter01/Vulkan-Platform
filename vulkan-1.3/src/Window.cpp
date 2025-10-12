#include <Window.hpp>
#include <stdexcept>

engine::Window::Window(const std::string &name, const int width,
                       const int height)
    : name_(name), width_(width), height_(height) {
  initWindow();
}

engine::Window::~Window() {
  glfwDestroyWindow(window_);
  glfwTerminate();
}

bool engine::Window::shouldClose() { return glfwWindowShouldClose(window_); }

VkExtent2D engine::Window::getExtent() const {
  return VkExtent2D{static_cast<unsigned int>(width_),
                    static_cast<unsigned int>(height_)};
}

void engine::Window::resetWindowResizedFlag() { isWindowResized_ = false; }

bool engine::Window::wasWindowResized() const { return isWindowResized_; }

void engine::Window::initWindow() {

  if (!glfwInit())
    throw std::runtime_error("failed to init glfw!");

  if (glfwVulkanSupported() == GLFW_FALSE)
    throw std::runtime_error("glfw does not support vulkan!");

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  window_ = glfwCreateWindow(width_, height_, name_.c_str(), nullptr, nullptr);
  if (!window_) {
    throw std::runtime_error("Failed to CreateWindow");
  }

  glfwSetWindowUserPointer(window_, this);
  glfwSetFramebufferSizeCallback(window_, resizeFrameBufferCallback);
}

void engine::Window::resizeFrameBufferCallback(GLFWwindow *win, int width,
                                               int height) {

  auto window = reinterpret_cast<Window *>(glfwGetWindowUserPointer(win));
  window->width_ = width;
  window->height_ = height;
  window->isWindowResized_ = true;
}

void engine::Window::createWindowSurface(VkInstance ins,
                                         VkSurfaceKHR *surface) {
  if (glfwCreateWindowSurface(ins, window_, nullptr, surface) != VK_SUCCESS) {
    throw std::runtime_error("Failed to CreateWindowSurface");
  }
}
