#include <VulkanEngine.hpp>
#include <iostream>

int main() {

  engine::Window win{std::string("Vulkan-1.3"), 800, 600};
  engine::VulkanEngine engine{win};
  engine.run();

  return 0;
}
