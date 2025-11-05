cmake_minimum_required(VERSION 3.10)
project(VulkanAdapt LANGUAGES C CXX)

if(PROJECT_BINARY_DIR EQUAL PROJECT_SOURCE_DIR)
  message(
    WARNING "Binary directory of CMake CANNOT be the same as Source Directory!")
endif()

file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS include/*.h include/*.hpp)

add_executable(${PROJECT_NAME} ${sources} ${headers})
add_dependencies(${PROJECT_NAME} GLSL_Compile_Shader)
add_dependencies(${PROJECT_NAME} SLANG_Compile_Shader)

target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw glm Vulkan::Vulkan
                                              tinyobjloader)

target_compile_definitions(${PROJECT_NAME}
                           PUBLIC CONFIG_HOME="${CMAKE_SOURCE_DIR}/")
                           
target_compile_definitions(${PROJECT_NAME}
                           PUBLIC GLSL_SHADER_PATH="${GLSL_SHADER_PATH}/")

target_compile_definitions(${PROJECT_NAME}
                           PUBLIC SLANG_SHADER_PATH="${SLANG_SHADER_PATH}/")