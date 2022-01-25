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
        auto test = [this, fog_mode, gen_mode, alpha]() { Test(fog_mode, gen_mode, alpha); };
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

void FogTests::Deinitialize() {
  vertex_buffer_.reset();
  TestSuite::Deinitialize();
}

void FogTests::CreateGeometry() {
  constexpr int kNumTriangles = 4;
  vertex_buffer_ = host_.AllocateVertexBuffer(kNumTriangles * 3);

  int index = 0;
  {
    float one[] = {-1.5f, -1.5f, 0.0f};
    float two[] = {-2.5f, 0.6f, 0.0f};
    float three[] = {-0.5f, 0.6f, 0.0f};
    vertex_buffer_->DefineTriangle(index++, one, two, three);
  }

  {
    float one[] = {0.0f, -1.5f, 5.0f};
    float two[] = {-1.0f, 0.75f, 10.0f};
    float three[] = {2.0f, 0.75f, 20.0f};
    vertex_buffer_->DefineTriangle(index++, one, two, three);
  }

  {
    float one[] = {5.0f, -2.0f, 30};
    float two[] = {3.0f, 2.0f, 40};
    float three[] = {12.0f, 2.0f, 70};
    vertex_buffer_->DefineTriangle(index++, one, two, three);
  }

  {
    float one[] = {20.0f, -10.0f, 50};
    float two[] = {12.0f, 10.0f, 125};
    float three[] = {80.0f, 10.0f, 200};
    vertex_buffer_->DefineTriangle(index++, one, two, three);
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

// FogInfiniteFogCoordinateTests

// clang format off
static constexpr uint32_t kInfiniteFogCShader[] = {
#include "shaders/fog_infinite_fogc_test.inl"
};
// clang format on

FogInfiniteFogCoordinateTests::FogInfiniteFogCoordinateTests(TestHost& host, std::string output_dir)
    : FogCustomShaderTests(host, std::move(output_dir), "Fog inf coord") {}

void FogInfiniteFogCoordinateTests::Initialize() {
  FogCustomShaderTests::Initialize();

  auto shader = host_.GetShaderProgram();
  shader->SetShaderOverride(kInfiniteFogCShader, sizeof(kInfiniteFogCShader));
  // const c[12] = 0
  shader->SetUniformF(12, 0.0f);
  host_.SetShaderProgram(shader);
}

// FogVec4CoordTests

// clang format off
static constexpr uint32_t kFogVec4Unset[] = {
#include "shaders/fog_vec4_unset.inl"
};
static constexpr uint32_t kFogVec4W[] = {
#include "shaders/fog_vec4_w.inl"
};
static constexpr uint32_t kFogVec4WX[] = {
#include "shaders/fog_vec4_wx.inl"
};
static constexpr uint32_t kFogVec4WY[] = {
#include "shaders/fog_vec4_wy.inl"
};
static constexpr uint32_t kFogVec4WZYX[] = {
#include "shaders/fog_vec4_wzyx.inl"
};
static constexpr uint32_t kFogVec4X[] = {
#include "shaders/fog_vec4_x.inl"
};
static constexpr uint32_t kFogVec4XYZW[] = {
#include "shaders/fog_vec4_xyzw.inl"
};
static constexpr uint32_t kFogVec4Y[] = {
#include "shaders/fog_vec4_y.inl"
};
static constexpr uint32_t kFogVec4Z[] = {
#include "shaders/fog_vec4_z.inl"
};
// clang format on

#define DEF_SHADER(shader) (shader), sizeof(shader)

// clang format off
static const FogVec4CoordTests::TestConfig kFogWTests[] = {
    {"None", DEF_SHADER(kFogVec4Unset), {0.0f, 0.0f, 0.0f, 0.0f}},

    {"W", DEF_SHADER(kFogVec4W), {0.0f, 0.0f, 0.0f, 0.0f}},
    {"W", DEF_SHADER(kFogVec4W), {0.0f, 0.0f, 0.0f, 1.0f}},

    {"WX", DEF_SHADER(kFogVec4WX), {0.25f, 0.0f, 0.0f, 0.5f}},
    {"WX", DEF_SHADER(kFogVec4WX), {0.65f, 0.0f, 0.0f, 0.0f}},

    {"WY", DEF_SHADER(kFogVec4WY), {0.0f, 0.0f, 0.25f, 0.75f}},
    {"WY", DEF_SHADER(kFogVec4WY), {0.0f, 0.0f, 0.75f, 0.25f}},

    {"WZYX", DEF_SHADER(kFogVec4WZYX), {0.25f, 0.5f, 0.75f, 1.0f}},
    {"WZYX", DEF_SHADER(kFogVec4WZYX), {1.0f, 0.75f, 0.5f, 0.25f}},

    {"X", DEF_SHADER(kFogVec4X), {0.0f, 0.0f, 0.0f, 0.0f}},
    {"X", DEF_SHADER(kFogVec4X), {0.9f, 0.0f, 0.0f, 0.0f}},

    {"XYZW", DEF_SHADER(kFogVec4XYZW), {1.0f, 0.25f, 0.75f, 0.5f}},
    {"XYZW", DEF_SHADER(kFogVec4XYZW), {0.0f, 0.33f, 0.66f, 0.9f}},

    {"Y", DEF_SHADER(kFogVec4Y), {0.0f, 4.0f, 0.0f, 0.0f}},
    {"Y", DEF_SHADER(kFogVec4Y), {0.0f, 6.0f, 0.0f, 0.0f}},

    {"Z", DEF_SHADER(kFogVec4Z), {0.0f, 0.0f, 0.2f, 0.0f}},
    {"Z", DEF_SHADER(kFogVec4Z), {0.0f, 0.0f, 0.8f, 0.0f}},
};
// clang format on

#undef DEF_TEST

FogVec4CoordTests::FogVec4CoordTests(TestHost& host, std::string output_dir)
    : FogCustomShaderTests(host, std::move(output_dir), "Fog coord vec4") {
  tests_.clear();

  for (auto& config : kFogWTests) {
    std::string name = MakeTestName(config);
    tests_[name] = [this, &config]() { Test(config); };
  }
}

void FogVec4CoordTests::Initialize() {
  FogCustomShaderTests::Initialize();

  // Set specular alpha on vertices.
  uint32_t num_vertices = vertex_buffer_->GetNumVertices();
  float inc = 1.0f / (float)num_vertices;

  auto vertex = vertex_buffer_->Lock();
  float alpha = 0.0f;
  for (auto i = 0; i < num_vertices; ++i, ++vertex) {
    vertex->SetDiffuse(0.0f, 0.0, 1.0f, 1.0f);
    alpha += inc;
  }
  vertex_buffer_->Unlock();

  host_.ClearInputColorCombiners();
  host_.ClearInputAlphaCombiners();
  host_.ClearOutputColorCombiners();
  host_.ClearOutputAlphaCombiners();
}

void FogVec4CoordTests::Test(const TestConfig& config) {
  auto shader = host_.GetShaderProgram();
  shader->SetShaderOverride(config.shader, config.shader_size);
  shader->SetUniformF(12, config.fog[0], config.fog[1], config.fog[2], config.fog[3]);
  host_.SetShaderProgram(shader);

  static constexpr uint32_t kBackgroundColor = 0xFF303030;

  host_.PrepareDraw(kBackgroundColor);

  host_.SetInputColorCombiner(0, TestHost::SRC_DIFFUSE, false, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                              false, TestHost::MAP_UNSIGNED_INVERT);
  host_.SetInputAlphaCombiner(0, TestHost::SRC_DIFFUSE, true, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                              false, TestHost::MAP_UNSIGNED_INVERT);

  host_.SetOutputColorCombiner(0, TestHost::DST_R0);
  host_.SetOutputAlphaCombiner(0, TestHost::DST_R0);

  host_.SetFinalCombiner0(TestHost::SRC_FOG, true, false, TestHost::SRC_FOG, false, false, TestHost::SRC_R0);
  host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_R0, true);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_CONTROL, 1);

  p = pb_push1(p, NV097_SET_FOG_ENABLE, true);

  // Note: Fog color is ABGR and not ARGB
  p = pb_push1(p, NV097_SET_FOG_COLOR, 0x00FF00);

  // Gen mode does not seem to matter when using a vertex shader.
  p = pb_push1(p, NV097_SET_FOG_GEN_MODE, FOG_GEN_SPEC_ALPHA);
  p = pb_push1(p, NV097_SET_FOG_MODE, FOG_LINEAR);

  // The final fog calculation should be exactly the output of the shader.
  p = pb_push3f(p, NV097_SET_FOG_PARAMS, 1.0f, 1.0f, 0.0f);

  pb_end(p);

  host_.DrawArrays(host_.POSITION | host_.DIFFUSE);

  std::string name = MakeTestName(config);
  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, name);
}

std::string FogVec4CoordTests::MakeTestName(const TestConfig& config) {
  char buf[40] = {0};
  snprintf(buf, 39, "%s-%.2f_%.2f_%.2f_%.2f", config.prefix, config.fog[0], config.fog[1], config.fog[2],
           config.fog[3]);
  return buf;
}
