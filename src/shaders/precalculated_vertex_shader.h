#ifndef NXDK_PGRAPH_TESTS_SHADERS_PRECALCULATED_VERTEX_SHADER_H_
#define NXDK_PGRAPH_TESTS_SHADERS_PRECALCULATED_VERTEX_SHADER_H_

#include <cstdint>

#include "vertex_shader_program.h"
#include "xbox_math_types.h"

using namespace XboxMath;

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
