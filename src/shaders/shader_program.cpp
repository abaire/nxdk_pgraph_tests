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
  p = pb_push1(p, NV097_SET_SHADER_STAGE_PROGRAM, 0);
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

  if (base_transform_constants_.empty() || !uniform_upload_required_) {
    return;
  }

  UploadConstants();
}

void ShaderProgram::UploadConstants() {
  MergeUniforms();

  auto p = pb_begin();

  /* Set shader constants cursor at C0 */
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_VP_UPLOAD_CONST_ID, 96 + uniform_start_offset_);

  uint32_t *uniforms = base_transform_constants_.data();
  uint32_t values_remaining = base_transform_constants_.size();
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

void ShaderProgram::SetTransformConstantBlock(uint32_t slot, const uint32_t *values, uint32_t num_slots) {
  // Constants are always provided as int4 vectors.
  uint32_t index = slot * 4;
  uint32_t num_ints = num_slots * 4;
  uint32_t required_size = index + num_ints;

  if (base_transform_constants_.size() < required_size) {
    base_transform_constants_.resize(required_size);
  }

  uint32_t *data = base_transform_constants_.data() + index;
  memcpy(data, values, num_ints * sizeof(uint32_t));

  uniform_upload_required_ = true;
}

void ShaderProgram::SetBaseUniform4x4F(uint32_t slot, const float *value) {
  SetTransformConstantBlock(slot, reinterpret_cast<const uint32_t *>(value), 4);
}

void ShaderProgram::SetBaseUniform4F(uint32_t slot, const float *value) {
  SetTransformConstantBlock(slot, reinterpret_cast<const uint32_t *>(value), 1);
}

void ShaderProgram::SetBaseUniform4I(uint32_t slot, const uint32_t *value) {
  SetTransformConstantBlock(slot, value, 1);
}

void ShaderProgram::SetBaseUniformI(uint32_t slot, uint32_t x, uint32_t y, uint32_t z, uint32_t w) {
  const uint32_t vector[] = {x, y, z, w};
  SetBaseUniform4I(slot, vector);
}

void ShaderProgram::SetBaseUniformF(uint32_t slot, float x, float y, float z, float w) {
  const float vector[] = {x, y, z, w};
  SetBaseUniform4F(slot, vector);
}

void ShaderProgram::SetUniform4x4F(uint32_t slot, const float *value) {
  SetUniformBlock(slot, reinterpret_cast<const uint32_t *>(value), 4);
}

void ShaderProgram::SetUniform4F(uint32_t slot, const float *value) {
  SetUniformBlock(slot, reinterpret_cast<const uint32_t *>(value), 1);
}

void ShaderProgram::SetUniform4I(uint32_t slot, const uint32_t *value) { SetUniformBlock(slot, value, 1); }

void ShaderProgram::SetUniformI(uint32_t slot, uint32_t x, uint32_t y, uint32_t z, uint32_t w) {
  const uint32_t vector[] = {x, y, z, w};
  SetUniform4I(slot, vector);
}

void ShaderProgram::SetUniformF(uint32_t slot, float x, float y, float z, float w) {
  const float vector[] = {x, y, z, w};
  SetUniform4F(slot, vector);
}

void ShaderProgram::SetUniformBlock(uint32_t slot, const uint32_t *values, uint32_t num_slots) {
  for (auto i = 0; i < num_slots; ++i, ++slot) {
    TransformConstant val = {*values++, *values++, *values++, *values++};
    uniforms_[slot] = val;
  }

  uint32_t index = slot * 4;
  uint32_t num_ints = num_slots * 4;
  uint32_t required_size = index + num_ints;

  if (base_transform_constants_.size() < required_size) {
    base_transform_constants_.resize(required_size);
  }

  uniform_upload_required_ = true;
}

void ShaderProgram::MergeUniforms() {
  for (auto item : uniforms_) {
    uint32_t slot = item.first;

    uint32_t index = slot * 4;
    base_transform_constants_[index++] = item.second.x;
    base_transform_constants_[index++] = item.second.y;
    base_transform_constants_[index++] = item.second.z;
    base_transform_constants_[index++] = item.second.w;
  }
}
