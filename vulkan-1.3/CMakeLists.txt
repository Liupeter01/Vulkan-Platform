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
  GIT_TAG v1.4.327 # suggest using a tag so the library doesn't update whenever
                   # new commits are pushed to a branch
)

FetchContent_MakeAvailable(fetch_vk_bootstrap)

if(CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]")
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.92.3
    )
    FetchContent_MakeAvailable(imgui)

    file(GLOB IMGUI_SOURCES CONFIGURE_DEPENDS
        ${imgui_SOURCE_DIR}/imgui*.cpp
    )

    list(APPEND IMGUI_SOURCES
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
    )

    add_library(imgui STATIC ${IMGUI_SOURCES})
    target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )

    target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)

endif()

add_executable(${PROJECT_NAME} ${sources} ${header})
add_dependencies(${PROJECT_NAME} Compile_Shader)

target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(
  ${PROJECT_NAME} PUBLIC glfw glm Vulkan::Vulkan tinyobjloader
                          vk-bootstrap::vk-bootstrap VulkanMemoryAllocator)

if(TARGET imgui)
    target_link_libraries(${PROJECT_NAME} PRIVATE imgui)
endif()

target_compile_definitions(${PROJECT_NAME}
                           PUBLIC CONFIG_HOME="${CMAKE_SOURCE_DIR}/")
