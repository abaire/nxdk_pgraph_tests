#include "projection_vertex_shader.h"

#include <pbkit/pbkit.h>

#include <cstdint>
#include <memory>

#include "debug_output.h"
#include "nxdk/samples/mesh/math3d.h"
#include "xbox_math_d3d.h"
#include "xbox_math_matrix.h"
#include "xbox_math_types.h"
#include "xbox_math_util.h"

using namespace XboxMath;

// clang-format off
static constexpr uint32_t kVertexShaderLighting[] = {
#include "projection_vertex_shader.inl"
};

static constexpr uint32_t kVertexShaderNoLighting[] = {
#include "projection_vertex_shader_no_lighting.inl"
};

static constexpr uint32_t kVertexShaderNoLighting4ComponentTexcoord[] = {
#include "projection_vertex_shader_no_lighting_4c_texcoords.inl"
};
// clang-format on

ProjectionVertexShader::ProjectionVertexShader(uint32_t framebuffer_width, uint32_t framebuffer_height, float z_min,
                                               float z_max, bool enable_lighting, bool use_4_component_texcoords)
    : VertexShaderProgram(),
      framebuffer_width_(static_cast<float>(framebuffer_width)),
      framebuffer_height_(static_cast<float>(framebuffer_height)),
      z_min_(z_min),
      z_max_(z_max),
      enable_lighting_{enable_lighting},
      use_4_component_texcoords_{use_4_component_texcoords} {
  MatrixSetIdentity(model_matrix_);
  MatrixSetIdentity(view_matrix_);

  vector_t rot = {0, 0, 0, 1};
  CreateWorldView(camera_position_, rot, view_matrix_);
}

void ProjectionVertexShader::LookAt(const vector_t &camera_position, const vector_t &look_at_point,
                                    const vector_t &up) {
  vector_t direction;
  direction[0] = look_at_point[0] - camera_position[0];
  direction[1] = look_at_point[1] - camera_position[1];
  direction[2] = look_at_point[2] - camera_position[2];
  direction[3] = 1.0f;

  LookTo(camera_position, direction, up);
}

void ProjectionVertexShader::LookTo(const vector_t &camera_position, const vector_t &camera_direction,
                                    const vector_t &up) {
  memcpy(camera_position_, camera_position, sizeof(camera_position_));

  vector_t z_axis;
  z_axis[3] = 1.0f;
  VectorNormalize(camera_direction, z_axis);

  vector_t x_axis{0.0f, 0.0f, 0.0f, 1.0f};
  VectorCrossVector(up, z_axis, x_axis);
  VectorNormalize(x_axis);

  vector_t y_axis;
  y_axis[3] = 1.0f;
  VectorCrossVector(z_axis, x_axis, y_axis);

  memset(view_matrix_, 0, sizeof(view_matrix_));
  view_matrix_[0][0] = x_axis[0];
  view_matrix_[0][1] = y_axis[0];
  view_matrix_[0][2] = z_axis[0];
  view_matrix_[0][3] = 0.0f;

  view_matrix_[1][0] = x_axis[1];
  view_matrix_[1][1] = y_axis[1];
  view_matrix_[1][2] = z_axis[1];
  view_matrix_[1][3] = 0.0f;

  view_matrix_[2][0] = x_axis[2];
  view_matrix_[2][1] = y_axis[2];
  view_matrix_[2][2] = z_axis[2];
  view_matrix_[2][3] = 0.0f;

  view_matrix_[3][0] = -VectorDotVector(x_axis, camera_position);
  view_matrix_[3][1] = -VectorDotVector(y_axis, camera_position);
  view_matrix_[3][2] = -VectorDotVector(z_axis, camera_position);
  view_matrix_[3][3] = 1.0f;

  UpdateMatrices();
}

void ProjectionVertexShader::SetCamera(const vector_t &position, const vector_t &rotation) {
  memcpy(camera_position_, position, sizeof(camera_position_));

  MatrixSetIdentity(view_matrix_);
  CreateWorldView(camera_position_, rotation, view_matrix_);
  UpdateMatrices();
}

void ProjectionVertexShader::SetDirectionalLightDirection(const vector_t &direction) {
  memcpy(light_direction_, direction, sizeof(light_direction_));
}

void ProjectionVertexShader::SetDirectionalLightCastDirection(const vector_t &direction) {
  ScalarMultVector(direction, -1.f, light_direction_);
}

void ProjectionVertexShader::UpdateMatrices() {
  CalculateProjectionMatrix();
  CalculateViewportMatrix();
  MatrixMultMatrix(projection_matrix_, viewport_matrix_, projection_viewport_matrix_);

  matrix4_t model_view_matrix;
  MatrixMultMatrix(model_matrix_, view_matrix_, model_view_matrix);

  MatrixMultMatrix(model_view_matrix, projection_viewport_matrix_, composite_matrix_);
  MatrixInvert(composite_matrix_, inverse_composite_matrix_);
}

void ProjectionVertexShader::OnActivate() { UpdateMatrices(); }

void ProjectionVertexShader::OnLoadShader() {
  if (enable_lighting_) {
    LoadShaderProgram(kVertexShaderLighting, sizeof(kVertexShaderLighting));
  } else {
    if (use_4_component_texcoords_) {
      LoadShaderProgram(kVertexShaderNoLighting4ComponentTexcoord, sizeof(kVertexShaderNoLighting4ComponentTexcoord));
    } else {
      LoadShaderProgram(kVertexShaderNoLighting, sizeof(kVertexShaderLighting));
    }
  }
}

void ProjectionVertexShader::OnLoadConstants() {
  /* Send shader constants
   *
   * WARNING: Changing shader source code may impact constant locations!
   * Check the intermediate file (*.inl) for the expected locations after
   * changing the code.
   */

  int index = 0;
  auto upload_matrix = [this, &index](const matrix4_t &matrix) {
    if (transpose_on_upload_) {
      matrix4_t temp;
      MatrixTranspose(matrix, temp);
      SetBaseUniform4x4F(index, temp);
    } else {
      SetBaseUniform4x4F(index, matrix);
    }
    index += 4;
  };

  auto upload_vector = [this, &index](const vector_t &vec) {
    SetBaseUniform4F(index, vec);
    ++index;
  };

  // In performance critical code or shaders where space matters, these matrices would all be premultiplied.
  upload_matrix(model_matrix_);
  upload_matrix(view_matrix_);
  upload_matrix(projection_viewport_matrix_);

  upload_vector(camera_position_);

  if (enable_lighting_) {
    upload_vector(light_direction_);
  }

  // Send shader constants
  float constants_0[4] = {0, 0, 0, 0};
  upload_vector(constants_0);
}

void ProjectionVertexShader::CalculateViewportMatrix() {
  if (use_d3d_style_viewport_) {
    // TODO: Support alternative screen space Z range and take in the max depthbuffer value separately.
    // This should mirror the `create_d3d_viewport` parameters. In practice none of the tests use a range other than
    // 0..1 so this is not currently implemented and z_far is understood to contain the maximum depthbuffer value.
    ASSERT(z_min_ == 0.0f && "Viewport z-range only implemented for 0..1");
    CreateD3DViewport(viewport_matrix_, framebuffer_width_, framebuffer_height_, z_max_, 0.0f, 1.0f);
  } else {
    MatrixSetIdentity(viewport_matrix_);
    viewport_matrix_[0][0] = framebuffer_width_ * 0.5f;
    viewport_matrix_[3][0] = viewport_matrix_[0][0];
    viewport_matrix_[3][1] = framebuffer_height_ * 0.5f;
    viewport_matrix_[1][1] = -1.0f * viewport_matrix_[3][1];

    viewport_matrix_[2][2] = (z_max_ - z_min_) * 0.5f;
    viewport_matrix_[3][2] = (z_min_ + z_max_) * 0.5f;
  }
}

void ProjectionVertexShader::ProjectPoint(vector_t &result, const vector_t &world_point) const {
  XboxMath::ProjectPoint(world_point, composite_matrix_, result);
}

void ProjectionVertexShader::UnprojectPoint(vector_t &result, const vector_t &screen_point) const {
  XboxMath::UnprojectPoint(screen_point, inverse_composite_matrix_, result);
}

void ProjectionVertexShader::UnprojectPoint(vector_t &result, const vector_t &screen_point, float world_z) const {
  XboxMath::UnprojectPoint(screen_point, inverse_composite_matrix_, world_z, result);
}
