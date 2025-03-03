#ifndef NXDK_PGRAPH_TESTS_SHADERS_PROJECTION_VERTEX_SHADER_H_
#define NXDK_PGRAPH_TESTS_SHADERS_PROJECTION_VERTEX_SHADER_H_

#include <cstdint>

#include "vertex_shader_program.h"
#include "xbox_math_types.h"

using namespace XboxMath;

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

  inline void LookAt(const vector_t &camera_position, const vector_t &look_at_point) {
    vector_t y_axis{0, 1, 0, 1};
    LookAt(camera_position, look_at_point, y_axis);
  }

  void LookAt(const vector_t &camera_position, const vector_t &look_at_point, const vector_t &up);
  void LookTo(const vector_t &camera_position, const vector_t &camera_direction, const vector_t &up);
  void SetCamera(const vector_t &position, const vector_t &rotation);

  //! Sets the direction from the origin towards the directional light.
  void SetDirectionalLightDirection(const vector_t &direction);

  //! Sets the direction in which the directional light is casting light.
  void SetDirectionalLightCastDirection(const vector_t &direction);

  matrix4_t &GetModelMatrix() { return model_matrix_; }
  matrix4_t &GetViewMatrix() { return view_matrix_; }
  matrix4_t &GetProjectionMatrix() { return projection_matrix_; }
  matrix4_t &GetViewportMatrix() { return viewport_matrix_; }
  matrix4_t &GetProjectionViewportMatrix() { return projection_viewport_matrix_; }

  // Projects the given point (on the CPU), placing the resulting screen coordinates into `result`.
  void ProjectPoint(vector_t &result, const vector_t &world_point) const;

  //! Unprojects the given screen point, producing world coordinates that will project there.
  void UnprojectPoint(vector_t &result, const vector_t &screen_point) const;

  //! Unprojects the given screen point, producing world coordinates that will project there with the given Z value.
  void UnprojectPoint(vector_t &result, const vector_t &screen_point, float world_z) const;

  //! Causes matrices to be transposed before being uploaded to the shader.
  void SetTransposeOnUpload(bool transpose = true) { transpose_on_upload_ = transpose; }

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

  matrix4_t model_matrix_{};
  matrix4_t view_matrix_{};
  matrix4_t projection_matrix_{};
  matrix4_t viewport_matrix_{};
  matrix4_t projection_viewport_matrix_{};

  matrix4_t composite_matrix_{};
  matrix4_t inverse_composite_matrix_{};

  vector_t camera_position_ = {0, 0, -2.25, 1};
  vector_t light_direction_ = {0, 0, 1, 1};

  bool transpose_on_upload_{false};
};

#endif  // NXDK_PGRAPH_TESTS_SHADERS_PROJECTION_VERTEX_SHADER_H_
