#ifndef NXDK_PGRAPH_TESTS_SRC_SHADERS_PERSPECTIVE_VERTEX_SHADER_NO_LIGHTING_H_
#define NXDK_PGRAPH_TESTS_SRC_SHADERS_PERSPECTIVE_VERTEX_SHADER_NO_LIGHTING_H_

#include "shaders/perspective_vertex_shader.h"

//! Simple perspective projection shader with no support for lighting calculations.
class PerspectiveVertexShaderNoLighting : public PBKitPlusPlus::PerspectiveVertexShader {
 public:
  PerspectiveVertexShaderNoLighting(uint32_t framebuffer_width, uint32_t framebuffer_height, float z_min = 0,
                                    float z_max = 0x7FFF, float fov_y = M_PI * 0.25f, float near = 1.0f,
                                    float far = 10.0f);
};

#endif  // NXDK_PGRAPH_TESTS_SRC_SHADERS_PERSPECTIVE_VERTEX_SHADER_NO_LIGHTING_H_
