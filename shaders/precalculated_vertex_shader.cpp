#include "precalculated_vertex_shader.h"

#include <pbkit/pbkit.h>

// clang format off
static constexpr uint32_t kShader[] = {
#include "precalculated_vertex_shader.inl"
};
// clang format on

static constexpr uint32_t kShaderSize = sizeof(kShader);

PrecalculatedVertexShader::PrecalculatedVertexShader(bool enable_texture) : ShaderProgram(), enable_texture_(enable_texture) {}

void PrecalculatedVertexShader::Activate() {
  LoadShaderProgram(kShader, kShaderSize);
  if (enable_texture_) {
    LoadTexturedPixelShader();
  } else {
    LoadUntexturedPixelShader();
  }
}

void PrecalculatedVertexShader::PrepareDraw() {
  auto p = pb_begin();

  /* Set shader constants cursor at C0 */
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_VP_UPLOAD_CONST_ID, 96);

  /* Clear all attributes */
  pb_push(p++, NV097_SET_VERTEX_DATA_ARRAY_FORMAT, 16);
  for (auto i = 0; i < 16; i++) {
    *(p++) = NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F;
  }
  pb_end(p);
}
