#ifndef NXDK_PGRAPH_TESTS_VERTEX_SHADER_PROGRAM_H
#define NXDK_PGRAPH_TESTS_VERTEX_SHADER_PROGRAM_H

#include <cstdint>
#include <map>
#include <vector>

#include "xbox_math_types.h"

/**
 * Provides methods to manage and upload a vertex shader.
 */
class VertexShaderProgram {
 public:
  // The offset into the constant table of the first user-defined constant.
  static constexpr auto kShaderUserConstantOffset = 96;

 public:
  VertexShaderProgram() = default;
  virtual ~VertexShaderProgram() = default;

  void Activate();
  void PrepareDraw();

  // Note: It is the caller's responsibility to ensure that the shader array remains valid for the lifetime of this
  // instance.
  void SetShaderOverride(const uint32_t *shader, uint32_t shader_size) {
    shader_override_ = shader;
    shader_override_size_ = shader_size;
  }

  void SetUniform4x4F(uint32_t slot, const XboxMath::matrix4_t &value);
  void SetUniform4F(uint32_t slot, const float *value);
  void SetUniform4I(uint32_t slot, const uint32_t *value);

  void SetUniformF(uint32_t slot, float x, float y = 0.0f, float z = 0.0f, float w = 0.0f);
  void SetUniformI(uint32_t slot, uint32_t x, uint32_t y = 0, uint32_t z = 0, uint32_t w = 0);

 protected:
  struct TransformConstant {
    uint32_t x, y, z, w;
  };

 protected:
  virtual void OnActivate() {}
  virtual void OnLoadShader() {}
  virtual void OnLoadConstants() {};

  void LoadShaderProgram(const uint32_t *shader, uint32_t shader_size) const;

  void SetTransformConstantBlock(std::map<uint32_t, TransformConstant> &constants_map, uint32_t slot,
                                 const uint32_t *values, uint32_t num_slots);

  void SetBaseUniform4x4F(uint32_t slot, const XboxMath::matrix4_t &value);
  void SetBaseUniform4F(uint32_t slot, const float *value);
  void SetBaseUniform4I(uint32_t slot, const uint32_t *value);

  void SetBaseUniformF(uint32_t slot, float x, float y = 0.0f, float z = 0.0f, float w = 0.0f);
  void SetBaseUniformI(uint32_t slot, uint32_t x, uint32_t y = 0, uint32_t z = 0, uint32_t w = 0);

  void UploadConstants();

 protected:
  //! Compiled vertex shader code that should be used instead of using OnLoadShader.
  //! Note that this data is not owned by this VertexShaderProgram.
  const uint32_t *shader_override_{nullptr};
  uint32_t shader_override_size_{0};

  //! Map of index to constants that will be loaded starting at c[96].
  // Note: Use of registers before 96 is reserved as the fixed function pipeline
  // appears to make use of them (overwriting c0-c14 at least breaks tests that
  // use the FF pipeline).
  std::map<uint32_t, TransformConstant> base_transform_constants_;

  //! Map of index to constants that will overwrite/extend the base values. This
  //! allows a VertexShaderProgram subclass to populate values that are
  //! overwritten by users of the class.
  std::map<uint32_t, TransformConstant> uniforms_;

  bool uniform_upload_required_{true};
};

#endif  // NXDK_PGRAPH_TESTS_VERTEX_SHADER_PROGRAM_H
