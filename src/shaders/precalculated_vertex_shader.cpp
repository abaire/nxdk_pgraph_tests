#include "precalculated_vertex_shader.h"

#include <pbkit/pbkit.h>

// clang format off
static constexpr uint32_t kShader[] = {
#include "precalculated_vertex_shader.inl"
};
// clang format on

void PrecalculatedVertexShader::OnLoadShader() {
  LoadShaderProgram(kShader, sizeof(kShader));
}

void PrecalculatedVertexShader::OnLoadConstants() {
  auto p = pb_begin();

  /* Set shader constants cursor at C0 */
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_VP_UPLOAD_CONST_ID, 96);
  pb_end(p);
}
