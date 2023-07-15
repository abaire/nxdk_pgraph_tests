#include "vertex_shader_program.h"

#include <pbkit/pbkit.h>

#include <memory>

#include "pbkit_ext.h"

using namespace XboxMath;

void VertexShaderProgram::LoadShaderProgram(const uint32_t *shader, uint32_t shader_size) const {
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
}

void VertexShaderProgram::Activate() {
  OnActivate();

  if (shader_override_) {
    LoadShaderProgram(shader_override_, shader_override_size_);
  } else {
    OnLoadShader();
  }
}

void VertexShaderProgram::PrepareDraw() {
  OnLoadConstants();

  if (base_transform_constants_.empty() || !uniform_upload_required_) {
    return;
  }

  UploadConstants();
}

void VertexShaderProgram::UploadConstants() {
  MergeUniforms();

  auto p = pb_begin();

  p = pb_push1(p, NV097_SET_TRANSFORM_CONSTANT_LOAD, 96 + uniform_start_offset_);

  uint32_t *uniforms = base_transform_constants_.data();
  uint32_t values_remaining = base_transform_constants_.size();
  while (values_remaining > 16) {
    pb_push(p++, NV097_SET_TRANSFORM_CONSTANT, 16);
    memcpy(p, uniforms, 16 * 4);
    uniforms += 16;
    p += 16;
    values_remaining -= 16;
  }

  if (values_remaining) {
    pb_push(p++, NV097_SET_TRANSFORM_CONSTANT, values_remaining);
    memcpy(p, uniforms, values_remaining * 4);
    p += values_remaining;
  }

  pb_end(p);
  uniform_upload_required_ = false;
}

void VertexShaderProgram::SetTransformConstantBlock(uint32_t slot, const uint32_t *values, uint32_t num_slots) {
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

void VertexShaderProgram::SetBaseUniform4x4F(uint32_t slot, const matrix4_t &value) {
  SetTransformConstantBlock(slot, reinterpret_cast<const uint32_t *>(value), 4);
}

void VertexShaderProgram::SetBaseUniform4F(uint32_t slot, const float *value) {
  SetTransformConstantBlock(slot, reinterpret_cast<const uint32_t *>(value), 1);
}

void VertexShaderProgram::SetBaseUniform4I(uint32_t slot, const uint32_t *value) {
  SetTransformConstantBlock(slot, value, 1);
}

void VertexShaderProgram::SetBaseUniformI(uint32_t slot, uint32_t x, uint32_t y, uint32_t z, uint32_t w) {
  const uint32_t vector[] = {x, y, z, w};
  SetBaseUniform4I(slot, vector);
}

void VertexShaderProgram::SetBaseUniformF(uint32_t slot, float x, float y, float z, float w) {
  const float vector[] = {x, y, z, w};
  SetBaseUniform4F(slot, vector);
}

void VertexShaderProgram::SetUniform4x4F(uint32_t slot, const XboxMath::matrix4_t &value) {
  SetUniformBlock(slot, reinterpret_cast<const uint32_t *>(value), 4);
}

void VertexShaderProgram::SetUniform4F(uint32_t slot, const float *value) {
  SetUniformBlock(slot, reinterpret_cast<const uint32_t *>(value), 1);
}

void VertexShaderProgram::SetUniform4I(uint32_t slot, const uint32_t *value) { SetUniformBlock(slot, value, 1); }

void VertexShaderProgram::SetUniformI(uint32_t slot, uint32_t x, uint32_t y, uint32_t z, uint32_t w) {
  const uint32_t vector[] = {x, y, z, w};
  SetUniform4I(slot, vector);
}

void VertexShaderProgram::SetUniformF(uint32_t slot, float x, float y, float z, float w) {
  const float vector[] = {x, y, z, w};
  SetUniform4F(slot, vector);
}

void VertexShaderProgram::SetUniformBlock(uint32_t slot, const uint32_t *values, uint32_t num_slots) {
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

void VertexShaderProgram::MergeUniforms() {
  for (auto item : uniforms_) {
    uint32_t slot = item.first;

    uint32_t index = slot * 4;
    base_transform_constants_[index++] = item.second.x;
    base_transform_constants_[index++] = item.second.y;
    base_transform_constants_[index++] = item.second.z;
    base_transform_constants_[index++] = item.second.w;
  }
}
