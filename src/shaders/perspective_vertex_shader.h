#ifndef NXDK_PGRAPH_TESTS_SHADERS_PERSPECTIVE_VERTEX_SHADER_H_
#define NXDK_PGRAPH_TESTS_SHADERS_PERSPECTIVE_VERTEX_SHADER_H_

#include <cstdint>
#define _USE_MATH_DEFINES
#include <cmath>

#include "projection_vertex_shader.h"

class PerspectiveVertexShader : public ProjectionVertexShader {
 public:
  PerspectiveVertexShader(uint32_t framebuffer_width, uint32_t framebuffer_height, float z_min = 0,
                          float z_max = 0x7FFF, float fov_y = M_PI * 0.25f, float left = -1.0f, float right = 1.0f,
                          float bottom = 1.0f, float top = -1.0f, float near = 1.0f, float far = 10.0f);

  inline void SetNear(float val) { near_ = val; }
  inline void SetFar(float val) { far_ = val; }

 protected:
  void CalculateProjectionMatrix() override;

 private:
  float fov_y_;
  float aspect_ratio_;
  float left_{0.0f};
  float right_{1.0f};
  float top_{0.0f};
  float bottom_{1.0f};
  float near_{0.0f};
  float far_{100.0f};
};

#endif  // NXDK_PGRAPH_TESTS_SHADERS_PERSPECTIVE_VERTEX_SHADER_H_
