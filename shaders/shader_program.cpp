#include "shader_program.h"

#include <pbkit/pbkit.h>

#include <memory>

#include "pbkit_ext.h"

void ShaderProgram::LoadShaderProgram(const uint32_t *shader, uint32_t shader_size) {
  /* Load the shader we will render with */
  uint32_t *p;
  int i;

  /* Setup vertex shader */
  p = pb_begin();

  /* Set run address of shader */
  p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_START, 0);

  /* Set execution mode */
  p = pb_push1(
      p, NV097_SET_TRANSFORM_EXECUTION_MODE,
      MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_MODE_PROGRAM) |
          MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE_PRIV));

  p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_CXT_WRITE_EN, 0);
  pb_end(p);

  /* Set cursor and begin copying program */
  p = pb_begin();
  p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_LOAD, 0);
  pb_end(p);

  /* Copy program instructions (16-bytes each) */
  for (i = 0; i < shader_size / 16; i++) {
    p = pb_begin();
    pb_push(p++, NV097_SET_TRANSFORM_PROGRAM, 4);
    memcpy(p, &shader[i * 4], 4 * 4);
    p += 4;
    pb_end(p);
  }
}

void ShaderProgram::LoadTexturedPixelShader() {
  /* Setup fragment shader */
  uint32_t *p = pb_begin();

// clang format off
#include "shaders/textured_pixelshader.inl"
  // clang format on

  pb_end(p);
}

void ShaderProgram::LoadUntexturedPixelShader() {
  /* Setup fragment shader */
  uint32_t *p = pb_begin();

// clang format off
#include "shaders/untextured_pixelshader.inl"
  // clang format on

  pb_end(p);
}
