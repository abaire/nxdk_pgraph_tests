#include "shader_program.h"

#include <pbkit/pbkit.h>

#include <memory>

#include "pbkit_ext.h"

void ShaderProgram::LoadShaderProgram(const uint32_t *shader, uint32_t shader_size) const {
  uint32_t *p;
  int i;

  p = pb_begin();

  // Set run address of shader
  p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_START, 0);

  p = pb_push1(
      p, NV097_SET_TRANSFORM_EXECUTION_MODE,
      MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_MODE_PROGRAM) |
          MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE_PRIV));

  p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_CXT_WRITE_EN, 0);
  pb_end(p);

  // Set cursor and begin copying program
  p = pb_begin();
  p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_LOAD, 0);
  pb_end(p);

  for (i = 0; i < shader_size / 16; i++) {
    p = pb_begin();
    pb_push(p++, NV097_SET_TRANSFORM_PROGRAM, 4);
    memcpy(p, &shader[i * 4], 4 * 4);
    p += 4;
    pb_end(p);
  }

  if (enable_texture_) {
    LoadTexturedPixelShader();
  } else {
    LoadUntexturedPixelShader();
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

void ShaderProgram::DisablePixelShader() {
  uint32_t *p = pb_begin();
  p = pb_push1(p, NV097_SET_SHADER_STAGE_PROGRAM, NV097_SET_SHADER_STAGE_PROGRAM_STAGE0_PROGRAM_NONE);
  pb_end(p);
}

void ShaderProgram::Activate() {
  OnActivate();

  if (shader_override_) {
    LoadShaderProgram(shader_override_, shader_override_size_);
  } else {
    OnLoadShader();
  }
}

void ShaderProgram::PrepareDraw() {
  OnLoadConstants();

  if (uniforms_.empty() || !uniform_upload_required_) {
    return;
  }

  UploadConstants();
}

void ShaderProgram::UploadConstants() {
  auto p = pb_begin();

  /* Set shader constants cursor at C0 */
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_VP_UPLOAD_CONST_ID, 96 + uniform_start_offset_);

  uint32_t *uniforms = uniforms_.data();
  uint32_t values_remaining = uniforms_.size();
  while (values_remaining > 16) {
    pb_push(p++, NV20_TCL_PRIMITIVE_3D_VP_UPLOAD_CONST_X, 16);
    memcpy(p, uniforms, 16 * 4);
    uniforms += 16;
    p += 16;
    values_remaining -= 16;
  }

  if (values_remaining) {
    pb_push(p++, NV20_TCL_PRIMITIVE_3D_VP_UPLOAD_CONST_X, values_remaining);
    memcpy(p, uniforms, values_remaining * 4);
    p += values_remaining;
  }

  pb_end(p);
  uniform_upload_required_ = false;
}

void ShaderProgram::SetUniformBlock(uint32_t slot, const uint32_t *values, uint32_t num_slots) {
  // Constants are always provided as int4 vectors.
  uint32_t index = slot * 4;
  uint32_t num_ints = num_slots * 4;
  uint32_t required_size = index + num_ints;

  if (uniforms_.size() < required_size) {
    uniforms_.resize(required_size);
  }

  uint32_t *data = uniforms_.data() + index;
  memcpy(data, values, num_ints * sizeof(uint32_t));

  uniform_upload_required_ = true;
}

void ShaderProgram::SetUniform4x4F(uint32_t slot, const float *value) {
  SetUniformBlock(slot, reinterpret_cast<const uint32_t *>(value), 4);
}

void ShaderProgram::SetUniform4F(uint32_t slot, const float *value) {
  SetUniformBlock(slot, reinterpret_cast<const uint32_t *>(value), 1);
}

void ShaderProgram::SetUniform4F(uint32_t slot, const uint32_t *value) {
  SetUniformF(slot, value[0], value[1], value[2], value[3]);
}

void ShaderProgram::SetUniformF(uint32_t slot, uint32_t value) { SetUniformF(slot, value, 0, 0, 0); }

void ShaderProgram::SetUniformF(uint32_t slot, uint32_t x, uint32_t y) { SetUniformF(slot, x, y, 0, 0); }

void ShaderProgram::SetUniformF(uint32_t slot, uint32_t x, uint32_t y, uint32_t z) { SetUniformF(slot, x, y, z, 0); }

void ShaderProgram::SetUniformF(uint32_t slot, uint32_t x, uint32_t y, uint32_t z, uint32_t w) {
  SetUniformF(slot, static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), static_cast<float>(w));
}

void ShaderProgram::SetUniformF(uint32_t slot, float value) { SetUniformF(slot, *(uint32_t *)&value); }

void ShaderProgram::SetUniformF(uint32_t slot, float x, float y) { SetUniformF(slot, x, y, 0, 0); }

void ShaderProgram::SetUniformF(uint32_t slot, float x, float y, float z) { SetUniformF(slot, x, y, z, 0); }

void ShaderProgram::SetUniformF(uint32_t slot, float x, float y, float z, float w) {
  const float vector[] = {x, y, z, w};
  SetUniform4F(slot, vector);
}
