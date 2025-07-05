#include "perspective_vertex_shader_no_lighting.h"

// clang-format off
static constexpr uint32_t kShader[] = {
#include "projection_vertex_shader_no_lighting.vshinc"
};
// clang-format on

PerspectiveVertexShaderNoLighting::PerspectiveVertexShaderNoLighting(uint32_t framebuffer_width,
                                                                     uint32_t framebuffer_height, float z_min,
                                                                     float z_max, float fov_y, float near, float far)
    : PBKitPlusPlus::PerspectiveVertexShader(framebuffer_width, framebuffer_height, z_min, z_max, fov_y, near, far) {
  SetShader(kShader, sizeof(kShader));
  SetTransposeOnUpload(true);
}
