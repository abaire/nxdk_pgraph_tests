#ifndef NXDK_PGRAPH_TESTS_SHADERS_PROJECTION_VERTEX_SHADER_H_
#define NXDK_PGRAPH_TESTS_SHADERS_PROJECTION_VERTEX_SHADER_H_

#include <cstdint>

#include "math3d.h"
#include "vertex_shader_program.h"

class ProjectionVertexShader : public VertexShaderProgram {
 public:
  ProjectionVertexShader(uint32_t framebuffer_width, uint32_t framebuffer_height, float z_min = 0, float z_max = 0x7FFF,
                         bool enable_lighting = true, bool use_4_component_texcoords = false);

  void SetLightingEnabled(bool enabled = true) { enable_lighting_ = enabled; }

  inline void SetUse4ComponentTexcoords(bool enable = true) { use_4_component_texcoords_ = enable; }
  inline void SetUseD3DStyleViewport(bool enable = true) {
    use_d3d_style_viewport_ = enable;
    UpdateMatrices();
  }

  inline void SetZMin(float val) { z_min_ = val; }
  inline void SetZMax(float val) { z_max_ = val; }

  inline float GetZMin() const { return z_min_; }
  inline float GetZMax() const { return z_max_; }

  inline void LookAt(const VECTOR camera_position, const VECTOR look_at_point) {
    VECTOR y_axis{0, 1, 0, 1};
    LookAt(camera_position, look_at_point, y_axis);
  }

  void LookAt(const VECTOR camera_position, const VECTOR look_at_point, const VECTOR up);
  void LookTo(const VECTOR camera_position, const VECTOR camera_direction, const VECTOR up);
  void SetCamera(const VECTOR position, const VECTOR rotation);
  void SetDirectionalLightDirection(const VECTOR &direction);

  MATRIX &GetModelMatrix() { return model_matrix_; }
  MATRIX &GetViewMatrix() { return view_matrix_; }
  MATRIX &GetProjectionMatrix() { return projection_matrix_; }
  MATRIX &GetViewportMatrix() { return viewport_matrix_; }
  MATRIX &GetProjectionViewportMatrix() { return projection_viewport_matrix_; }

 protected:
  void OnActivate() override;
  void OnLoadShader() override;
  void OnLoadConstants() override;
  virtual void CalculateProjectionMatrix() = 0;

 private:
  void UpdateMatrices();
  void CalculateViewportMatrix();

 protected:
  float framebuffer_width_{0.0};
  float framebuffer_height_{0.0};
  float z_min_;
  float z_max_;

  bool enable_lighting_{true};
  bool use_4_component_texcoords_{false};
  // Generate a viewport matrix that matches XDK/D3D behavior.
  bool use_d3d_style_viewport_{false};

  MATRIX model_matrix_{};
  MATRIX view_matrix_{};
  MATRIX projection_matrix_{};
  MATRIX viewport_matrix_{};
  MATRIX projection_viewport_matrix_{};

  VECTOR camera_position_ = {0, 0, -2.25, 1};
  VECTOR light_direction_ = {0, 0, 1, 1};
};

#endif  // NXDK_PGRAPH_TESTS_SHADERS_PROJECTION_VERTEX_SHADER_H_
