#pragma once
#ifndef _KEYBOARD_CONTROLLER_HPP_
#define _KEYBOARD_CONTROLLER_HPP_
#include <nodes/BaseNode.hpp>
#include <Window.hpp>

namespace engine {
struct KeyBoardController {
public:
  struct KeyMappings {
    int moveLeft = GLFW_KEY_A;
    int moveRight = GLFW_KEY_D;
    int moveForward = GLFW_KEY_W;
    int moveBackward = GLFW_KEY_S;
    int moveUp = GLFW_KEY_E;
    int moveDown = GLFW_KEY_Q;
    int lookLeft = GLFW_KEY_LEFT;
    int lookRight = GLFW_KEY_RIGHT;
    int lookUp = GLFW_KEY_UP;
    int lookDown = GLFW_KEY_DOWN;
  } keys;

  float moveSpeed = {3.f};
  float lookSpeed = {1.5f};

  void movePlaneYXZ(GLFWwindow *win, float dt, std::shared_ptr<node::BaseNode> &obj);
};
} // namespace engine

#endif //_KEYBOARD_CONTROLLER_HPP_
