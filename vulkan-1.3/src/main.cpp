#include <VulkanEngine.hpp>
#include <exception>
#include <iostream>

int main() {
  try {

    engine::Window win{std::string("Vulkan-1.3"), 800, 600};
    engine::VulkanEngine engine{win};
    engine.run();

  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
  }

  return 0;
}
