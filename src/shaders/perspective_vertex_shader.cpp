#include "perspective_vertex_shader.h"

#include <memory>

#include "math3d.h"

PerspectiveVertexShader::PerspectiveVertexShader(uint32_t framebuffer_width, uint32_t framebuffer_height, float z_min,
                                                 float z_max, float fov_y, float left, float right, float bottom,
                                                 float top, float near, float far)
    : ProjectionVertexShader(framebuffer_width, framebuffer_height, z_min, z_max),
      left_(left),
      right_(right),
      top_(top),
      bottom_(bottom),
      near_(near),
      far_(far),
      fov_y_(fov_y),
      aspect_ratio_(framebuffer_width_ / framebuffer_height_) {}

void PerspectiveVertexShader::CalculateProjectionMatrix() {
  memset(projection_matrix_, 0, sizeof(projection_matrix_));

  float y_scale = 1.0f / tanf(fov_y_ * 0.5f);
  float far_over_distance = far_ / (far_ - near_);

  projection_matrix_[_11] = y_scale / aspect_ratio_;
  projection_matrix_[_22] = y_scale;
  projection_matrix_[_33] = far_over_distance;
  projection_matrix_[_34] = 1.0f;
  projection_matrix_[_43] = -near_ * far_over_distance;

  //  create_view_screen(projection_matrix_, aspect_ratio_, left_, right_, bottom_, top_, near_, far_);
}
