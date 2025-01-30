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
static const FogTests::FogMode kFogModes[] = {
    FogTests::FOG_LINEAR,
    FogTests::FOG_EXP,
    FogTests::FOG_EXP2,
    FogTests::FOG_EXP_ABS,
    FogTests::FOG_EXP2_ABS,
    FogTests::FOG_LINEAR_ABS,
};

static const FogTests::FogGenMode kGenModes[] = {
//    FogTests::FOG_GEN_SPEC_ALPHA,
//    FogTests::FOG_GEN_RADIAL,
    FogTests::FOG_GEN_PLANAR,
//    FogTests::FOG_GEN_ABS_PLANAR,
//    FogTests::FOG_GEN_FOG_X,
};
// clang-format on

FogTests::FogTests(TestHost& host, std::string output_dir, const Config& config, std::string suite_name)
    : TestSuite(host, std::move(output_dir), std::move(suite_name), config) {
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

  host_.SetVertexShaderProgram(nullptr);
  CreateGeometry();
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  host_.SetFinalCombiner0(TestHost::SRC_FOG, true, false, TestHost::SRC_DIFFUSE, false, false, TestHost::SRC_FOG, false,
                          false);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_FOG_ENABLE, true);
  pb_end(p);
}

void FogTests::Deinitialize() {
  vertex_buffer_.reset();
  TestSuite::Deinitialize();
}

void FogTests::CreateGeometry() {
  constexpr int kNumTriangles = 4;
  vertex_buffer_ = host_.AllocateVertexBuffer(kNumTriangles * 3);

  Color diffuse = {1.0, 1.0, 1.0, 1.0};
  int index = 0;
  {
    float one[] = {-1.5f, -1.5f, 0.0f};
    float two[] = {-2.5f, 0.6f, 0.0f};
    float three[] = {-0.5f, 0.6f, 0.0f};
    vertex_buffer_->DefineTriangle(index++, one, two, three, diffuse, diffuse, diffuse);
  }

  {
    float one[] = {0.0f, -1.5f, 5.0f};
    float two[] = {-1.0f, 0.75f, 10.0f};
    float three[] = {2.0f, 0.75f, 20.0f};
    vertex_buffer_->DefineTriangle(index++, one, two, three, diffuse, diffuse, diffuse);
  }

  {
    float one[] = {5.0f, -2.0f, 30};
    float two[] = {3.0f, 2.0f, 40};
    float three[] = {12.0f, 2.0f, 70};
    vertex_buffer_->DefineTriangle(index++, one, two, three, diffuse, diffuse, diffuse);
  }

  {
    float one[] = {20.0f, -10.0f, 50};
    float two[] = {12.0f, 10.0f, 125};
    float three[] = {80.0f, 10.0f, 200};
    vertex_buffer_->DefineTriangle(index++, one, two, three, diffuse, diffuse, diffuse);
  }
}

void FogTests::Test(FogTests::FogMode fog_mode, FogTests::FogGenMode gen_mode, uint32_t fog_alpha) {
  // See https://docs.microsoft.com/en-us/previous-versions/windows/desktop/bb324452(v=vs.85)
  // https://docs.microsoft.com/en-us/previous-versions/windows/desktop/bb322857(v=vs.85)
  static constexpr uint32_t kBackgroundColor = 0xFF303030;
  host_.PrepareDraw(kBackgroundColor);

  auto p = pb_begin();
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

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
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

FogCustomShaderTests::FogCustomShaderTests(TestHost& host, std::string output_dir, const Config& config,
                                           std::string suite_name)
    : FogTests(host, std::move(output_dir), config, std::move(suite_name)) {}

void FogCustomShaderTests::Initialize() {
  FogTests::Initialize();

  auto shader = std::make_shared<PerspectiveVertexShader>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  shader->SetNear(kFogStart);
  shader->SetFar(kFogEnd);
  vector_t camera_position{0.0f, 0.0f, -7.0f, 1.0f};
  vector_t look_at{0.0f, 0.0f, 0.0f, 1.0f};

  shader->LookAt(camera_position, look_at);

  shader->SetLightingEnabled(false);
  host_.SetVertexShaderProgram(shader);
}

// FogInfiniteFogCoordinateTests

// clang-format off
static const uint32_t kInfiniteFogCShader[] = {
#include "fog_infinite_fogc_test.vshinc"
};
// clang-format on

FogInfiniteFogCoordinateTests::FogInfiniteFogCoordinateTests(TestHost& host, std::string output_dir,
                                                             const Config& config)
    : FogCustomShaderTests(host, std::move(output_dir), config, "Fog inf coord") {}

void FogInfiniteFogCoordinateTests::Initialize() {
  FogCustomShaderTests::Initialize();

  auto shader = host_.GetShaderProgram();
  shader->SetShaderOverride(kInfiniteFogCShader, sizeof(kInfiniteFogCShader));
  // const c[12] = 0
  shader->SetUniformF(12, 0.0f);
  host_.SetVertexShaderProgram(shader);
}

// FogVec4CoordTests

// clang-format off
static const uint32_t kFogVec4Unset[] = {
#include "fog_vec4_unset.vshinc"
};

// Individual setters.
static const uint32_t kFogVec4X[] = {
#include "fog_vec4_x.vshinc"
};
static const uint32_t kFogVec4Y[] = {
#include "fog_vec4_y.vshinc"
};
static const uint32_t kFogVec4Z[] = {
#include "fog_vec4_z.vshinc"
};
static const uint32_t kFogVec4W[] = {
#include "fog_vec4_w.vshinc"
};
static const uint32_t kFogVec4W_X[] = {
#include "fog_vec4_w_x.vshinc"
};
static const uint32_t kFogVec4W_Y[] = {
#include "fog_vec4_w_y.vshinc"
};
static const uint32_t kFogVec4W_Z_Y_X[] = {
#include "fog_vec4_w_z_y_x.vshinc"
};
static const uint32_t kFogVec4X_Y_Z_W[] = {
#include "fog_vec4_x_y_z_w.vshinc"
};

// Bulk setters.
static const uint32_t kFogVec4XW[] = {
#include "fog_vec4_xw.vshinc"
};
static const uint32_t kFogVec4XY[] = {
#include "fog_vec4_xy.vshinc"
};
static const uint32_t kFogVec4XYZ[] = {
#include "fog_vec4_xyz.vshinc"
};
static const uint32_t kFogVec4XYZW[] = {
#include "fog_vec4_xyzw.vshinc"
};
static const uint32_t kFogVec4XZ[] = {
#include "fog_vec4_xz.vshinc"
};
static const uint32_t kFogVec4XZW[] = {
#include "fog_vec4_xzw.vshinc"
};
static const uint32_t kFogVec4YW[] = {
#include "fog_vec4_yw.vshinc"
};
static const uint32_t kFogVec4YZ[] = {
#include "fog_vec4_yz.vshinc"
};
static const uint32_t kFogVec4YZW[] = {
#include "fog_vec4_yzw.vshinc"
};
static const uint32_t kFogVec4ZW[] = {
#include "fog_vec4_zw.vshinc"
};
// clang-format on

#define DEF_SHADER(shader) (shader), sizeof(shader)

// clang-format off
static const FogVec4CoordTests::TestConfig kFogWTests[] = {
    {"W", DEF_SHADER(kFogVec4W), {0.0f, 0.25f, 0.0f, 0.0f}},
    {"W", DEF_SHADER(kFogVec4W), {0.5f, 0.5f, 0.0f, 0.0f}},
    {"W", DEF_SHADER(kFogVec4W), {1.0f, 0.0f, 0.0f, 0.5f}},
    {"W", DEF_SHADER(kFogVec4W), {0.3f, 0.3f, 0.3f, 1.0f}},

    {"W_X", DEF_SHADER(kFogVec4W_X), {0.25f, 0.0f, 0.0f, 0.5f}},
    {"W_X", DEF_SHADER(kFogVec4W_X), {0.65f, 0.0f, 0.0f, 0.0f}},

    {"W_Y", DEF_SHADER(kFogVec4W_Y), {1.0f, 0.0f, 1.00f, 0.75f}},
    {"W_Y", DEF_SHADER(kFogVec4W_Y), {0.0f, 0.75f, 0.75f, 0.25f}},

    {"W_Z_Y_X", DEF_SHADER(kFogVec4W_Z_Y_X), {0.25f, 0.5f, 0.75f, 1.0f}},
    {"W_Z_Y_X", DEF_SHADER(kFogVec4W_Z_Y_X), {1.0f, 0.75f, 0.5f, 0.25f}},

    {"X", DEF_SHADER(kFogVec4X), {0.0f, 0.0f, 0.0f, 0.0f}},
    {"X", DEF_SHADER(kFogVec4X), {0.9f, 0.0f, 0.0f, 0.0f}},

    {"X_Y_Z_W", DEF_SHADER(kFogVec4X_Y_Z_W), {1.0f, 0.25f, 0.75f, 0.5f}},
    {"X_Y_Z_W", DEF_SHADER(kFogVec4X_Y_Z_W), {0.0f, 0.33f, 0.66f, 0.9f}},

    {"Y", DEF_SHADER(kFogVec4Y), {0.0f, 0.0f, 0.0f, 0.0f}},
    {"Y", DEF_SHADER(kFogVec4Y), {0.0f, 0.1f, 0.0f, 0.0f}},
    {"Y", DEF_SHADER(kFogVec4Y), {0.0f, 0.6f, 0.0f, 0.0f}},

    {"Z", DEF_SHADER(kFogVec4Z), {0.0f, 0.0f, 0.0f, 0.0f}},
    {"Z", DEF_SHADER(kFogVec4Z), {0.0f, 0.0f, 0.2f, 0.0f}},
    {"Z", DEF_SHADER(kFogVec4Z), {0.0f, 0.0f, 0.8f, 0.0f}},

    {"XW", DEF_SHADER(kFogVec4XW), {0.25f, 0.0f, 0.0f, 0.5f}},
    {"XW", DEF_SHADER(kFogVec4XW), {0.65f, 0.0f, 0.0f, 0.0f}},
    {"XY", DEF_SHADER(kFogVec4XY), {0.25f, 0.5f, 0.0f, 0.0f}},
    {"XY", DEF_SHADER(kFogVec4XY), {0.65f, 0.75f, 0.0f, 0.0f}},
    {"XYZ", DEF_SHADER(kFogVec4XYZ), {0.25f, 0.5f, 0.75f, 0.0f}},
    {"XYZ", DEF_SHADER(kFogVec4XYZ), {0.65f, 0.75f, 0.33f, 0.0f}},
    {"XYZW", DEF_SHADER(kFogVec4XYZW), {0.25f, 0.50f, 0.75f, 1.0f}},
    {"XYZW", DEF_SHADER(kFogVec4XYZW), {0.65f, 0.75f, 0.33f, 0.0f}},

    {"XZ", DEF_SHADER(kFogVec4XZ), {0.25f, 0.0f, 0.75f, 0.0f}},
    {"XZ", DEF_SHADER(kFogVec4XZ), {0.65f, 0.0f, 0.33f, 0.0f}},

    {"XZW", DEF_SHADER(kFogVec4XZW), {0.25f, 0.0f, 0.75f, 1.0f}},
    {"XZW", DEF_SHADER(kFogVec4XZW), {0.65f, 0.0f, 0.33f, 0.0f}},

    {"YW", DEF_SHADER(kFogVec4YW), {0.0f, 0.50f, 0.0f, 1.0f}},
    {"YW", DEF_SHADER(kFogVec4YW), {0.0f, 0.75f, 0.0f, 0.0f}},

    {"YZ", DEF_SHADER(kFogVec4YZ), {0.00f, 0.50f, 0.75f, 0.0f}},
    {"YZ", DEF_SHADER(kFogVec4YZ), {0.00f, 0.75f, 0.33f, 0.0f}},

    {"YZW", DEF_SHADER(kFogVec4YZW), {0.0f, 0.50f, 0.75f, 1.0f}},
    {"YZW", DEF_SHADER(kFogVec4YZW), {0.0f, 0.75f, 0.33f, 0.0f}},

    {"ZW", DEF_SHADER(kFogVec4ZW), {0.0f, 0.0f, 0.75f, 1.0f}},
    {"ZW", DEF_SHADER(kFogVec4ZW), {0.0f, 0.0f, 0.33f, 0.0f}},
};
// clang-format on

#undef DEF_TEST

static constexpr const char kUnsetTest[] = "CoordNotSet";

FogVec4CoordTests::FogVec4CoordTests(TestHost& host, std::string output_dir, const Config& config)
    : FogCustomShaderTests(host, std::move(output_dir), config, "Fog coord vec4") {
  tests_.clear();

  for (auto& config : kFogWTests) {
    std::string name = MakeTestName(config);
    tests_[name] = [this, &config]() { Test(config); };
  }

  tests_[kUnsetTest] = [this]() { TestUnset(); };
}

void FogVec4CoordTests::Initialize() {
  FogCustomShaderTests::Initialize();

  // Force full opacity. This shouldn't matter in practice since the fog color's alpha is set based on the calculated
  // fog factor.
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  auto p = pb_begin();
  // Note: Fog color is ABGR and not ARGB
  p = pb_push1(p, NV097_SET_FOG_COLOR, 0x00FF00);
  // Gen mode does not seem to matter when using a vertex shader.
  p = pb_push1(p, NV097_SET_FOG_GEN_MODE, FOG_GEN_SPEC_ALPHA);
  p = pb_push1(p, NV097_SET_FOG_MODE, FOG_LINEAR);

  // The final fog calculation should be exactly the output of the shader.
  p = pb_push3f(p, NV097_SET_FOG_PARAMS, 1.0f, 1.0f, 0.0f);
  pb_end(p);
}

void FogVec4CoordTests::Test(const TestConfig& config) {
  SetShader(config);
  host_.SetFinalCombiner0(TestHost::SRC_FOG, true, false, TestHost::SRC_FOG, false, false, TestHost::SRC_DIFFUSE);

  host_.PrepareDraw(0xFF303030);
  host_.DrawArrays(host_.POSITION | host_.DIFFUSE);

  std::string name = MakeTestName(config);
  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

void FogVec4CoordTests::TestUnset() {
  auto shader = host_.GetShaderProgram();

  // Draw with the fog coordinates explicitly set to put the hardware into a consistent state.
  // This is done repeatedly to fix an apparent issue with parallelism, drawing only once can lead to a situation where
  // one or more of the vertices in the "unset" draw case below still have arbitrary values from previous operations
  // and are non-hermetic, leading to test results that are dependent on previous draws.
  for (int i = 0; i < 2; ++i) {
    // It is expected that only the X value will have an effect.
    SetShader({"XYZW", DEF_SHADER(kFogVec4XYZW), {0.25f, 0.95f, 0.5f, 0.75f}});

    host_.PrepareDraw(0xFFFF00FF);
    host_.DrawArrays(host_.POSITION | host_.DIFFUSE);
  }

  // Draw a second time, leaving the coordinates alone.
  shader->SetShaderOverride(kFogVec4Unset, sizeof(kFogVec4Unset));
  host_.SetVertexShaderProgram(shader);

  host_.PrepareDraw(0xFF341010);

  host_.SetFinalCombinerFactorC0(0.5f, 0.0f, 0.75f, 1.0f);
  host_.SetFinalCombiner0(TestHost::SRC_FOG, true, false, TestHost::SRC_C0, false, false, TestHost::SRC_DIFFUSE);

  host_.DrawArrays(host_.POSITION | host_.DIFFUSE);

  pb_print("%s\n", kUnsetTest);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kUnsetTest);
}

void FogVec4CoordTests::SetShader(const FogVec4CoordTests::TestConfig& config) const {
  auto shader = host_.GetShaderProgram();
  shader->SetShaderOverride(config.shader, config.shader_size);
  shader->SetUniformF(12, config.fog[0], config.fog[1], config.fog[2], config.fog[3]);
  // const c[13] = 1 0
  shader->SetUniformF(13, 1.0f, 0.0f);
  host_.SetVertexShaderProgram(shader);
}

std::string FogVec4CoordTests::MakeTestName(const TestConfig& config) {
  char buf[40] = {0};
  snprintf(buf, 39, "%s-%.2f_%.2f_%.2f_%.2f", config.prefix, config.fog[0], config.fog[1], config.fog[2],
           config.fog[3]);
  return buf;
}
