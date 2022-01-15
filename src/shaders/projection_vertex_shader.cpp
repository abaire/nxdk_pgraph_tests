#include "projection_vertex_shader.h"

#include <pbkit/pbkit.h>

#include <cstdint>
#include <memory>

#include "math3d.h"

// clang format off
static constexpr uint32_t kVertexShaderLighting[] = {
#include "projection_vertex_shader.inl"
};

static constexpr uint32_t kVertexShaderNoLighting[] = {
#include "projection_vertex_shader_no_lighting.inl"
};
// clang format on

static void matrix_viewport(MATRIX out, float x, float y, float width, float height, float z_min, float z_max);

ProjectionVertexShader::ProjectionVertexShader(uint32_t framebuffer_width, uint32_t framebuffer_height, float z_min,
                                               float z_max, bool enable_texture, bool enable_lighting)
    : ShaderProgram(enable_texture),
      framebuffer_width_(static_cast<float>(framebuffer_width)),
      framebuffer_height_(static_cast<float>(framebuffer_height)),
      z_min_(z_min),
      z_max_(z_max),
      enable_lighting_{enable_lighting} {
  matrix_unit(view_matrix_);

  VECTOR rot = {0, 0, 0, 1};
  create_world_view(view_matrix_, camera_position_, rot);
}

void ProjectionVertexShader::Activate() {
  UpdateMatrices();

  if (enable_lighting_) {
    LoadShaderProgram(kVertexShaderLighting, sizeof(kVertexShaderLighting));
  } else {
    LoadShaderProgram(kVertexShaderNoLighting, sizeof(kVertexShaderLighting));
  }
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

  if (enable_lighting_) {
    /* Send light direction */
    pb_push(p++, NV20_TCL_PRIMITIVE_3D_VP_UPLOAD_CONST_X, 4);
    memcpy(p, light_direction_, 4 * 4);
    p += 4;
  }

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

void ProjectionVertexShader::LookAt(const float *camera_position, const float *look_at_point, const float *up) {
  VECTOR direction;
  direction[0] = look_at_point[0] - camera_position[0];
  direction[1] = look_at_point[1] - camera_position[1];
  direction[2] = look_at_point[2] - camera_position[2];
  direction[3] = 1.0f;

  LookTo(camera_position, direction, up);
}

void ProjectionVertexShader::LookTo(const float *camera_position, const float *camera_direction, const float *up) {
  memcpy(camera_position_, camera_position, sizeof(camera_position_));

  VECTOR z_axis;
  z_axis[3] = 1.0f;
  vector_normalize_into(z_axis, const_cast<float *>(camera_direction));

  VECTOR x_axis_work;
  x_axis_work[3] = 1.0f;
  vector_outerproduct(x_axis_work, const_cast<float *>(up), z_axis);
  VECTOR x_axis{0.0f, 0.0f, 0.0f, 1.0f};
  vector_normalize_into(x_axis, x_axis_work);

  VECTOR y_axis;
  y_axis[3] = 1.0f;
  vector_outerproduct(y_axis, z_axis, x_axis_work);

  memset(view_matrix_, 0, sizeof(view_matrix_));
  view_matrix_[_11] = x_axis_work[0];
  view_matrix_[_12] = y_axis[0];
  view_matrix_[_13] = z_axis[0];
  view_matrix_[_14] = 0.0f;

  view_matrix_[_21] = x_axis_work[1];
  view_matrix_[_22] = y_axis[1];
  view_matrix_[_23] = z_axis[1];
  view_matrix_[_24] = 0.0f;

  view_matrix_[_31] = x_axis_work[2];
  view_matrix_[_32] = y_axis[2];
  view_matrix_[_33] = z_axis[2];
  view_matrix_[_34] = 0.0f;

  view_matrix_[_41] = -vector_innerproduct(x_axis_work, const_cast<float *>(camera_position));
  view_matrix_[_42] = -vector_innerproduct(y_axis, const_cast<float *>(camera_position));
  view_matrix_[_43] = -vector_innerproduct(z_axis, const_cast<float *>(camera_position));
  view_matrix_[_44] = 1.0f;
}

void ProjectionVertexShader::SetCamera(const VECTOR position, const VECTOR rotation) {
  memcpy(camera_position_, position, sizeof(camera_position_));

  matrix_unit(view_matrix_);
  create_world_view(view_matrix_, camera_position_, rotation);
}

void ProjectionVertexShader::SetDirectionalLightDirection(const VECTOR &direction) {
  memcpy(light_direction_, direction, sizeof(light_direction_));
}

void ProjectionVertexShader::UpdateMatrices() {
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
