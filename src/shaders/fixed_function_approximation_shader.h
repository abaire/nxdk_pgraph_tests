#ifndef NXDK_PGRAPH_TESTS_SRC_SHADERS_FIXED_FUNCTION_APPROXIMATION_SHADER_H_
#define NXDK_PGRAPH_TESTS_SRC_SHADERS_FIXED_FUNCTION_APPROXIMATION_SHADER_H_

#include "shaders/perspective_vertex_shader.h"
#include "xbox_math_matrix.h"

using namespace XboxMath;

//! Programmable vertex shader that mimicks the behavior of the fixed function pipeline.
class FixedFunctionApproximationShader : public PBKitPlusPlus::PerspectiveVertexShader {
 public:
  FixedFunctionApproximationShader(uint32_t framebuffer_width, uint32_t framebuffer_height, float z_min = 0,
                                   float z_max = 0x7FFF, float fov_y = M_PI * 0.25f, float near = 1.0f,
                                   float far = 10.0f);

  //! Sets the direction from the origin towards the directional light.
  void SetDirectionalLightDirection(const vector_t &direction);

  //! Sets the direction in which the directional light is casting light.
  void SetDirectionalLightCastDirection(const vector_t &direction);

 protected:
  int OnLoadConstants() override;

 private:
  vector_t light_direction_ = {0, 0, 1, 1};
};

#endif  // NXDK_PGRAPH_TESTS_SRC_SHADERS_FIXED_FUNCTION_APPROXIMATION_SHADER_H_
