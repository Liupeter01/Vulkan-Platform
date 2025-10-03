cmake_minimum_required(VERSION 3.10)
project(Vulkan13 LANGUAGES C CXX)

if(PROJECT_BINARY_DIR EQUAL PROJECT_SOURCE_DIR)
  message(
    WARNING "Binary directory of CMake CANNOT be the same as Source Directory!")
endif()

file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS include/*.h include/*.hpp)

  FetchContent_Declare(
    fetch_vk_bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
    GIT_TAG        v1.4.327 #suggest using a tag so the library doesn't update whenever new commits are pushed to a branch
)

FetchContent_MakeAvailable(fetch_vk_bootstrap)

add_executable(${PROJECT_NAME} ${sources} ${headers} ${SHADER_OUTPUTS})
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw glm Vulkan::Vulkan
                                              tinyobjloader vk-bootstrap::vk-bootstrap VulkanMemoryAllocator)

target_compile_definitions(${PROJECT_NAME}
    PUBLIC CONFIG_HOME="${CMAKE_SOURCE_DIR}/"
)