#include "fixed_function_approximation_shader.h"

// clang-format off
static const uint32_t kShader[] = {
#include "fixed_function_approximation_shader.vshinc"
};
// clang-format on

FixedFunctionApproximationShader::FixedFunctionApproximationShader(uint32_t framebuffer_width,
                                                                   uint32_t framebuffer_height, float z_min,
                                                                   float z_max, float fov_y, float near, float far)
    : PBKitPlusPlus::PerspectiveVertexShader(framebuffer_width, framebuffer_height, z_min, z_max, fov_y, near, far) {
  SetShader(kShader, sizeof(kShader));
  SetTransposeOnUpload(true);
}

void FixedFunctionApproximationShader::SetDirectionalLightDirection(const vector_t &direction) {
  memcpy(light_direction_, direction, sizeof(light_direction_));
}

void FixedFunctionApproximationShader::SetDirectionalLightCastDirection(const vector_t &direction) {
  ScalarMultVector(direction, -1.f, light_direction_);
}

int FixedFunctionApproximationShader::OnLoadConstants() {
  auto index = ProjectionVertexShader::OnLoadConstants();

  auto upload_vector = [this, &index](const vector_t &vec) {
    SetBaseUniform4F(index, vec);
    ++index;
  };

  upload_vector(light_direction_);

  vertex_t invalid_color{1.f, 0.f, 1.f, 1.f};
  upload_vector(invalid_color);

  return index;
}
