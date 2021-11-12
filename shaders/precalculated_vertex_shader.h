#ifndef NXDK_PGRAPH_TESTS_SHADERS_PRECALCULATED_VERTEX_SHADER_H_
#define NXDK_PGRAPH_TESTS_SHADERS_PRECALCULATED_VERTEX_SHADER_H_

#include <cstdint>

#include "../math3d.h"
#include "math3d.h"
#include "shader_program.h"

class PrecalculatedVertexShader : public ShaderProgram {
 public:
  PrecalculatedVertexShader(bool enable_texture = true);

  void Activate() override;
  void PrepareDraw() override;

 protected:
  bool enable_texture_;
};

#endif  // NXDK_PGRAPH_TESTS_SHADERS_PRECALCULATED_VERTEX_SHADER_H_
