#ifndef NXDK_PGRAPH_TESTS_SHADERS_PROJECTION_VERTEX_SHADER_H_
#define NXDK_PGRAPH_TESTS_SHADERS_PROJECTION_VERTEX_SHADER_H_

#include <cstdint>

#include "../math3d.h"
#include "math3d.h"
#include "shader_program.h"

class ProjectionVertexShader : public ShaderProgram {
 public:
  ProjectionVertexShader(uint32_t framebuffer_width, uint32_t framebuffer_height);

  void Activate() override;
  void PrepareDraw() override;

  void SetCamera(const VECTOR &position, const VECTOR &rotation);
  void SetOmniLightDirection(const VECTOR &direction);

  MATRIX &GetModelMatrix() { return model_matrix_; }
  MATRIX &GetViewMatrix() { return view_matrix_; }
  MATRIX &GetProjectionMatrix() { return projection_matrix_; }
  MATRIX &GetViewportMatrix() { return viewport_matrix_; }
  MATRIX &GetProjectionViewportMatrix() { return projection_viewport_matrix_; }

 protected:
  virtual void CalculateProjectionMatrix() = 0;

 private:
  void UpdateMatrices();

 protected:
  float framebuffer_width_{0.0};
  float framebuffer_height_{0.0};

  MATRIX model_matrix_{};
  MATRIX view_matrix_{};
  MATRIX projection_matrix_{};
  MATRIX viewport_matrix_{};
  MATRIX projection_viewport_matrix_{};

  VECTOR camera_position_ = {0, 0, 1, 1};
  VECTOR camera_rotation_ = {0, 0, 0, 1};
  VECTOR light_direction_ = {0, 0, 1, 1};
};

#endif  // NXDK_PGRAPH_TESTS_SHADERS_PROJECTION_VERTEX_SHADER_H_
