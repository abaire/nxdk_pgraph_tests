#include "lighting_specular_tests.h"

#include <pbkit/pbkit.h>

#include "debug_output.h"
#include "models/distorted_cylinder.h"
#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "test_host.h"
#include "vertex_buffer.h"
#include "xbox_math_matrix.h"
#include "xbox_math_types.h"
#include "xbox_math_vector.h"

using namespace XboxMath;

static const bool kSpecularEnabled[] = {true, false};

LightingSpecularTests::LightingSpecularTests(TestHost& host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Lighting Specular") {
  for (auto specular_enabled : kSpecularEnabled) {
    std::string name = MakeTestName(specular_enabled);
    tests_[name] = [this, specular_enabled]() { this->Test(specular_enabled); };
  }
}

static void SetPointLight() {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK, NV097_SET_LIGHT_ENABLE_MASK_LIGHT0_LOCAL);

  p = pb_push3(p, NV097_SET_LIGHT_AMBIENT_COLOR, 0, 0, 0);
  p = pb_push3f(p, NV097_SET_LIGHT_DIFFUSE_COLOR, 1.f, 0.f, 0.f);
  p = pb_push3f(p, NV097_SET_LIGHT_SPECULAR_COLOR, 0.f, 1.f, 0.f);

  p = pb_push1f(p, NV097_SET_LIGHT_LOCAL_RANGE, 10.f);
  p = pb_push3f(p, NV097_SET_LIGHT_LOCAL_POSITION, -1.f, 0.25f, 4.f);
  p = pb_push3f(p, NV097_SET_LIGHT_LOCAL_ATTENUATION, 0.25f, 0.5f, 0.25f);
  pb_end(p);
}

static void SetLightAndMaterial(const matrix4_t& view_matrix) {
  auto p = pb_begin();

  p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_DIFFUSE_FROM_VERTEX_DIFFUSE);
  p = pb_push3f(p, NV097_SET_SCENE_AMBIENT_COLOR, 0.031373, 0.031373, 0.031373);
  p = pb_push3(p, NV097_SET_MATERIAL_EMISSION, 0x0, 0x0, 0x0);
  p = pb_push1f(p, NV097_SET_MATERIAL_ALPHA, 1.0f);
  pb_end(p);

  SetPointLight();

  // Set up a directional light.
  //  vector_t light_dir{0.f, 0.f, 1.f, 1.f};
  //  vector_t light_infinite_direction;
  //  VectorMultMatrix(light_dir, view_matrix, light_infinite_direction);
  //  light_infinite_direction[3] = 1.f;
  //  VectorNormalize(light_infinite_direction);
  //
  //  p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK, NV097_SET_LIGHT_ENABLE_MASK_LIGHT0_INFINITE);
  //  p = pb_push3fv(p, NV097_SET_LIGHT_INFINITE_DIRECTION, light_infinite_direction);
  //
  //  // Calculate the Blinn half vector.
  //  // https://paroj.github.io/gltut/Illumination/Tut11%20BlinnPhong%20Model.html
  //  {
  //    // The look at vector from the default XDK initialization in TestHost::BuildDefaultXDKModelViewMatrix
  //    vector_t at{0.0f, 0.0f, 0.0f, 1.0f};
  //    vector_t half_angle_vector;
  //    VectorAddVector(at, light_infinite_direction, half_angle_vector);
  //    VectorNormalize(half_angle_vector);
  //
  //    p = pb_push3fv(p, NV097_SET_LIGHT_INFINITE_HALF_VECTOR, half_angle_vector);
  //  }
}

void LightingSpecularTests::Initialize() {
  TestSuite::Initialize();

  host_.SetVertexShaderProgram(nullptr);
  CreateGeometry();
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, true);

    p = pb_push4f(p, NV097_SET_VERTEX_DATA4F_M + (4 * NV2A_VERTEX_ATTR_DIFFUSE), 1.f, 0.f, 0.f, 1.f);
    p = pb_push4f(p, NV097_SET_VERTEX_DATA4F_M + (4 * NV2A_VERTEX_ATTR_SPECULAR), 0.f, 1.f, 0.f, 1.f);
    p = pb_push4f(p, NV097_SET_VERTEX_DATA4F_M + (4 * NV2A_VERTEX_ATTR_BACK_DIFFUSE), 0.f, 0.f, 1.f, 1.f);
    p = pb_push4f(p, NV097_SET_VERTEX_DATA4F_M + (4 * NV2A_VERTEX_ATTR_BACK_SPECULAR), 0.5f, 0.f, 0.5f, 1.f);

    p = pb_push1(p, NV097_SET_CULL_FACE_ENABLE, true);
    p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, true);

    pb_end(p);
  }
  host_.SetCombinerControl(1);
  host_.SetInputColorCombiner(0, TestHost::SRC_DIFFUSE, false, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                              false, TestHost::MAP_UNSIGNED_INVERT);
  host_.SetInputAlphaCombiner(0, TestHost::SRC_DIFFUSE, true, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                              false, TestHost::MAP_UNSIGNED_INVERT);

  host_.SetOutputColorCombiner(0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0);
  host_.SetOutputAlphaCombiner(0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0);

  host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_R0, true,
                          false, /*specular_add_invert_r0*/ false, /* specular_add_invert_v1*/ false,
                          /* specular_clamp */ true);

  // The model view matrix is used as a view matrix since there is no model component by default.
  SetLightAndMaterial(host_.GetFixedFunctionModelViewMatrix());

  {
    auto p = pb_begin();
    p = pb_push1(p, 0x3b8 /* NV097_SET_SPECULAR_ENABLE */, 0x1);  // NV097_SET_SPECULAR_ENABLE_V_TRUE<0x1>
    pb_end(p);
  }
}

void LightingSpecularTests::Deinitialize() {
  vertex_buffer_.reset();
  TestSuite::Deinitialize();
}

void LightingSpecularTests::CreateGeometry() {
  vector_t diffuse{0.27f, 0.f, 0.27f, 1.f};
  vector_t specular{0.f, 0.f, 0.f, 1.f};
  auto model = DistortedCylinder(diffuse, specular);
  vertex_buffer_ = host_.AllocateVertexBuffer(model.GetVertexCount());
  matrix4_t translation_matrix;
  MatrixSetIdentity(translation_matrix);
  vector_t translate = {0.5, 0.0, 2.0, 0.0};
  MatrixTranslate(translation_matrix, translate);

  model.PopulateVertexBuffer(vertex_buffer_);  //, (const float*)translation_matrix[0]);
}

void LightingSpecularTests::Test(bool specular_enabled) {
  static constexpr uint32_t kBackgroundColor = 0xFF404040;
  host_.PrepareDraw(kBackgroundColor);

  // Specular enable/disable consists of a few settings
  // Testing with a point light source:
  // 1) NV097_SET_SPECULAR_ENABLE
  // 2) NV097_SET_COMBINER_SPECULAR_FOG_CW should use [D: SRC_SPEC_R0_SUM] if enabled (or the default [D: SRC_R0] if
  //    not)
  // 3) NV097_SET_LIGHT_CONTROL is set to 0x10001 when enabled, 0x1 when not.
  //
  // 6 values are also written to 0x9E0

  {  // D3D doesn't seem to toggle this specular setting when disabling specular lighting. It instead drops the
     // SPEC_R0_SUM
    // from the combiner, leaving off the specular element.
    // To reproduce D3D behavior, comment out this block.
    //
    // When specular is disabled here, it seems to go full-bright into R0.
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, specular_enabled);
    pb_end(p);
  }

  if (specular_enabled) {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 0, 0x80000000);   // -0.0
    p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 4, 0x3F800000);   // 1.0
    p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 8, 0x00000000);   // 0.0
    p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 12, 0x80000000);  // -0.0
    p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 16, 0x3F800000);  // 1.0
    p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 20, 0x00000000);  // 0.0
    p = pb_push1(p, NV097_SET_LIGHT_CONTROL, 0x10001);
    pb_end(p);
    host_.SetFinalCombiner0Just(TestHost::SRC_SPEC_R0_SUM);
  } else {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_LIGHT_CONTROL, 0x1);
    pb_end(p);
    host_.SetFinalCombiner0Just(TestHost::SRC_R0);
  }

  host_.SetVertexBuffer(vertex_buffer_);
  host_.DrawArrays(host_.POSITION | host_.NORMAL | host_.DIFFUSE | host_.SPECULAR);

  std::string name = MakeTestName(specular_enabled);
  pb_print("%s", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, name);
}

std::string LightingSpecularTests::MakeTestName(bool specular_enabled) {
  char buf[128] = {0};
  snprintf(buf, sizeof(buf), "Specular%s", specular_enabled ? "On" : "Off");
  return buf;
}
