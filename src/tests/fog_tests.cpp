#include "fog_tests.h"

#include <pbkit/pbkit.h>

#include <utility>

#include "pbkit_ext.h"
#include "shaders/perspective_vertex_shader.h"
#include "test_host.h"
#include "vertex_buffer.h"

static constexpr float kFogStart = 1.0f;
static constexpr float kFogEnd = 200.0f;

// clang-format off
static constexpr FogTests::FogMode kFogModes[] = {
    FogTests::FOG_LINEAR,
    FogTests::FOG_EXP,
    FogTests::FOG_EXP2,
    FogTests::FOG_EXP_ABS,
    FogTests::FOG_EXP2_ABS,
    FogTests::FOG_LINEAR_ABS,
};

static constexpr FogTests::FogGenMode kGenModes[] = {
//    FogTests::FOG_GEN_SPEC_ALPHA,
//    FogTests::FOG_GEN_RADIAL,
    FogTests::FOG_GEN_PLANAR,
//    FogTests::FOG_GEN_ABS_PLANAR,
//    FogTests::FOG_GEN_FOG_X,
};
// clang-format on

FogTests::FogTests(TestHost& host, std::string output_dir, std::string suite_name)
    : TestSuite(host, std::move(output_dir), std::move(suite_name)) {
  for (const auto fog_mode : kFogModes) {
    for (const auto gen_mode : kGenModes) {
      // Alpha doesn't seem to actually have any effect.
      for (auto alpha : {0xFF}) {
        const std::string test_name = MakeTestName(fog_mode, gen_mode, alpha);
        auto test = [this, fog_mode, gen_mode, alpha]() { this->Test(fog_mode, gen_mode, alpha); };
        tests_[test_name] = test;
      }
    }
  }
}

void FogTests::Initialize() {
  TestSuite::Initialize();

  host_.SetShaderProgram(nullptr);
  CreateGeometry();
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
}

void FogTests::CreateGeometry() {
  constexpr int kNumTriangles = 4;
  auto buffer = host_.AllocateVertexBuffer(kNumTriangles * 3);

  int index = 0;
  {
    float one[] = {-1.5f, -1.5f, 0.0f};
    float two[] = {-2.5f, 0.6f, 0.0f};
    float three[] = {-0.5f, 0.6f, 0.0f};
    buffer->DefineTriangle(index++, one, two, three);
  }

  {
    float one[] = {0.0f, -1.5f, 5.0f};
    float two[] = {-1.0f, 0.75f, 10.0f};
    float three[] = {2.0f, 0.75f, 20.0f};
    buffer->DefineTriangle(index++, one, two, three);
  }

  {
    float one[] = {5.0f, -2.0f, 30};
    float two[] = {3.0f, 2.0f, 40};
    float three[] = {12.0f, 2.0f, 70};
    buffer->DefineTriangle(index++, one, two, three);
  }

  {
    float one[] = {20.0f, -10.0f, 50};
    float two[] = {12.0f, 10.0f, 125};
    float three[] = {80.0f, 10.0f, 200};
    buffer->DefineTriangle(index++, one, two, three);
  }
}

void FogTests::Test(FogTests::FogMode fog_mode, FogTests::FogGenMode gen_mode, uint32_t fog_alpha) {
  // See https://docs.microsoft.com/en-us/previous-versions/windows/desktop/bb324452(v=vs.85)
  // https://docs.microsoft.com/en-us/previous-versions/windows/desktop/bb322857(v=vs.85)
  static constexpr uint32_t kBackgroundColor = 0xFF303030;
  host_.PrepareDraw(kBackgroundColor);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_CONTROL, 1);

  pb_push_to(SUBCH_3D, p++, NV097_SET_COMBINER_COLOR_ICW, 8);
  *(p++) = 0x4200000;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;

  pb_push_to(SUBCH_3D, p++, NV097_SET_COMBINER_COLOR_OCW, 8);
  *(p++) = 0xC00;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;

  pb_push_to(SUBCH_3D, p++, NV097_SET_COMBINER_ALPHA_ICW, 8);
  *(p++) = 0x14200000;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;

  pb_push_to(SUBCH_3D, p++, NV097_SET_COMBINER_ALPHA_OCW, 8);
  *(p++) = 0xC00;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;

  p = pb_push1(p, NV097_SET_FOG_ENABLE, true);
  p = pb_push1(p, NV097_SET_COMBINER_SPECULAR_FOG_CW0, 0x130C0300);
  p = pb_push1(p, NV097_SET_COMBINER_SPECULAR_FOG_CW1, 0x1c80);

  // Note: Fog color is ABGR and not ARGB
  p = pb_push1(p, NV097_SET_FOG_COLOR, 0x7F2030 + (fog_alpha << 24));

  p = pb_push1(p, NV097_SET_FOG_GEN_MODE, gen_mode);
  p = pb_push1(p, NV097_SET_FOG_MODE, fog_mode);

  // Linear parameters.
  // TODO: Parameterize.
  // Right now these are just the near and far planes.
  // Exponential parameters.
  const float fog_density = 0.025f;

  static constexpr float LN_256 = 5.5452f;
  static constexpr float SQRT_LN_256 = 2.3548f;

  float bias_param = 0.0f;
  float multiplier_param = 1.0f;

  switch (fog_mode) {
    case FOG_LINEAR:
    case FOG_LINEAR_ABS:
      multiplier_param = -1.0f / (kFogEnd - kFogStart);
      bias_param = 1.0f + -kFogEnd * multiplier_param;
      break;

    case FOG_EXP:
    case FOG_EXP_ABS:
      bias_param = 1.5f;
      multiplier_param = -fog_density / (2.0f * LN_256);
      break;

    case FOG_EXP2:
    case FOG_EXP2_ABS:
      bias_param = 1.5f;
      multiplier_param = -fog_density / (2.0f * SQRT_LN_256);
      break;

    default:
      break;
  }

  // TODO: Figure out what the third parameter is. In all examples I've seen it's always been 0.
  p = pb_push3f(p, NV097_SET_FOG_PARAMS, bias_param, multiplier_param, 0.0f);

  pb_end(p);

  host_.DrawArrays(host_.POSITION | host_.DIFFUSE);

  std::string name = MakeTestName(fog_mode, gen_mode, fog_alpha);
  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, name);
}

std::string FogTests::MakeTestName(FogTests::FogMode fog_mode, FogTests::FogGenMode gen_mode, uint32_t fog_alpha) {
  std::string ret;

  {
    char buf[8] = {0};
    snprintf(buf, 7, "A%02X", fog_alpha);
    ret = buf;
  }

  switch (fog_mode) {
    case FOG_LINEAR:
      ret += "-linear";
      break;
    case FOG_EXP:
      ret += "-exp";
      break;
    case FOG_EXP2:
      ret += "-exp2";
      break;
    case FOG_EXP_ABS:
      ret += "-exp_abs";
      break;
    case FOG_EXP2_ABS:
      ret += "-exp2_abs";
      break;
    case FOG_LINEAR_ABS:
      ret += "-linear_abs";
      break;
  }

  switch (gen_mode) {
    case FOG_GEN_SPEC_ALPHA:
      ret += "-spec_alpha";
      break;
    case FOG_GEN_RADIAL:
      ret += "-radial";
      break;
    case FOG_GEN_PLANAR:
      ret += "-planar";
      break;
    case FOG_GEN_ABS_PLANAR:
      ret += "-abs_planar";
      break;
    case FOG_GEN_FOG_X:
      ret += "-fog_x";
      break;
  }

  return std::move(ret);
}

FogCustomShaderTests::FogCustomShaderTests(TestHost& host, std::string output_dir, std::string suite_name)
    : FogTests(host, std::move(output_dir), std::move(suite_name)) {}

void FogCustomShaderTests::Initialize() {
  FogTests::Initialize();

  auto shader = std::make_shared<PerspectiveVertexShader>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  shader->SetNear(kFogStart);
  shader->SetFar(kFogEnd);
  VECTOR camera_position{0.0f, 0.0f, -7.0f, 1.0f};
  VECTOR look_at{0.0f, 0.0f, 0.0f, 1.0f};

  shader->LookAt(camera_position, look_at);

  shader->SetLightingEnabled(false);
  shader->SetTextureEnabled(false);
  host_.SetShaderProgram(shader);
}

FogInfiniteFogCoordinateTests::FogInfiniteFogCoordinateTests(TestHost& host, std::string output_dir)
    : FogCustomShaderTests(host, std::move(output_dir), "Fog inf coord") {}

// clang format off
static constexpr uint32_t kInfiniteFogCShader[] = {
#include "shaders/fog_infinite_fogc_test.inl"
};
// clang format on

void FogInfiniteFogCoordinateTests::Initialize() {
  FogCustomShaderTests::Initialize();

  auto shader = host_.GetShaderProgram();
  shader->SetShaderOverride(kInfiniteFogCShader, sizeof(kInfiniteFogCShader));
  host_.SetShaderProgram(shader);
}
