#ifndef NXDK_PGRAPH_TESTS_SHADERS_PRECALCULATED_VERTEX_SHADER_H_
#define NXDK_PGRAPH_TESTS_SHADERS_PRECALCULATED_VERTEX_SHADER_H_

#include <cstdint>

#include "../math3d.h"
#include "math3d.h"
#include "vertex_shader_program.h"

class PrecalculatedVertexShader : public VertexShaderProgram {
 public:
  explicit PrecalculatedVertexShader() : VertexShaderProgram() {}

 protected:
  void OnLoadShader() override;
};

#endif  // NXDK_PGRAPH_TESTS_SHADERS_PRECALCULATED_VERTEX_SHADER_H_
