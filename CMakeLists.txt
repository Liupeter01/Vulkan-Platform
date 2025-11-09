cmake_minimum_required(VERSION 3.10)
include(FetchContent)
include(cmake/glsl_shader_target.cmake)
include(cmake/slang_shader_target.cmake)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(VulkanBundle LANGUAGES C CXX)

if(PROJECT_BINARY_DIR EQUAL PROJECT_SOURCE_DIR)
  message(
    WARNING "Binary directory of CMake CANNOT be the same as Source Directory!")
endif()

message(STATUS "CMake build type: ${CMAKE_BUILD_TYPE}")

if(MSVC) # 1. fix string literial cannot include unicode characters; 2. rid min
         #   and max macros; 3. get back M_PI, M_E macros
  add_compile_options(
    $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/utf-8$<SEMICOLON>/DNOMINMAX$<SEMICOLON>/D_USE_MATH_DEFINES>
  )
  add_compile_options(
    $<$<COMPILE_LANG_AND_ID:C,MSVC>:/utf-8$<SEMICOLON>/DNOMINMAX$<SEMICOLON>/D_USE_MATH_DEFINES>
  )

  add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
elseif(CMAKE_COMPILER_IS_GNUCXX) # Add necessary checks and warnings
  if(CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]")
    add_compile_options(
      $<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wall$<SEMICOLON>-Wextra>)
    add_compile_options(
      $<$<COMPILE_LANG_AND_ID:CXX,GNU>:-ftrapv$<SEMICOLON>-fbounds-check$<SEMICOLON>-fsanitize-trap>
    )
  endif()
endif()

if(NOT MSVC)
  find_program(CCACHE_PROGRAM ccache)
  if(CCACHE_PROGRAM)
    message(STATUS "Found CCache: ${CCACHE_PROGRAM}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_PROGRAM})
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE_PROGRAM})
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions(ENABLE_VALIDATION_LAYERS=1)
else()
  add_compile_definitions(ENABLE_VALIDATION_LAYERS=0)
endif()

set(GLSL_SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/shaders/glsl)
add_glsl_shader_target(GLSL_Compile_Shader ${GLSL_SHADER_PATH})

set(SLANG_SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/shaders/slang)
add_slang_shader_target(SLANG_Compile_Shader ${SLANG_SHADER_PATH})

find_package(Vulkan REQUIRED)

# GLFW
set(GLFW_BUILD_EXAMPLES
    OFF
    CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS
    OFF
    CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS
    OFF
    CACHE BOOL "" FORCE)
set(GLFW_INSTALL
    OFF
    CACHE BOOL "" FORCE)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4)

FetchContent_MakeAvailable(glm)
FetchContent_MakeAvailable(glfw)

add_subdirectory(external/tinyobjloader)
add_subdirectory(external/VulkanMemoryAllocator)
add_subdirectory(external/spdlog)
add_subdirectory(vulkan-1.3)
add_subdirectory(vulkan-adapt)
