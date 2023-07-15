#ifndef NXDK_PGRAPH_TESTS_VERTEX_SHADER_PROGRAM_H
#define NXDK_PGRAPH_TESTS_VERTEX_SHADER_PROGRAM_H

#include <cstdint>
#include <map>
#include <vector>

class VertexShaderProgram {
 public:
  VertexShaderProgram() = default;
  virtual ~VertexShaderProgram() = default;

  void Activate();
  void PrepareDraw();

  // Note: It is the caller's responsibility to ensure that the shader array remains valid for the lifetime of this
  // instance.
  void SetShaderOverride(const uint32_t *shader, uint32_t shader_size, int32_t uniform_start_offset = 0) {
    shader_override_ = shader;
    shader_override_size_ = shader_size;
    uniform_start_offset_ = uniform_start_offset;
  }

  void SetUniform4x4F(uint32_t slot, const float *value);
  void SetUniform4F(uint32_t slot, const float *value);
  void SetUniform4I(uint32_t slot, const uint32_t *value);

  void SetUniformF(uint32_t slot, float x, float y = 0.0f, float z = 0.0f, float w = 0.0f);
  void SetUniformI(uint32_t slot, uint32_t x, uint32_t y = 0, uint32_t z = 0, uint32_t w = 0);

 protected:
  virtual void OnActivate() {}
  virtual void OnLoadShader() {}
  virtual void OnLoadConstants(){};

  void LoadShaderProgram(const uint32_t *shader, uint32_t shader_size) const;

  void SetTransformConstantBlock(uint32_t slot, const uint32_t *values, uint32_t num_slots);

  void SetBaseUniform4x4F(uint32_t slot, const float *value);
  void SetBaseUniform4F(uint32_t slot, const float *value);
  void SetBaseUniform4I(uint32_t slot, const uint32_t *value);

  void SetBaseUniformF(uint32_t slot, float x, float y = 0.0f, float z = 0.0f, float w = 0.0f);
  void SetBaseUniformI(uint32_t slot, uint32_t x, uint32_t y = 0, uint32_t z = 0, uint32_t w = 0);

  void SetUniformBlock(uint32_t slot, const uint32_t *values, uint32_t num_slots);

  void UploadConstants();
  void MergeUniforms();

 protected:
  const uint32_t *shader_override_{nullptr};
  uint32_t shader_override_size_{0};

  int32_t uniform_start_offset_{0};
  // Transform program constants calculated by the shader C++ code.
  std::vector<uint32_t> base_transform_constants_;
  // Overrides for
  struct TransformConstant {
    uint32_t x, y, z, w;
  };
  std::map<uint32_t, TransformConstant> uniforms_;
  bool uniform_upload_required_{true};
};

#endif  // NXDK_PGRAPH_TESTS_VERTEX_SHADER_PROGRAM_H
