#include "lighting_control_tests.h"

#include <pbkit/pbkit.h>

#include "debug_output.h"
#include "models/flat_grid_model.h"
#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "test_host.h"
#include "texture_generator.h"
#include "vertex_buffer.h"
#include "xbox_math_matrix.h"

static constexpr uint32_t kCheckerboardA = 0xFF202020;
static constexpr uint32_t kCheckerboardB = 0xFF000000;

static constexpr float PI_OVER_180 = (float)M_PI / 180.0f;
#define DEG2RAD(c) ((float)(c) * PI_OVER_180)

/**
 *
 */
LightingControlTests::LightingControlTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Lighting control", config) {
  tests_["foo"] = [this]() { this->Test("foo"); };
  // for (auto draw_mode : kDrawMode) {
  //   for (auto params : kTests) {
  //     std::string name = MakeTestName(params.set_normal, params.normal, draw_mode);
  //     tests_[name] = [this, params, draw_mode]() { this->Test(params.set_normal, params.normal, draw_mode); };
  //   }
  // }
}

void LightingControlTests::Deinitialize() { vertex_buffer_.reset(); }

static std::shared_ptr<VertexBuffer> CreateGeometry(TestHost& host) {
  vector_t diffuse{0.27f, 0.f, 0.27f, 0.75f};
  vector_t specular{0.f, 1.f, 0.f, 0.9f};
  auto model = FlatGridModel(diffuse, specular);
  auto vertex_buffer = host.AllocateVertexBuffer(model.GetVertexCount());

  matrix4_t translation_matrix;
  MatrixSetIdentity(translation_matrix);
  vector_t translate = {0.f, 0.f, 0.25f, 0.f};
  MatrixTranslate(translation_matrix, translate);
  model.PopulateVertexBuffer(vertex_buffer, (const float*)translation_matrix[0]);

  return vertex_buffer;
}

void LightingControlTests::Initialize() {
  TestSuite::Initialize();

  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  vertex_buffer_ = CreateGeometry(host_);
}

// static void SetLightAndMaterial() {
//   auto p = pb_begin();
//   p = pb_push1(p, NV097_SET_SPECULAR_PARAMS, 0xbf34dce5);
//   p = pb_push1(p, 0x09e4, 0xc020743f);
//   p = pb_push1(p, 0x09e8, 0x40333d06);
//   p = pb_push1(p, 0x09ec, 0xbf003612);
//   p = pb_push1(p, 0x09f0, 0xbff852a5);
//   p = pb_push1(p, 0x09f4, 0x401c1bce);
//
//   p = pb_push1(p, NV097_SET_LIGHT_LOCAL_RANGE, 0x7149f2ca);  // 1e30
//   p = pb_push3(p, NV097_SET_LIGHT_INFINITE_HALF_VECTOR, 0, 0, 0);
//   p = pb_push3f(p, NV097_SET_LIGHT_INFINITE_DIRECTION, 0.0f, 0.0f, 1.0f);
//
//   p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK, NV097_SET_LIGHT_ENABLE_MASK_LIGHT0_INFINITE);
//
//   pb_end(p);
// }

static void SetupLights(TestHost& host) {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, true);
  p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, true);

  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + (4 * NV2A_VERTEX_ATTR_SPECULAR), 0);
  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + (4 * NV2A_VERTEX_ATTR_BACK_DIFFUSE), 0xFFFFFFFF);
  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + (4 * NV2A_VERTEX_ATTR_BACK_SPECULAR), 0);

  p = pb_push3f(p, NV097_SET_SCENE_AMBIENT_COLOR, 0.031373, 0.031373, 0.031373);

  p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_ALL_FROM_MATERIAL);
  p = pb_push3(p, NV097_SET_MATERIAL_EMISSION, 0x0, 0x0, 0x0);
  p = pb_push1f(p, NV097_SET_MATERIAL_ALPHA, 0.75f);

  // Directional light.
  {
    vector_t light_dir{1.f, 0.f, 1.f, 1.f};
    VectorNormalize(light_dir);

    // Calculate the Blinn half vector.
    // https://paroj.github.io/gltut/Illumination/Tut11%20BlinnPhong%20Model.html

    // The look at vector from the default XDK initialization in TestHost::BuildDefaultXDKModelViewMatrix
    vector_t eye{0.0f, 0.0f, -7.0f, 1.0f};
    vector_t at{0.0f, 0.0f, 0.0f, 1.0f};
    vector_t look_dir{0.f, 0.f, 0.f, 1.f};
    VectorSubtractVector(at, eye, look_dir);
    VectorNormalize(look_dir);
    vector_t half_angle_vector{0.f, 0.f, 0.f, 1.f};
    VectorAddVector(look_dir, light_dir, half_angle_vector);
    VectorNormalize(half_angle_vector);

    // Infinite direction is the direction towards the light source.
    ScalarMultVector(light_dir, -1.f);
    ScalarMultVector(half_angle_vector, -1.f);

    p = pb_push3f(p, SET_LIGHT(0, NV097_SET_LIGHT_AMBIENT_COLOR), 0.f, 0.f, 0.5f);
    p = pb_push3f(p, SET_LIGHT(0, NV097_SET_LIGHT_DIFFUSE_COLOR), 1.f, 0.f, 0.f);
    p = pb_push3f(p, SET_LIGHT(0, NV097_SET_LIGHT_SPECULAR_COLOR), 0.f, 1.f, 0.f);
    p = pb_push1f(p, SET_LIGHT(0, NV097_SET_LIGHT_LOCAL_RANGE), 1e30f);
    p = pb_push3fv(p, SET_LIGHT(0, NV097_SET_LIGHT_INFINITE_HALF_VECTOR), half_angle_vector);
    p = pb_push3fv(p, SET_LIGHT(0, NV097_SET_LIGHT_INFINITE_DIRECTION), light_dir);
  }

  // Spotlight
  // {
  //   const matrix4_t& view_matrix = host.GetFixedFunctionModelViewMatrix();
  //   vector_t position{0.f, 0.f, -1.5f, 1.f};
  //   VectorMultMatrix(position, view_matrix);
  //   position[3] = 1.f;
  //
  //   p = pb_push3f(p, SET_LIGHT(1, NV097_SET_LIGHT_AMBIENT_COLOR), 0.f, 0.f, 0.5f);
  //   p = pb_push3f(p, SET_LIGHT(1, NV097_SET_LIGHT_DIFFUSE_COLOR), 1.f, 0.f, 0.f);
  //   p = pb_push3f(p, SET_LIGHT(1, NV097_SET_LIGHT_SPECULAR_COLOR), 0.f, 1.f, 0.f);
  //   p = pb_push1f(p, SET_LIGHT(1, NV097_SET_LIGHT_LOCAL_RANGE), 10.0f);
  //   p = pb_push3fv(p, SET_LIGHT(1, NV097_SET_LIGHT_LOCAL_POSITION), position);
  //   p = pb_push3f(p, SET_LIGHT(1, NV097_SET_LIGHT_LOCAL_ATTENUATION), 0.0f, 0.0f, 0.0f);
  //
  //   // falloff=1.000000: A=0.000000 B=-0.494592 C=1.494592
  //   p = pb_push3f(p, SET_LIGHT(1, NV097_SET_LIGHT_SPOT_FALLOFF), 0.f, -0.494592f, 1.494592f);
  //
  //   vector_t direction{0.25f, 0.f, 1.f, 1.f};
  //   VectorNormalize(direction);
  //
  //   float theta = DEG2RAD(15.f);
  //   float phi = DEG2RAD(45.f);
  //
  //   // See spotlight handling code here:
  //   // https://github.com/xemu-project/xemu/blob/15bf68594f186c3df876fa7856b0d7a13e80f0e1/hw/xbox/nv2a/shaders.c#L653
  //   float cos_half_theta = cosf(0.5f * theta);
  //   float cos_half_phi = cosf(0.5f * phi);
  //
  //   float inv_scale = -1.f / (cos_half_theta - cos_half_phi);
  //   ScalarMultVector(direction, inv_scale);
  //   direction[3] = cos_half_phi * inv_scale;
  //
  //   p = pb_push4fv(p, SET_LIGHT(1, NV097_SET_LIGHT_SPOT_DIRECTION), direction);
  // }

  // Point light
  {
    vector_t position{0.f, 0.f, -0.1f, 1.f};
    const matrix4_t& view_matrix = host.GetFixedFunctionModelViewMatrix();
    VectorMultMatrix(position, view_matrix);
    position[3] = 1.f;

    p = pb_push3f(p, SET_LIGHT(1, NV097_SET_LIGHT_AMBIENT_COLOR), 0.f, 0.f, 0.f);
    p = pb_push3f(p, SET_LIGHT(1, NV097_SET_LIGHT_DIFFUSE_COLOR), 1.f, 0.f, 0.f);
    p = pb_push3f(p, SET_LIGHT(1, NV097_SET_LIGHT_SPECULAR_COLOR), 0.f, 1.f, 0.f);
    p = pb_push1f(p, SET_LIGHT(1, NV097_SET_LIGHT_LOCAL_RANGE), 10.f);
    p = pb_push3fv(p, SET_LIGHT(1, NV097_SET_LIGHT_LOCAL_POSITION), position);
    // Attenuation = att0 + (att1 * d) + (att2 * d^2)
    p = pb_push3f(p, SET_LIGHT(1, NV097_SET_LIGHT_LOCAL_ATTENUATION), 0.15f, 0.5f, 0.75f);
  }

  p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK,
               NV097_SET_LIGHT_ENABLE_MASK_LIGHT0_INFINITE | NV097_SET_LIGHT_ENABLE_MASK_LIGHT1_LOCAL);

  pb_end(p);

  host.SetCombinerControl(1);
  host.SetInputColorCombiner(0, TestHost::SRC_DIFFUSE, false, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                             false, TestHost::MAP_UNSIGNED_INVERT);
  host.SetInputAlphaCombiner(0, TestHost::SRC_DIFFUSE, true, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO, false,
                             TestHost::MAP_UNSIGNED_INVERT);

  host.SetOutputColorCombiner(0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0);
  host.SetOutputAlphaCombiner(0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0);

  host.SetFinalCombiner0Just(TestHost::SRC_R0);
  host.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_R0, true,
                         false, /*specular_add_invert_r0*/ false, /* specular_add_invert_v1*/ false,
                         /* specular_clamp */ true);
}

static void DrawCheckerboardBackground(TestHost& host) {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, false);
  p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, false);
  pb_end(p);

  host.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
  host.SetTextureStageEnabled(0, true);
  host.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  static constexpr uint32_t kTextureSize = 256;
  static constexpr uint32_t kCheckerSize = 24;

  auto& texture_stage = host.GetTextureStage(0);
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
  texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
  host.SetupTextureStages();

  auto texture_memory = host.GetTextureMemoryForStage(0);
  GenerateSwizzledRGBACheckerboard(texture_memory, 0, 0, kTextureSize, kTextureSize, kTextureSize * 4, kCheckerboardA,
                                   kCheckerboardB, kCheckerSize);

  host.Begin(TestHost::PRIMITIVE_QUADS);
  host.SetTexCoord0(0.0f, 0.0f);
  vector_t world_point;
  vector_t screen_point = {0.f, 0.f, 0.0f, 1.0f};
  host.UnprojectPoint(world_point, screen_point, 1.f);
  host.SetVertex(world_point);

  host.SetTexCoord0(1.0f, 0.0f);
  screen_point[0] = host.GetFramebufferWidthF();
  host.UnprojectPoint(world_point, screen_point, 1.f);
  host.SetVertex(world_point);

  host.SetTexCoord0(1.0f, 1.0f);
  screen_point[1] = host.GetFramebufferHeightF();
  host.UnprojectPoint(world_point, screen_point, 1.f);
  host.SetVertex(world_point);

  host.SetTexCoord0(0.0f, 1.0f);
  screen_point[0] = 0.f;
  host.UnprojectPoint(world_point, screen_point, 1.f);
  host.SetVertex(world_point);

  host.End();

  host.SetTextureStageEnabled(0, false);
  host.SetShaderStageProgram(TestHost::STAGE_NONE);
}

void LightingControlTests::Test(const std::string& name) {
  static constexpr uint32_t kBackgroundColor = 0xFF232623;
  host_.PrepareDraw(kBackgroundColor);

  DrawCheckerboardBackground(host_);

  SetupLights(host_);

  host_.SetVertexBuffer(vertex_buffer_);
  host_.DrawArrays(host_.POSITION | host_.NORMAL | host_.DIFFUSE | host_.SPECULAR);

  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

void LightingControlTests::TestFixed(const std::string& name) {}
