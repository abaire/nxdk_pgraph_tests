#include "precalculated_vertex_shader.h"

#include <pbkit/pbkit.h>

// clang format off
static constexpr uint32_t kShader[] = {
#include "precalculated_vertex_shader.inl"
};
// clang format on

void PrecalculatedVertexShader::OnLoadShader() { LoadShaderProgram(kShader, sizeof(kShader)); }
