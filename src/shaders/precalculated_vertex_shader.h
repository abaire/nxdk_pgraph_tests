#ifndef NXDK_PGRAPH_TESTS_SHADERS_PRECALCULATED_VERTEX_SHADER_H_
#define NXDK_PGRAPH_TESTS_SHADERS_PRECALCULATED_VERTEX_SHADER_H_

#include <cstdint>

#include "../math3d.h"
#include "math3d.h"
#include "shader_program.h"

class PrecalculatedVertexShader : public ShaderProgram {
 public:
  explicit PrecalculatedVertexShader(bool enable_texture = true) : ShaderProgram((enable_texture)) {}

 protected:
  void OnLoadShader() override;
};

#endif  // NXDK_PGRAPH_TESTS_SHADERS_PRECALCULATED_VERTEX_SHADER_H_
