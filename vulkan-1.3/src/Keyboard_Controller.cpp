#include <interactive/Keyboard_Controller.hpp>
#include <spdlog/spdlog.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace engine {
void KeyBoardController::movePlaneYXZ(GLFWwindow *win, float dt,
                                      std::shared_ptr<node::BaseNode> obj) {

  if (!obj) {
    spdlog::error(
        "[KeyBoardController movePlaneYXZ]: Invalid BaseNode Deteced!");
    throw std::runtime_error("Invalid BaseNode");
  }

  glm::vec3 rotate_camera{}; //
  if (glfwGetKey(win, keys.lookRight) == GLFW_PRESS)
    rotate_camera.y += 1.f;
  if (glfwGetKey(win, keys.lookLeft) == GLFW_PRESS)
    rotate_camera.y -= 1.f;
  if (glfwGetKey(win, keys.lookUp) == GLFW_PRESS)
    rotate_camera.x -= 1.f;
  if (glfwGetKey(win, keys.lookDown) == GLFW_PRESS)
    rotate_camera.x += 1.f;

  if (glm::length(rotate_camera) > std::numeric_limits<float>::epsilon()) {
    obj->localTransform.rotation +=
        lookSpeed * dt * glm::normalize(rotate_camera);
  }

  // RAD
  obj->localTransform.rotation.x =
      glm::clamp(obj->localTransform.rotation.x, -glm::half_pi<float>(),
                 glm::half_pi<float>());
  obj->localTransform.rotation.y =
      glm::mod(obj->localTransform.rotation.y, glm::two_pi<float>());

  auto rotation_matrix = glm::eulerAngleYXZ(obj->localTransform.rotation.y,
                                            obj->localTransform.rotation.x,
                                            obj->localTransform.rotation.z);
  glm::vec3 right = glm::vec3(rotation_matrix[0]);   // X
  glm::vec3 up = glm::vec3(rotation_matrix[1]);      // Y
  glm::vec3 forward = glm::vec3(rotation_matrix[2]); // Z

  glm::vec3 move_direction{};
  if (glfwGetKey(win, keys.moveForward) == GLFW_PRESS)
    move_direction += forward;
  if (glfwGetKey(win, keys.moveBackward) == GLFW_PRESS)
    move_direction -= forward;
  if (glfwGetKey(win, keys.moveRight) == GLFW_PRESS)
    move_direction += right;
  if (glfwGetKey(win, keys.moveLeft) == GLFW_PRESS)
    move_direction -= right;
  if (glfwGetKey(win, keys.moveUp) == GLFW_PRESS)
    move_direction += up;
  if (glfwGetKey(win, keys.moveDown) == GLFW_PRESS)
    move_direction -= up;

  if (glm::length(move_direction) > std::numeric_limits<float>::epsilon()) {
    obj->localTransform.translation +=
        moveSpeed * dt * glm::normalize(move_direction);
  }
}
} // namespace engine
