#include "orthographic_vertex_shader.h"

#include "xbox_math_matrix.h"

OrthographicVertexShader::OrthographicVertexShader(uint32_t framebuffer_width, uint32_t framebuffer_height, float left,
                                                   float right, float bottom, float top, float near, float far,
                                                   float z_min, float z_max)
    : ProjectionVertexShader(framebuffer_width, framebuffer_height, z_min, z_max) {
  width_ = right - left;
  height_ = bottom - top;
  far_minus_near_ = far - near;
  far_plus_near_ = far + near;
}

void OrthographicVertexShader::CalculateProjectionMatrix() {
  MatrixSetIdentity(projection_matrix_);
  projection_matrix_[0][0] = 2.0f / width_;
  projection_matrix_[1][1] = 2.0f / height_;
  projection_matrix_[2][2] = -2.0f / far_minus_near_;
  projection_matrix_[3][2] = -1.0f * (far_plus_near_ / far_minus_near_);
  projection_matrix_[3][3] = 1.0;
}
