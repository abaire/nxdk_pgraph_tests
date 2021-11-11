#include "perspective_vertex_shader.h"

#include "math3d.h"

PerspectiveVertexShader::PerspectiveVertexShader(uint32_t framebuffer_width, uint32_t framebuffer_height, float left,
                                                 float right, float top, float bottom, float near, float far)
    : ProjectionVertexShader(framebuffer_width, framebuffer_height),
      left_(left),
      right_(right),
      top_(top),
      bottom_(bottom),
      near_(near),
      far_(far),
      aspect_ratio_(framebuffer_width_ / framebuffer_height_) {}

void PerspectiveVertexShader::CalculateProjectionMatrix() {
  matrix_unit(projection_matrix_);
  create_view_screen(projection_matrix_, aspect_ratio_, left_, right_, bottom_, top_, near_, far_);
}
