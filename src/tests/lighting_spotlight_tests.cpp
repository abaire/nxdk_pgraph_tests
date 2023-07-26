#include "lighting_spotlight_tests.h"

#include <pbkit/pbkit.h>

#include "debug_output.h"
#include "models/flat_grid.h"
#include "pbkit_ext.h"
#include "test_host.h"
#include "vertex_buffer.h"
#include "xbox_math_matrix.h"

static constexpr float PI_OVER_180 = (float)M_PI / 180.0f;
#define DEG2RAD(c) ((float)(c)*PI_OVER_180)

static constexpr char kFalloffName[] = "Fo";
static constexpr char kFalloffFixedName[] = "FoFixed";
static constexpr char kPhiThetaName[] = "PT";
static constexpr char kPhiThetaFixedName[] = "PTFixed";

static constexpr float kZPos = -5.f;
#define FALLOFF_ENTRY(a, b, c) \
  { {0.f, 0.f, kZPos, 1.f}, {0.f, 0.f, 1.f, 1.f}, DEG2RAD(30.f), DEG2RAD(15.f), {(a), (b), (c)}, }

static const LightingSpotlightTests::Spotlight kFalloffTests[] = {
    FALLOFF_ENTRY(0.f, -0.494592f, 1.494592f),         // falloff = 1.f
    FALLOFF_ENTRY(-0.f, 1.f, 0.f),                     // falloff = 0.f
    FALLOFF_ENTRY(-0.000244f, 0.500122f, 0.499634f),   // falloff=0.5
    FALLOFF_ENTRY(-0.170208f, -0.855843f, 1.685635f),  // falloff=2.0
    FALLOFF_ENTRY(-0.706496f, -2.507095f, 2.800600f),  // falloff=10.0
    FALLOFF_ENTRY(-0.932112f, -3.097628f, 3.165516f),  // falloff=50.0
    FALLOFF_ENTRY(-0.986137f, -3.165117f, 3.178980f),  // falloff=250.0
    FALLOFF_ENTRY(-0.993286f, -2.953324f, 2.960038f),  // falloff=500.0
    FALLOFF_ENTRY(-0.996561f, -3.084043f, 3.087482f),  // falloff=1000.0

    //    FALLOFF_ENTRY(0.f, 0.f, 0.f),
    //    FALLOFF_ENTRY(1.f, 0.f, 0.f),
    //    FALLOFF_ENTRY(-1.f, 0.f, 0.f),
    //    FALLOFF_ENTRY(0.f, 1.f, 0.f),
    //    FALLOFF_ENTRY(0.f, -1.f, 0.f),
    //    FALLOFF_ENTRY(0.f, -4.f, 0.f),
    //    FALLOFF_ENTRY(0.f, 0.f, 4.f),
    //    FALLOFF_ENTRY(0.f, 0.f, -1.f),
};

#define PHI_THETA(phi, theta) \
  { {0.f, 0.f, kZPos, 1.f}, {0.f, 0.f, 1.f, 1.f}, DEG2RAD(phi), DEG2RAD(theta), {0.f, -0.494592f, 1.494592f}, }
static const LightingSpotlightTests::Spotlight kPhiThetaTests[] = {
    PHI_THETA(DEG2RAD(15.f), DEG2RAD(30.f)),  // Inverted phi and theta
    PHI_THETA(DEG2RAD(15.f), DEG2RAD(15.f)),  // phi == theta
    PHI_THETA(DEG2RAD(0.f), DEG2RAD(0.f)),
    PHI_THETA(DEG2RAD(-15.f), DEG2RAD(-15.f)),
};

static std::string MakeFalloffTestName(const char* prefix, const LightingSpotlightTests::Spotlight& light) {
  char buf[64];
  snprintf(buf, sizeof(buf), "%s_%f_%f_%f", prefix, light.falloff[0], light.falloff[1], light.falloff[2]);
  return buf;
}

static std::string MakePhiThetaTestName(const char* prefix, const LightingSpotlightTests::Spotlight& light) {
  char buf[64];
  snprintf(buf, sizeof(buf), "%s_%f_%f", prefix, light.phi, light.theta);
  return buf;
}

LightingSpotlightTests::LightingSpotlightTests(TestHost& host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Lighting Spotlight") {
  for (auto& light : kFalloffTests) {
    //    {
    //      auto name = MakeFalloffTestName(kFalloffName, light);
    //      tests_[name] = [this, name, light]() { this->Test(name, light); };
    //    }
    {
      auto name = MakeFalloffTestName(kFalloffFixedName, light);
      tests_[name] = [this, name, light]() { this->TestFixed(name, light); };
    }
  }

  for (auto& light : kPhiThetaTests) {
    //    {
    //      auto name = MakePhiThetaTestName(kPhiThetaName, light);
    //      tests_[name] = [this, name, light]() { this->Test(name, light); };
    //    }
    {
      auto name = MakePhiThetaTestName(kPhiThetaFixedName, light);
      tests_[name] = [this, name, light]() { this->TestFixed(name, light); };
    }
  }
}

void LightingSpotlightTests::Initialize() {
  TestSuite::Initialize();

  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, true);
    p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, false);
    p = pb_push1(p, NV097_SET_LIGHT_CONTROL, 0x1);
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

  CreateGeometry();

  auto p = pb_begin();

  p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_ALL_FROM_MATERIAL);
  p = pb_push3f(p, NV097_SET_SCENE_AMBIENT_COLOR, 0.031373, 0.031373, 0.031373);
  p = pb_push3(p, NV097_SET_MATERIAL_EMISSION, 0x0, 0x0, 0x0);
  p = pb_push1f(p, NV097_SET_MATERIAL_ALPHA, 1.0f);
  pb_end(p);
}

void LightingSpotlightTests::Deinitialize() {
  vertex_buffer_.reset();
  TestSuite::Deinitialize();
}

void LightingSpotlightTests::CreateGeometry() {
  vector_t diffuse{0.27f, 0.f, 0.27f, 1.f};
  vector_t specular{0.f, 0.f, 0.f, 1.f};
  auto model = FlatGrid(diffuse, specular);
  vertex_buffer_ = host_.AllocateVertexBuffer(model.GetVertexCount());

  matrix4_t translation_matrix;
  MatrixSetIdentity(translation_matrix);
  vector_t translate = {0.f, 0.f, 0.25f, 0.f};
  MatrixTranslate(translation_matrix, translate);
  model.PopulateVertexBuffer(vertex_buffer_, (const float*)translation_matrix[0]);
}

void LightingSpotlightTests::Spotlight::Commit(uint32_t light_index, const XboxMath::matrix4_t& view_matrix) const {
  vector_t transformed_position;
  VectorMultMatrix(position, view_matrix, transformed_position);
  transformed_position[3] = 1.f;

  vector_t normalized_direction{0.f, 0.f, 0.f, 1.f};
  VectorNormalize(direction, normalized_direction);

  // See spotlight handling code here:
  // https://github.com/xemu-project/xemu/blob/15bf68594f186c3df876fa7856b0d7a13e80f0e1/hw/xbox/nv2a/shaders.c#L653
  // and directx description here:
  // https://learn.microsoft.com/en-us/windows/uwp/graphics-concepts/attenuation-and-spotlight-factor
  float cos_half_theta = cosf(0.5f * theta);
  float cos_half_phi = cosf(0.5f * phi);

  float inv_scale = -1.f / (cos_half_theta - cos_half_phi);
  ScalarMultVector(normalized_direction, inv_scale);
  normalized_direction[3] = cos_half_phi * inv_scale;

  auto p = pb_begin();

  uint32_t light_offset = light_index * 128;
  p = pb_push3(p, NV097_SET_LIGHT_AMBIENT_COLOR + light_offset, 0, 0, 0);
  p = pb_push3f(p, NV097_SET_LIGHT_DIFFUSE_COLOR + light_offset, 1.f, 0.f, 0.25f);
  p = pb_push3f(p, NV097_SET_LIGHT_SPECULAR_COLOR + light_offset, 0.f, 1.f, 0.f);
  p = pb_push1f(p, NV097_SET_LIGHT_LOCAL_RANGE + light_offset, 10.0f);
  p = pb_push3fv(p, NV097_SET_LIGHT_LOCAL_POSITION + light_offset, transformed_position);
  p = pb_push3f(p, NV097_SET_LIGHT_LOCAL_ATTENUATION + light_offset, 0.0f, 0.0f, 0.0f);
  p = pb_push3fv(p, NV097_SET_LIGHT_SPOT_FALLOFF + light_offset, falloff);
  p = pb_push4fv(p, NV097_SET_LIGHT_SPOT_DIRECTION + light_offset, normalized_direction);
  p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK, NV097_SET_LIGHT_ENABLE_MASK_LIGHT0_SPOT << (2 * light_index));

  pb_end(p);
}

void LightingSpotlightTests::Test(const std::string& name, const Spotlight& light) {}

void LightingSpotlightTests::TestFixed(const std::string& name, const Spotlight& light) {
  static constexpr uint32_t kBackgroundColor = 0xFF232623;
  host_.PrepareDraw(kBackgroundColor);

  light.Commit(0, host_.GetFixedFunctionModelViewMatrix());

  host_.SetVertexBuffer(vertex_buffer_);
  host_.DrawArrays(host_.POSITION | host_.NORMAL | host_.DIFFUSE | host_.SPECULAR);

  pb_print("%s", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, name);
}
