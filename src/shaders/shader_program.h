#ifndef NXDK_PGRAPH_TESTS_SHADER_PROGRAM_H
#define NXDK_PGRAPH_TESTS_SHADER_PROGRAM_H

#include <cstdint>
#include <vector>

class ShaderProgram {
 public:
  ShaderProgram(bool enable_texture = true) : enable_texture_(enable_texture) {}

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
  void SetUniform4F(uint32_t slot, const uint32_t *value);

  void SetUniformF(uint32_t slot, uint32_t value);
  void SetUniformF(uint32_t slot, uint32_t x, uint32_t y);
  void SetUniformF(uint32_t slot, uint32_t x, uint32_t y, uint32_t z);
  void SetUniformF(uint32_t slot, uint32_t x, uint32_t y, uint32_t z, uint32_t w);
  void SetUniformF(uint32_t slot, float value);
  void SetUniformF(uint32_t slot, float x, float y);
  void SetUniformF(uint32_t slot, float x, float y, float z);
  void SetUniformF(uint32_t slot, float x, float y, float z, float w);

  void SetTextureEnabled(bool enabled = true) { enable_texture_ = enabled; }
  bool RequiresTextureStage() const { return enable_texture_; }

  static void LoadTexturedPixelShader();
  static void LoadUntexturedPixelShader();
  static void DisablePixelShader();

 protected:
  virtual void OnActivate() {}
  virtual void OnLoadShader() {}
  virtual void OnLoadConstants(){};

  void LoadShaderProgram(const uint32_t *shader, uint32_t shader_size) const;

  void SetUniformBlock(uint32_t slot, const uint32_t *values, uint32_t num_slots);
  void UploadConstants();

 protected:
  bool enable_texture_;
  const uint32_t *shader_override_{nullptr};
  uint32_t shader_override_size_{0};

  int32_t uniform_start_offset_{0};
  std::vector<uint32_t> uniforms_;
  bool uniform_upload_required_{true};
};

#endif  // NXDK_PGRAPH_TESTS_SHADER_PROGRAM_H
