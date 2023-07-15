#include "projection_vertex_shader.h"

#include <pbkit/pbkit.h>

#include <cstdint>
#include <memory>

#include "debug_output.h"
#include "nxdk/samples/mesh/math3d.h"
#include "xbox_math_d3d.h"
#include "xbox_math_matrix.h"
#include "xbox_math_types.h"

using namespace XboxMath;

// clang format off
static constexpr uint32_t kVertexShaderLighting[] = {
#include "projection_vertex_shader.inl"
};

static constexpr uint32_t kVertexShaderNoLighting[] = {
#include "projection_vertex_shader_no_lighting.inl"
};

static constexpr uint32_t kVertexShaderNoLighting4ComponentTexcoord[] = {
#include "projection_vertex_shader_no_lighting_4c_texcoords.inl"
};
// clang format on

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

  vector_t x_axis_work;
  x_axis_work[3] = 1.0f;
  VectorCrossVector(up, z_axis, x_axis_work);
  vector_t x_axis{0.0f, 0.0f, 0.0f, 1.0f};
  VectorNormalize(x_axis_work, x_axis);

  vector_t y_axis;
  y_axis[3] = 1.0f;
  VectorCrossVector(z_axis, x_axis_work, y_axis);

  memset(view_matrix_, 0, sizeof(view_matrix_));
  view_matrix_[0][0] = x_axis_work[0];
  view_matrix_[0][1] = y_axis[0];
  view_matrix_[0][2] = z_axis[0];
  view_matrix_[0][3] = 0.0f;

  view_matrix_[1][0] = x_axis_work[1];
  view_matrix_[1][1] = y_axis[1];
  view_matrix_[1][2] = z_axis[1];
  view_matrix_[1][3] = 0.0f;

  view_matrix_[2][0] = x_axis_work[2];
  view_matrix_[2][1] = y_axis[2];
  view_matrix_[2][2] = z_axis[2];
  view_matrix_[2][3] = 0.0f;

  view_matrix_[3][0] = -VectorDotVector(x_axis_work, camera_position);
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

void ProjectionVertexShader::UpdateMatrices() {
  CalculateProjectionMatrix();
  CalculateViewportMatrix();
  MatrixMultMatrix(projection_matrix_, viewport_matrix_, projection_viewport_matrix_);

  matrix4_t model_view_matrix;
  MatrixMultMatrix(model_matrix_, view_matrix_, model_view_matrix);

  MatrixMultMatrix(model_view_matrix, projection_viewport_matrix_, composite_matrix_);
  MatrixTranspose(composite_matrix_);
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
  SetBaseUniform4x4F(index, model_matrix_);
  index += 4;
  SetBaseUniform4x4F(index, view_matrix_);
  index += 4;
  SetBaseUniform4x4F(index, projection_viewport_matrix_);
  index += 4;
  SetBaseUniform4F(index, camera_position_);
  ++index;

  if (enable_lighting_) {
    SetBaseUniform4F(index, light_direction_);
    ++index;
  }

  // Send shader constants
  float constants_0[4] = {0, 0, 0, 0};
  SetBaseUniform4F(index, constants_0);
  ++index;
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
  vector_t screen_point;
  VectorMultMatrix(world_point, composite_matrix_, screen_point);

  result[0] = screen_point[0] / screen_point[3];
  result[1] = screen_point[1] / screen_point[3];
  result[2] = screen_point[2] / screen_point[3];
  result[3] = 1.0f;
}

void ProjectionVertexShader::UnprojectPoint(vector_t &result, const vector_t &screen_point) const {
  VectorMultMatrix(screen_point, inverse_composite_matrix_, result);
}

void ProjectionVertexShader::UnprojectPoint(vector_t &result, const vector_t &screen_point, float world_z) const {
  vector_t work;
  VectorCopyVector(work, screen_point);

  // TODO: Get the near and far plane mappings from the viewport matrix.
  work[2] = 0.0f;
  vector_t near_plane;
  VectorMultMatrix(work, inverse_composite_matrix_, near_plane);
  VectorEuclidean(near_plane);

  work[2] = 64000.0f;
  vector_t far_plane;
  VectorMultMatrix(work, inverse_composite_matrix_, far_plane);
  VectorEuclidean(far_plane);

  float t = (world_z - near_plane[2]) / (far_plane[2] - near_plane[2]);
  result[0] = near_plane[0] + (far_plane[0] - near_plane[0]) * t;
  result[1] = near_plane[1] + (far_plane[1] - near_plane[1]) * t;
  result[2] = world_z;
  result[3] = 1.0f;
}
