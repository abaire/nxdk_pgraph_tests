#include "pixel_shader_program.h"

#include <pbkit/pbkit.h>

#include "pushbuffer.h"

void PixelShaderProgram::LoadTexturedPixelShader() {
  uint32_t *p = pb_begin();

  // clang-format off
#include "textured_pixelshader.inl"
  // clang-format on

  pb_end(p);
}

void PixelShaderProgram::LoadUntexturedPixelShader() {
  uint32_t *p = pb_begin();

  // clang-format off
#include "untextured_pixelshader.inl"
  // clang-format on

  pb_end(p);
}

void PixelShaderProgram::DisablePixelShader() {
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_SHADER_STAGE_PROGRAM, 0);
  Pushbuffer::End();
}
