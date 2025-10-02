#include <Keyboard_Controller.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

void engine::KeyBoardController::movePlaneYXZ(GLFWwindow *win, float dt,
                                              GameObject &obj) {
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
    obj.transform.rotation += lookSpeed * dt * glm::normalize(rotate_camera);
  }

  // RAD
  obj.transform.rotation.x = glm::clamp(
      obj.transform.rotation.x, -glm::half_pi<float>(), glm::half_pi<float>());
  obj.transform.rotation.y =
      glm::mod(obj.transform.rotation.y, glm::two_pi<float>());

  auto rotation_matrix =
      glm::eulerAngleYXZ(obj.transform.rotation.y, obj.transform.rotation.x,
                         obj.transform.rotation.z);
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
    obj.transform.translation +=
        lookSpeed * dt * glm::normalize(move_direction);
  }
}
