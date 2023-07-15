#include "perspective_vertex_shader.h"

#include <memory>

#include "math3d.h"

PerspectiveVertexShader::PerspectiveVertexShader(uint32_t framebuffer_width, uint32_t framebuffer_height, float z_min,
                                                 float z_max, float fov_y, float near, float far)
    : ProjectionVertexShader(framebuffer_width, framebuffer_height, z_min, z_max),
      fov_y_(fov_y),
      aspect_ratio_(framebuffer_width_ / framebuffer_height_),
      near_(near),
      far_(far) {}

void PerspectiveVertexShader::CalculateProjectionMatrix() {
  memset(projection_matrix_, 0, sizeof(projection_matrix_));

  float y_scale = 1.0f / tanf(fov_y_ * 0.5f);
  float far_over_distance = far_ / (far_ - near_);

  projection_matrix_[_11] = y_scale / aspect_ratio_;
  projection_matrix_[_22] = y_scale;
  projection_matrix_[_33] = far_over_distance;
  projection_matrix_[_34] = 1.0f;
  projection_matrix_[_43] = -near_ * far_over_distance;
}
