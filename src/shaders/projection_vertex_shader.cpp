#include <pbkit/pbkit.h>

#include <cstdint>
#include <memory>

#include "math3d.h"
#include "perspective_vertex_shader.h"

// clang format off
static constexpr uint32_t kVertexShader[] = {
#include "projection_vertex_shader.inl"
};
// clang format on

static constexpr uint32_t kVertexShaderSize = sizeof(kVertexShader);

static void matrix_viewport(MATRIX out, float x, float y, float width, float height, float z_min, float z_max);

ProjectionVertexShader::ProjectionVertexShader(uint32_t framebuffer_width, uint32_t framebuffer_height, float z_min,
                                               float z_max, bool enable_texture)
    : ShaderProgram(enable_texture),
      framebuffer_width_(static_cast<float>(framebuffer_width)),
      framebuffer_height_(static_cast<float>(framebuffer_height)),
      z_min_(z_min),
      z_max_(z_max) {}

void ProjectionVertexShader::Activate() {
  UpdateMatrices();
  LoadShaderProgram(kVertexShader, kVertexShaderSize);
}

void ProjectionVertexShader::PrepareDraw() {
  /* Send shader constants
   *
   * WARNING: Changing shader source code may impact constant locations!
   * Check the intermediate file (*.inl) for the expected locations after
   * changing the code.
   */
  auto p = pb_begin();

  /* Set shader constants cursor at C0 */
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_VP_UPLOAD_CONST_ID, 96);

  /* Send the model matrix */
  pb_push(p++, NV20_TCL_PRIMITIVE_3D_VP_UPLOAD_CONST_X, 16);
  memcpy(p, model_matrix_, 16 * 4);
  p += 16;

  /* Send the view matrix */
  pb_push(p++, NV20_TCL_PRIMITIVE_3D_VP_UPLOAD_CONST_X, 16);
  memcpy(p, view_matrix_, 16 * 4);
  p += 16;

  /* Send the projection matrix */
  pb_push(p++, NV20_TCL_PRIMITIVE_3D_VP_UPLOAD_CONST_X, 16);
  memcpy(p, projection_viewport_matrix_, 16 * 4);
  p += 16;

  /* Send camera position */
  pb_push(p++, NV20_TCL_PRIMITIVE_3D_VP_UPLOAD_CONST_X, 4);
  memcpy(p, camera_position_, 4 * 4);
  p += 4;

  /* Send light direction */
  pb_push(p++, NV20_TCL_PRIMITIVE_3D_VP_UPLOAD_CONST_X, 4);
  memcpy(p, light_direction_, 4 * 4);
  p += 4;

  /* Send shader constants */
  float constants_0[4] = {0, 0, 0, 0};
  pb_push(p++, NV20_TCL_PRIMITIVE_3D_VP_UPLOAD_CONST_X, 4);
  memcpy(p, constants_0, 4 * 4);
  p += 4;

  /* Clear all attributes */
  pb_push(p++, NV097_SET_VERTEX_DATA_ARRAY_FORMAT, 16);
  for (auto i = 0; i < 16; i++) {
    *(p++) = NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F;
  }
  pb_end(p);
}

void ProjectionVertexShader::SetCamera(const VECTOR &position, const VECTOR &rotation) {
  memcpy(camera_position_, position, sizeof(camera_position_));
  memcpy(camera_rotation_, rotation, sizeof(camera_rotation_));
}

void ProjectionVertexShader::SetOmniLightDirection(const VECTOR &direction) {
  memcpy(light_direction_, direction, sizeof(light_direction_));
}

void ProjectionVertexShader::UpdateMatrices() {
  /* Create view matrix (our camera is static) */
  matrix_unit(view_matrix_);
  create_world_view(view_matrix_, camera_position_, camera_rotation_);

  CalculateProjectionMatrix();

  matrix_viewport(viewport_matrix_, 0, 0, framebuffer_width_, framebuffer_height_, z_min_, z_max_);
  matrix_multiply(projection_viewport_matrix_, projection_matrix_, viewport_matrix_);

  /* Create local->world matrix given our updated object */
  matrix_unit(model_matrix_);
}

/* Construct a viewport transformation matrix */
static void matrix_viewport(MATRIX out, float x, float y, float width, float height, float z_min, float z_max) {
  matrix_unit(out);
  out[0] = width / 2.0f;
  out[5] = height / -2.0f;
  out[10] = (z_max - z_min) / 2.0f;
  out[12] = x + width / 2.0f;
  out[13] = y + height / 2.0f;
  out[14] = (z_min + z_max) / 2.0f;
  out[15] = 1.0f;
}
