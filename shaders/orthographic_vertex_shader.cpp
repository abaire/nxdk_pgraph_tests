#include "orthographic_vertex_shader.h"

OrthographicVertexShader::OrthographicVertexShader(uint32_t framebuffer_width, uint32_t framebuffer_height, float left,
                                                   float right, float bottom, float top, float near, float far)
    :

      ProjectionVertexShader(framebuffer_width, framebuffer_height) {
  width_ = right - left;
  height_ = bottom - top;
  far_minus_near_ = far - near;
  far_plus_near_ = far + near;
}

void OrthographicVertexShader::CalculateProjectionMatrix() {
  matrix_unit(projection_matrix_);
  projection_matrix_[0] = 2.0f / width_;
  projection_matrix_[5] = 2.0f / height_;
  projection_matrix_[10] = -2.0f / far_minus_near_;
  projection_matrix_[14] = -1.0f * (far_plus_near_ / far_minus_near_);
  projection_matrix_[15] = 1.0;
}
