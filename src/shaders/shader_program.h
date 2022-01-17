#ifndef NXDK_PGRAPH_TESTS_SHADER_PROGRAM_H
#define NXDK_PGRAPH_TESTS_SHADER_PROGRAM_H

#include <cstdint>

class ShaderProgram {
 public:
  ShaderProgram(bool enable_texture = true) : enable_texture_(enable_texture) {}

  void Activate();
  void PrepareDraw();

  // Note: It is the caller's responsibility to ensure that the shader array remains valid for the lifetime of this
  // instance.
  void SetShaderOverride(const uint32_t* shader, uint32_t shader_size) {
    shader_override_ = shader;
    shader_override_size_ = shader_size;
  }

  void SetTextureEnabled(bool enabled = true) { enable_texture_ = enabled; }
  bool RequiresTextureStage() { return enable_texture_; }

  static void LoadTexturedPixelShader();
  static void LoadUntexturedPixelShader();
  static void DisablePixelShader();

 protected:
  virtual void OnActivate() {}
  virtual void OnLoadShader() {}
  virtual void OnLoadConstants() {}

  void LoadShaderProgram(const uint32_t *shader, uint32_t shader_size) const;

 protected:
  bool enable_texture_;
  const uint32_t* shader_override_{nullptr};
  uint32_t shader_override_size_{0};
};

#endif  // NXDK_PGRAPH_TESTS_SHADER_PROGRAM_H
