cmake_minimum_required(VERSION 3.10)

# ==========================================
# Compile Slang Shaders to SPIR-V
# Usage:
#   add_slang_shader_target(MySlangShaders PathName)
# ==========================================
function(add_slang_shader_target TARGET SHADER_DIR)
  if(NOT SLANGC_EXECUTABLE)
  find_program(SLANGC_EXECUTABLE slangc REQUIRED)
  endif()

  file(GLOB SLANG_SOURCES "${SHADER_DIR}/*.slang" )

  set(SLANG_SHADER_OUTPUTS "")
  foreach(SLANG_SHADER ${SLANG_SOURCES})
    get_filename_component(SLANG_SHADER_NAME ${SLANG_SHADER} NAME)
    set(SLANG_SPIRV ${SLANG_SHADER}.spv)

    add_custom_command(
      OUTPUT ${SLANG_SPIRV}
                  COMMAND ${SLANGC_EXECUTABLE}
                    ${SLANG_SHADER}
                    -target spirv
                    -profile spirv_1_4
                    -emit-spirv-directly
                    -fvk-use-entrypoint-name
                    -entry vertMain
                    -entry fragMain
                    -o ${SLANG_SPIRV}
      DEPENDS ${SLANG_SHADER}
      COMMENT "Compiling Slang Shader: ${SLANG_SHADER_NAME} ¡ú ${SLANG_SPIRV}"
    )
    list(APPEND SLANG_SHADER_OUTPUTS ${SLANG_SPIRV})
  endforeach()

  add_custom_target(${TARGET} ALL DEPENDS ${SLANG_SHADER_OUTPUTS})
endfunction()