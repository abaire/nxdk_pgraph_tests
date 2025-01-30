#include "lighting_two_sided_tests.h"

#include <pbkit/pbkit.h>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "test_host.h"
#include "vertex_buffer.h"

static constexpr char kTestName[] = "TwoSidedLighting";

LightingTwoSidedTests::LightingTwoSidedTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Lighting Two Sided", config) {
  tests_[kTestName] = [this]() { this->Test(); };
}

static void SetLight() {
  auto p = pb_begin();

  p = pb_push3(p, NV097_SET_LIGHT_AMBIENT_COLOR, 0, 0, 0);
  p = pb_push3f(p, NV097_SET_LIGHT_DIFFUSE_COLOR, 1.f, 0.f, 0.25f);
  p = pb_push3f(p, NV097_SET_LIGHT_SPECULAR_COLOR, 0.f, 1.f, 0.f);

  p = pb_push3f(p, NV097_SET_BACK_LIGHT_AMBIENT_COLOR, 0, 0, 0.25f);
  p = pb_push3f(p, NV097_SET_BACK_LIGHT_DIFFUSE_COLOR, 0.f, 0.75f, 0.35f);
  p = pb_push3f(p, NV097_SET_BACK_LIGHT_SPECULAR_COLOR, 0.66f, 0.f, 0.66f);

  //  p = pb_push1f(p, NV097_SET_LIGHT_LOCAL_RANGE, 10.f);
  //  p = pb_push3f(p, NV097_SET_LIGHT_LOCAL_POSITION, 0.f, 0.f, 2.f);
  //  p = pb_push3f(p, NV097_SET_LIGHT_LOCAL_ATTENUATION, 0.25f, 0.5f, 0.25f);
  //  p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK, NV097_SET_LIGHT_ENABLE_MASK_LIGHT0_LOCAL);

  p = pb_push1f(p, NV097_SET_LIGHT_LOCAL_RANGE, 1e30f);
  p = pb_push3(p, NV097_SET_LIGHT_INFINITE_HALF_VECTOR, 0, 0, 0);
  p = pb_push3f(p, NV097_SET_LIGHT_INFINITE_DIRECTION, 0.0f, 0.0f, 1.0f);
  p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK, NV097_SET_LIGHT_ENABLE_MASK_LIGHT0_INFINITE);

  pb_end(p);
}

static void SetLightAndMaterial() {
  auto p = pb_begin();

  p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_ALL_FROM_MATERIAL);
  p = pb_push3f(p, NV097_SET_SCENE_AMBIENT_COLOR, 0.031373, 0.031373, 0.031373);
  p = pb_push3(p, NV097_SET_MATERIAL_EMISSION, 0x0, 0x0, 0x0);
  p = pb_push1f(p, NV097_SET_MATERIAL_ALPHA, 1.0f);
  pb_end(p);

  SetLight();
}

void LightingTwoSidedTests::Initialize() {
  TestSuite::Initialize();

  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, true);
    p = pb_push1(p, NV097_SET_LIGHT_CONTROL, 0x1);

    p = pb_push4f(p, NV097_SET_VERTEX_DATA4F_M + (4 * NV2A_VERTEX_ATTR_DIFFUSE), 1.f, 1.f, 1.f, 1.f);
    p = pb_push4f(p, NV097_SET_VERTEX_DATA4F_M + (4 * NV2A_VERTEX_ATTR_SPECULAR), 0.f, 1.f, 0.f, 1.f);
    p = pb_push4f(p, NV097_SET_VERTEX_DATA4F_M + (4 * NV2A_VERTEX_ATTR_BACK_DIFFUSE), 0.7f, 0.f, 1.f, 1.f);
    p = pb_push4f(p, NV097_SET_VERTEX_DATA4F_M + (4 * NV2A_VERTEX_ATTR_BACK_SPECULAR), 0.5f, 0.f, 0.5f, 1.f);

    // Culling must be disabled for two-sided lighting to have any effect.
    p = pb_push1(p, NV097_SET_CULL_FACE_ENABLE, false);
    p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, true);

    // By default back alpha is 0, so it must be set in the test.
    // It is set to half transparent to demonstrate that it has no effect when two sided lighting is off.
    p = pb_push1f(p, NV097_SET_BACK_MATERIAL_ALPHA, 0.5f);

    pb_end(p);
  }

  host_.SetCombinerControl(1);
  host_.SetInputColorCombiner(0, TestHost::SRC_DIFFUSE, false, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                              false, TestHost::MAP_UNSIGNED_INVERT);
  host_.SetInputAlphaCombiner(0, TestHost::SRC_DIFFUSE, true, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                              false, TestHost::MAP_UNSIGNED_INVERT);

  host_.SetOutputColorCombiner(0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0);
  host_.SetOutputAlphaCombiner(0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0);

  host_.SetFinalCombiner0Just(TestHost::SRC_R0);
  host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_R0, true,
                          false, /*specular_add_invert_r0*/ false, /* specular_add_invert_v1*/ false,
                          /* specular_clamp */ true);

  SetLightAndMaterial();
}

void LightingTwoSidedTests::Deinitialize() {
  vertex_buffer_.reset();
  TestSuite::Deinitialize();
}

void LightingTwoSidedTests::Test() {
  static constexpr uint32_t kBackgroundColor = 0xFF424243;
  host_.PrepareDraw(kBackgroundColor);

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_LIGHT_TWO_SIDE_ENABLE, true);
    pb_end(p);
  }

  static constexpr float z = -1.f;
  // Front face
  {
    // clang-format off
    constexpr float kVertices[][2] = {
        {-1.5f, 0.1f},
        {-1.25f, 1.5f},
        {-0.35f, 1.5f},
        {-0.25f, 0.1f},
    };

    // clang-format on
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetNormal(0.f, 0.f, 1.f);
    for (auto pt : kVertices) {
      host_.SetVertex(pt[0], pt[1], z);
    }
    host_.End();
  }

  // Back face
  {
    // clang-format off
    constexpr float kVertices[][2] = {
        {0.25f, 0.1f},
        {1.25f, 0.1f},
        {1.35f, 1.5f},
        {0.1f, 1.5f},
    };

    // clang-format on
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetNormal(0.f, 0.f, -1.f);
    for (auto pt : kVertices) {
      host_.SetVertex(pt[0], pt[1], z);
    }
    host_.End();
  }

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_LIGHT_TWO_SIDE_ENABLE, false);
    pb_end(p);
  }

  // Front face
  {
    // clang-format off
    constexpr float kVertices[][2] = {
        {-1.5f, -1.5f},
        {-1.25f, -0.1f},
        {-0.35f, -0.1f},
        {-0.25f, -1.5f},
    };

    // clang-format on
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetNormal(0.f, 0.f, 1.f);
    for (auto pt : kVertices) {
      host_.SetVertex(pt[0], pt[1], z);
    }
    host_.End();
  }

  // Back face
  {
    // clang-format off
    constexpr float kVertices[][2] = {
        {0.25f, -1.5f},
        {1.25f, -1.5f},
        {1.35f, -0.1f},
        {0.1f, -0.1f},
    };

    // clang-format on
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetNormal(0.f, 0.f, -1.f);
    for (auto pt : kVertices) {
      host_.SetVertex(pt[0], pt[1], z);
    }
    host_.End();
  }

  pb_printat(1, 19, (char *)"Front ON");
  pb_printat(1, 36, (char *)"Back ON");
  pb_printat(15, 19, (char *)"Front OFF");
  pb_printat(15, 36, (char *)"Back OFF");

  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kTestName);
}
