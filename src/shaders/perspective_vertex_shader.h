#ifndef NXDK_PGRAPH_TESTS_SHADERS_PERSPECTIVE_VERTEX_SHADER_H_
#define NXDK_PGRAPH_TESTS_SHADERS_PERSPECTIVE_VERTEX_SHADER_H_

#include <cmath>
#include <cstdint>

#include "projection_vertex_shader.h"

class PerspectiveVertexShader : public ProjectionVertexShader {
 public:
  PerspectiveVertexShader(uint32_t framebuffer_width, uint32_t framebuffer_height, float z_min = 0,
                          float z_max = 0x7FFF, float fov_y = M_PI * 0.25f, float near = 1.0f, float far = 10.0f);

  inline void SetNear(float val) { near_ = val; }
  inline void SetFar(float val) { far_ = val; }

 protected:
  void CalculateProjectionMatrix() override;

 private:
  float fov_y_;
  float aspect_ratio_;
  float near_{0.0f};
  float far_{100.0f};
};

#endif  // NXDK_PGRAPH_TESTS_SHADERS_PERSPECTIVE_VERTEX_SHADER_H_
