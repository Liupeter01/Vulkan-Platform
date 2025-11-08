cmake_minimum_required(VERSION 3.10)

# ==========================================
# Compile Slang Shaders to SPIR-V Usage: add_slang_shader_target(MySlangShaders
# PathName)
# ==========================================
function(add_slang_shader_target TARGET SHADER_DIR)

  if(NOT SLANGC_EXECUTABLE)

    if(APPLE)
      set(SLANG_LIB_PATH /opt/homebrew/lib)
      set(SLANG_SEARCH_PATHS
          /opt/homebrew/bin # macOS M1 Homebrew
          /usr/local/bin # Intel macOS
          $ENV{HOME}/.local/share/nvim/mason/packages/slang/bin /usr/bin)

      if(EXISTS "$ENV{HOME}/.local/share/nvim/mason/packages/slang/lib")
        set(SLANG_LIB_PATH
            "$ENV{HOME}/.local/share/nvim/mason/packages/slang/lib")
      endif()
      set(CMAKE_BUILD_RPATH "${CMAKE_BUILD_RPATH};${SLANG_LIB_PATH}")
      message(STATUS "Added Slang runtime lib path: ${SLANG_LIB_PATH}")

      find_program(
        SLANGC_EXECUTABLE
        NAMES slangc
        PATHS ${SLANG_SEARCH_PATHS} REQUIRED
        NO_DEFAULT_PATH)
    else()
      find_program(SLANGC_EXECUTABLE slangc REQUIRED)
    endif()

    if(SLANGC_EXECUTABLE)
      message(STATUS "Found Slang compiler: ${SLANGC_EXECUTABLE}")
    else()
      message(
        FATAL_ERROR "slangc not found! Please install via Homebrew or Mason.")
    endif()
  endif()

  file(GLOB SLANG_SOURCES "${SHADER_DIR}/*.slang")

  set(SLANG_SHADER_OUTPUTS "")
  foreach(SLANG_SHADER ${SLANG_SOURCES})
    get_filename_component(SLANG_SHADER_NAME ${SLANG_SHADER} NAME)
    set(SLANG_SPIRV ${SLANG_SHADER}.spv)

    add_custom_command(
      OUTPUT ${SLANG_SPIRV}
      COMMAND
        ${SLANGC_EXECUTABLE} ${SLANG_SHADER} -target spirv -profile spirv_1_4
        -emit-spirv-directly -fvk-use-entrypoint-name -entry vertMain -entry
        fragMain -o ${SLANG_SPIRV}
      DEPENDS ${SLANG_SHADER}
      COMMENT "Compiling Slang Shader: ${SLANG_SHADER_NAME} ¡ú ${SLANG_SPIRV}")
    list(APPEND SLANG_SHADER_OUTPUTS ${SLANG_SPIRV})
  endforeach()

  add_custom_target(${TARGET} ALL DEPENDS ${SLANG_SHADER_OUTPUTS})
endfunction()
