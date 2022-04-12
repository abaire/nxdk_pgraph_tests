#ifndef NXDK_PGRAPH_TESTS_SHADERS_PRECALCULATED_VERTEX_SHADER_H_
#define NXDK_PGRAPH_TESTS_SHADERS_PRECALCULATED_VERTEX_SHADER_H_

#include <cstdint>

#include "../math3d.h"
#include "math3d.h"
#include "vertex_shader_program.h"

class PrecalculatedVertexShader : public VertexShaderProgram {
 public:
  explicit PrecalculatedVertexShader(bool use_4c_texcoords = false)
      : VertexShaderProgram(), use_4_component_texcoords_(use_4c_texcoords){};

 protected:
  void OnLoadShader() override;

 protected:
  bool use_4_component_texcoords_;
};

#endif  // NXDK_PGRAPH_TESTS_SHADERS_PRECALCULATED_VERTEX_SHADER_H_
