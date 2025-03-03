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

  if (uniform_upload_required_) {
    UploadConstants();
  }
}

void VertexShaderProgram::UploadConstants() {
  auto load_constants = [](int start_offset, const std::map<uint32_t, TransformConstant> &constants) {
    if (constants.empty()) {
      return;
    }

    auto first_index = constants.begin()->first;
    auto last_index = constants.rbegin()->first;
    auto num_items = last_index - first_index + 1;

    std::vector<uint32_t> transform_constants(num_items * 4, 0);
    for (auto &entry : constants) {
      auto index = (entry.first - first_index) * 4;
      transform_constants[index++] = entry.second.x;
      transform_constants[index++] = entry.second.y;
      transform_constants[index++] = entry.second.z;
      transform_constants[index] = entry.second.w;
    }

    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_TRANSFORM_CONSTANT_LOAD, start_offset + first_index);

    uint32_t *uniforms = transform_constants.data();
    uint32_t values_remaining = transform_constants.size();
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
  };

  // Overwriting the lower constants will break the fixed function pipeline.
  // load_constants(0, base_96_transform_constants_);

  auto merged = base_transform_constants_;
  for (auto &entry : uniforms_) {
    merged[entry.first] = entry.second;
  }
  load_constants(kShaderUserConstantOffset, merged);

  uniform_upload_required_ = false;
}

void VertexShaderProgram::SetBaseUniform4x4F(uint32_t slot, const matrix4_t &value) {
  SetTransformConstantBlock(base_transform_constants_, slot, reinterpret_cast<const uint32_t *>(value), 4);
}

void VertexShaderProgram::SetBaseUniform4F(uint32_t slot, const float *value) {
  SetTransformConstantBlock(base_transform_constants_, slot, reinterpret_cast<const uint32_t *>(value), 1);
}

void VertexShaderProgram::SetBaseUniform4I(uint32_t slot, const uint32_t *value) {
  SetTransformConstantBlock(base_transform_constants_, slot, value, 1);
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
  SetTransformConstantBlock(uniforms_, slot, reinterpret_cast<const uint32_t *>(value), 4);
}

void VertexShaderProgram::SetUniform4F(uint32_t slot, const float *value) {
  SetTransformConstantBlock(uniforms_, slot, reinterpret_cast<const uint32_t *>(value), 1);
}

void VertexShaderProgram::SetUniform4I(uint32_t slot, const uint32_t *value) {
  SetTransformConstantBlock(uniforms_, slot, value, 1);
}

void VertexShaderProgram::SetUniformI(uint32_t slot, uint32_t x, uint32_t y, uint32_t z, uint32_t w) {
  const uint32_t vector[] = {x, y, z, w};
  SetUniform4I(slot, vector);
}

void VertexShaderProgram::SetUniformF(uint32_t slot, float x, float y, float z, float w) {
  const float vector[] = {x, y, z, w};
  SetUniform4F(slot, vector);
}

void VertexShaderProgram::SetTransformConstantBlock(std::map<uint32_t, TransformConstant> &constants_map, uint32_t slot,
                                                    const uint32_t *values, uint32_t num_slots) {
  for (auto i = 0; i < num_slots; ++i, ++slot) {
    TransformConstant val = {*values++, *values++, *values++, *values++};
    constants_map[slot] = val;
  }

  uniform_upload_required_ = true;
}
