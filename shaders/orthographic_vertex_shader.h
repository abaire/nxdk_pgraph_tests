#ifndef NXDK_PGRAPH_TESTS_SHADERS_ORTHOGRAPHIC_VERTEX_SHADER_H_
#define NXDK_PGRAPH_TESTS_SHADERS_ORTHOGRAPHIC_VERTEX_SHADER_H_

#include "projection_vertex_shader.h"

class OrthographicVertexShader : public ProjectionVertexShader {
 public:
  OrthographicVertexShader(uint32_t framebuffer_width, uint32_t framebuffer_height, float left, float right,
                           float bottom, float top, float near, float far);

 protected:
  void CalculateProjectionMatrix() override;

 private:
  float width_;
  float height_;
  float far_minus_near_;
  float far_plus_near_;
};

#endif  // NXDK_PGRAPH_TESTS_SHADERS_ORTHOGRAPHIC_VERTEX_SHADER_H_
