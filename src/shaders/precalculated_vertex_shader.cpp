#include "precalculated_vertex_shader.h"

#include <pbkit/pbkit.h>

// clang-format off
static constexpr uint32_t k2ComponentTexcoords[] = {
#include "precalculated_vertex_shader_2c_texcoords.inl"
};

static constexpr uint32_t k4ComponentTexcoords[] = {
#include "precalculated_vertex_shader_4c_texcoords.inl"
};
// clang-format on

void PrecalculatedVertexShader::OnLoadShader() {
  const uint32_t *shader;
  uint32_t shader_len;

  if (use_4_component_texcoords_) {
    shader = k4ComponentTexcoords;
    shader_len = sizeof(k4ComponentTexcoords);
  } else {
    shader = k2ComponentTexcoords;
    shader_len = sizeof(k2ComponentTexcoords);
  }

  LoadShaderProgram(shader, shader_len);
}
