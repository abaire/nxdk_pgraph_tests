#include "passthrough_vertex_shader.h"

#include <pbkit/pbkit.h>

// clang-format off
static const uint32_t kPassthroughVsh[] = {
#include "passthrough.vshinc"
};
// clang-format on

void PassthroughVertexShader::OnLoadShader() {
  const uint32_t *shader;
  uint32_t shader_len;

  shader = kPassthroughVsh;
  shader_len = sizeof(kPassthroughVsh);

  LoadShaderProgram(shader, shader_len);
}
