cmake_minimum_required(VERSION 3.10)

# ==========================================
# Compile GLSL Shaders to SPIR-V
# Usage:
#   add_glsl_shader_target(MyGLSLShaders PathName)
# ==========================================
function(add_glsl_shader_target TARGET SHADER_DIR)
  if(NOT GLSLC_EXECUTABLE)
    find_program(GLSLC_EXECUTABLE glslc REQUIRED)
  endif()

  file(GLOB GLSL_SOURCES
    "${SHADER_DIR}/*.vert" "${SHADER_DIR}/*.frag" "${SHADER_DIR}/*.comp"
    "${SHADER_DIR}/*.geom" "${SHADER_DIR}/*.tesc" "${SHADER_DIR}/*.tese"
  )

  set(GLSL_SHADER_OUTPUTS "")
  foreach(GLSL_SHADER ${GLSL_SOURCES})
    get_filename_component(GLSL_SHADER_NAME ${GLSL_SHADER} NAME)
    set(GLSL_SPIRV ${GLSL_SHADER}.spv)

    add_custom_command(
      OUTPUT ${GLSL_SPIRV}
      COMMAND ${GLSLC_EXECUTABLE} ${GLSL_SHADER} -o ${GLSL_SPIRV}
      DEPENDS ${GLSL_SHADER}
      COMMENT "Compiling GLSL Shader: ${GLSL_SHADER_NAME} ¡ú ${GLSL_SPIRV}"
    )
    list(APPEND GLSL_SHADER_OUTPUTS ${GLSL_SPIRV})
  endforeach()

  add_custom_target(${TARGET} ALL DEPENDS ${GLSL_SHADER_OUTPUTS})
endfunction()
