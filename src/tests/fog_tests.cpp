#include "fog_tests.h"

#include <pbkit/pbkit.h>

#include <utility>

#include "pbkit_ext.h"
#include "shaders/perspective_vertex_shader_no_lighting.h"
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

/**
 * Constructs the test suite and creates test cases.
 *
 * @tc AFF-linear-planar
 *  Linear planar fog with fixed-function vertex processing. Four white
 *  triangles fade into reddish-purple fog as depth increases.
 *
 * @tc AFF-exp-planar
 *  Exponential planar fog with fixed-function vertex processing. Four white
 *  triangles fade exponentially into reddish-purple fog as depth increases.
 *
 * @tc AFF-exp2-planar
 *  Exponential squared planar fog with fixed-function vertex processing. Four
 *  white triangles fade into reddish-purple fog with quadratic depth falloff.
 *
 * @tc AFF-linear_abs-planar
 *  Linear absolute-value planar fog with fixed-function vertex processing.
 *
 * @tc AFF-exp_abs-planar
 *  Exponential absolute-value planar fog with fixed-function vertex processing.
 *
 * @tc AFF-exp2_abs-planar
 *  Exponential squared absolute-value planar fog with fixed-function vertex
 *  processing.
 */
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

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_FOG_ENABLE, true);
  Pushbuffer::End();
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

  Pushbuffer::Begin();
  // Note: Fog color is ABGR and not ARGB
  Pushbuffer::Push(NV097_SET_FOG_COLOR, 0x7F2030 + (fog_alpha << 24));

  Pushbuffer::Push(NV097_SET_FOG_GEN_MODE, gen_mode);
  Pushbuffer::Push(NV097_SET_FOG_MODE, fog_mode);

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
  Pushbuffer::PushF(NV097_SET_FOG_PARAMS, bias_param, multiplier_param, 0.0f);

  Pushbuffer::End();

  host_.DrawArrays(host_.POSITION | host_.DIFFUSE);

  std::string name = MakeTestName(fog_mode, gen_mode, fog_alpha);
  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();

  FinishDraw(name);
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

/**
 * Constructs the test suite and creates test cases.
 *
 * @tc AFF-linear-planar
 *  Linear planar fog using a programmable vertex shader setting oFog to
 *  the transformed vertex W coordinate. Four white triangles fade into
 *  reddish-purple fog as depth increases.
 *
 * @tc AFF-exp-planar
 *  Exponential planar fog using a programmable vertex shader setting oFog to
 *  the transformed vertex W coordinate.
 *
 * @tc AFF-exp2-planar
 *  Exponential squared planar fog using a programmable vertex shader setting
 *  oFog to the transformed vertex W coordinate.
 *
 * @tc AFF-linear_abs-planar
 *  Linear absolute-value planar fog using a programmable vertex shader setting
 *  oFog to the transformed vertex W coordinate.
 *
 * @tc AFF-exp_abs-planar
 *  Exponential absolute-value planar fog using a programmable vertex shader
 *  setting oFog to the transformed vertex W coordinate.
 *
 * @tc AFF-exp2_abs-planar
 *  Exponential squared absolute-value planar fog using a programmable vertex
 *  shader setting oFog to the transformed vertex W coordinate.
 */
FogCustomShaderTests::FogCustomShaderTests(TestHost& host, std::string output_dir, const Config& config,
                                           std::string suite_name)
    : FogTests(host, std::move(output_dir), config, std::move(suite_name)) {}

void FogCustomShaderTests::Initialize() {
  FogTests::Initialize();

  auto shader =
      std::make_shared<PerspectiveVertexShaderNoLighting>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  shader->SetNear(kFogStart);
  shader->SetFar(kFogEnd);
  vector_t camera_position{0.0f, 0.0f, -7.0f, 1.0f};
  vector_t look_at{0.0f, 0.0f, 0.0f, 1.0f};

  shader->LookAt(camera_position, look_at);

  host_.SetVertexShaderProgram(shader);
}

// FogInfiniteFogCoordinateTests

// clang-format off
static const uint32_t kInfiniteFogCShader[] = {
#include "fog_infinite_fogc_test.vshinc"
};
// clang-format on

/**
 * Constructs the test suite and creates test cases.
 *
 * @tc AFF-linear-planar
 *  Linear planar fog using a vertex shader that sets oFog to 1.0 / 0.0 (+inf).
 *  Triangles are fully obscured by the fog color due to saturated distance.
 *
 * @tc AFF-exp-planar
 *  Exponential planar fog using a vertex shader that sets oFog to +inf.
 *  Triangles are fully obscured by the fog color.
 *
 * @tc AFF-exp2-planar
 *  Exponential squared planar fog using a vertex shader that sets oFog to +inf.
 *  Triangles are fully obscured by the fog color.
 *
 * @tc AFF-linear_abs-planar
 *  Linear absolute-value planar fog using a vertex shader that sets oFog to +inf.
 *  Triangles are fully obscured by the fog color.
 *
 * @tc AFF-exp_abs-planar
 *  Exponential absolute-value planar fog using a vertex shader that sets oFog to +inf.
 *  Triangles are fully obscured by the fog color.
 *
 * @tc AFF-exp2_abs-planar
 *  Exponential squared absolute-value planar fog using a vertex shader that sets
 *  oFog to +inf. Triangles are fully obscured by the fog color.
 */
FogInfiniteFogCoordinateTests::FogInfiniteFogCoordinateTests(TestHost& host, std::string output_dir,
                                                             const Config& config)
    : FogCustomShaderTests(host, std::move(output_dir), config, "Fog inf coord") {}

void FogInfiniteFogCoordinateTests::Initialize() {
  FogCustomShaderTests::Initialize();

  auto shader = host_.GetShaderProgram();
  shader->SetShader(kInfiniteFogCShader, sizeof(kInfiniteFogCShader));
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

/**
 * Constructs the test suite and creates test cases.
 *
 * @tc CoordNotSet
 *  Tests behavior when the vertex shader does not write to the oFog register
 *  after prior draws, demonstrating carryover/residual fog coordinate state.
 *
 * @tc W-0.00_0.25_0.00_0.00
 *  Sets oFog.w via shader to test that oFog.x receives the scalar fog value.
 *
 * @tc W-0.50_0.50_0.00_0.00
 *  Sets oFog.w via shader with input (0.50, 0.50, 0.00, 0.00).
 *
 * @tc W-1.00_0.00_0.00_0.50
 *  Sets oFog.w via shader with input (1.00, 0.00, 0.00, 0.50).
 *
 * @tc W-0.30_0.30_0.30_1.00
 *  Sets oFog.w via shader with input (0.30, 0.30, 0.30, 1.00).
 *
 * @tc W_X-0.25_0.00_0.00_0.50
 *  Sets oFog.w followed by oFog.x to verify that the last-set component takes precedence.
 *
 * @tc W_X-0.65_0.00_0.00_0.00
 *  Sets oFog.w followed by oFog.x with input (0.65, 0.00, 0.00, 0.00).
 *
 * @tc W_Y-1.00_0.00_1.00_0.75
 *  Sets oFog.w followed by oFog.y with input (1.00, 0.00, 1.00, 0.75).
 *
 * @tc W_Y-0.00_0.75_0.75_0.25
 *  Sets oFog.w followed by oFog.y with input (0.00, 0.75, 0.75, 0.25).
 *
 * @tc W_Z_Y_X-0.25_0.50_0.75_1.00
 *  Sets oFog.w, then z, then y, then x sequentially.
 *
 * @tc W_Z_Y_X-1.00_0.75_0.50_0.25
 *  Sets oFog.w, then z, then y, then x sequentially with inverted values.
 *
 * @tc X-0.00_0.00_0.00_0.00
 *  Sets oFog.x to 0.00.
 *
 * @tc X-0.90_0.00_0.00_0.00
 *  Sets oFog.x to 0.90.
 *
 * @tc X_Y_Z_W-1.00_0.25_0.75_0.50
 *  Sets oFog.x, then y, then z, then w sequentially.
 *
 * @tc X_Y_Z_W-0.00_0.33_0.66_0.90
 *  Sets oFog.x, then y, then z, then w sequentially.
 *
 * @tc Y-0.00_0.00_0.00_0.00
 *  Sets oFog.y to 0.00.
 *
 * @tc Y-0.00_0.10_0.00_0.00
 *  Sets oFog.y to 0.10.
 *
 * @tc Y-0.00_0.60_0.00_0.00
 *  Sets oFog.y to 0.60.
 *
 * @tc Z-0.00_0.00_0.00_0.00
 *  Sets oFog.z to 0.00.
 *
 * @tc Z-0.00_0.00_0.20_0.00
 *  Sets oFog.z to 0.20.
 *
 * @tc Z-0.00_0.00_0.80_0.00
 *  Sets oFog.z to 0.80.
 *
 * @tc XW-0.25_0.00_0.00_0.50
 *  Sets oFog.xw simultaneously.
 *
 * @tc XW-0.65_0.00_0.00_0.00
 *  Sets oFog.xw simultaneously.
 *
 * @tc XY-0.25_0.50_0.00_0.00
 *  Sets oFog.xy simultaneously.
 *
 * @tc XY-0.65_0.75_0.00_0.00
 *  Sets oFog.xy simultaneously.
 *
 * @tc XYZ-0.25_0.50_0.75_0.00
 *  Sets oFog.xyz simultaneously.
 *
 * @tc XYZ-0.65_0.75_0.33_0.00
 *  Sets oFog.xyz simultaneously.
 *
 * @tc XYZW-0.25_0.50_0.75_1.00
 *  Sets oFog.xyzw simultaneously.
 *
 * @tc XYZW-0.65_0.75_0.33_0.00
 *  Sets oFog.xyzw simultaneously.
 *
 * @tc XZ-0.25_0.00_0.75_0.00
 *  Sets oFog.xz simultaneously.
 *
 * @tc XZ-0.65_0.00_0.33_0.00
 *  Sets oFog.xz simultaneously.
 *
 * @tc XZW-0.25_0.00_0.75_1.00
 *  Sets oFog.xzw simultaneously.
 *
 * @tc XZW-0.65_0.00_0.33_0.00
 *  Sets oFog.xzw simultaneously.
 *
 * @tc YW-0.00_0.50_0.00_1.00
 *  Sets oFog.yw simultaneously.
 *
 * @tc YW-0.00_0.75_0.00_0.00
 *  Sets oFog.yw simultaneously.
 *
 * @tc YZ-0.00_0.50_0.75_0.00
 *  Sets oFog.yz simultaneously.
 *
 * @tc YZ-0.00_0.75_0.33_0.00
 *  Sets oFog.yz simultaneously.
 *
 * @tc YZW-0.00_0.50_0.75_1.00
 *  Sets oFog.yzw simultaneously.
 *
 * @tc YZW-0.00_0.75_0.33_0.00
 *  Sets oFog.yzw simultaneously.
 *
 * @tc ZW-0.00_0.00_0.75_1.00
 *  Sets oFog.zw simultaneously.
 *
 * @tc ZW-0.00_0.00_0.33_0.00
 *  Sets oFog.zw simultaneously.
 */
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

  Pushbuffer::Begin();
  // Note: Fog color is ABGR and not ARGB
  Pushbuffer::Push(NV097_SET_FOG_COLOR, 0x00FF00);
  // Gen mode does not seem to matter when using a vertex shader.
  Pushbuffer::Push(NV097_SET_FOG_GEN_MODE, FOG_GEN_SPEC_ALPHA);
  Pushbuffer::Push(NV097_SET_FOG_MODE, FOG_LINEAR);

  // The final fog calculation should be exactly the output of the shader.
  Pushbuffer::PushF(NV097_SET_FOG_PARAMS, 1.0f, 1.0f, 0.0f);
  Pushbuffer::End();
}

void FogVec4CoordTests::Test(const TestConfig& config) {
  SetShader(config);
  host_.SetFinalCombiner0(TestHost::SRC_FOG, true, false, TestHost::SRC_FOG, false, false, TestHost::SRC_DIFFUSE);

  host_.PrepareDraw(0xFF303030);
  host_.DrawArrays(host_.POSITION | host_.DIFFUSE);

  std::string name = MakeTestName(config);
  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();

  FinishDraw(name);
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
  shader->SetShader(kFogVec4Unset, sizeof(kFogVec4Unset));
  host_.SetVertexShaderProgram(shader);

  host_.PrepareDraw(0xFF341010);

  host_.SetFinalCombinerFactorC0(0.5f, 0.0f, 0.75f, 1.0f);
  host_.SetFinalCombiner0(TestHost::SRC_FOG, true, false, TestHost::SRC_C0, false, false, TestHost::SRC_DIFFUSE);

  host_.DrawArrays(host_.POSITION | host_.DIFFUSE);

  pb_print("%s\n", kUnsetTest);
  pb_draw_text_screen();

  FinishDraw(kUnsetTest);
}

void FogVec4CoordTests::SetShader(const FogVec4CoordTests::TestConfig& config) const {
  auto shader = host_.GetShaderProgram();
  shader->SetShader(config.shader, config.shader_size);

  auto index = 120 - PerspectiveVertexShader::kShaderUserConstantOffset;
  // c[120].xyzw = fog test value
  shader->SetUniformF(index++, config.fog[0], config.fog[1], config.fog[2], config.fog[3]);
  // #one_and_zero vector
  shader->SetUniformF(index++, 1.0f, 0.0f);
  host_.SetVertexShaderProgram(shader);
}

std::string FogVec4CoordTests::MakeTestName(const TestConfig& config) {
  char buf[40] = {0};
  snprintf(buf, 39, "%s-%.2f_%.2f_%.2f_%.2f", config.prefix, config.fog[0], config.fog[1], config.fog[2],
           config.fog[3]);
  return buf;
}

// FogPlanarVertexShaderTests

// clang-format off
static constexpr uint32_t kVertexShader[] = {
#include "projection_vertex_shader_no_lighting_explicit_fog.vshinc"
};

static constexpr uint32_t kRCPFogVertexShader[] = {
#include "projection_vertex_shader_no_lighting_rcp_fog.vshinc"
};
// clang-format on

/**
 * Constructs the test suite and creates test cases reproducing the planar fog
 * configuration used during the Tron 2.0 loading screen / glow blur passes.
 *
 * @tc ZeroPlane_RCP1_Planar
 *  Reproduces the exact Tron 2.0 glow accumulation pass fog setup: linear fog
 *  mode, planar gen mode with a zero plane (0,0,0,0), parameters
 *  bias = 2.48148155, multiplier = -0.0007407407, and vertex shader setting
 *  oFog via `rcp oFog, 1.0`. Tests whether hardware clamps Fog.a to 1.0 or
 *  evaluates planar fog differently.
 *
 * @tc ZeroPlane_RCP1_FogX
 *  Same as ZeroPlane_RCP1_Planar but using FOG_GEN_MODE_V_FOG_X, testing whether
 *  planar vs fog_x generation mode alters vertex shader fog coordinate handling.
 *
 * @tc ZeroPlane_MOV1_Planar
 *  Same as ZeroPlane_RCP1_Planar but using `mov oFog, 1.0` instead of `rcp`,
 *  verifying that the instruction opcode does not affect the fog unit.
 *
 * @tc ZeroPlane_RCP0_Planar
 *  Tests planar fog with zero plane when the vertex shader sets oFog via
 *  `rcp oFog, 0.0` (+inf), verifying saturation to full fog.
 *
 * @tc Sweep_Planar
 *  Displays Fog.a directly as grayscale across a sweep of oFog values
 *  (0.0, 1.0, 650.0, 1000.0, 1325.0, 1650.0, 2000.0, 2500.0) under Tron 2.0
 *  linear fog parameters using FOG_GEN_MODE_V_PLANAR.
 *
 * @tc Sweep_FogX
 *  Displays Fog.a directly as grayscale across the same oFog sweep using
 *  FOG_GEN_MODE_V_FOG_X for comparison with planar mode.
 */
FogPlanarVertexShaderTests::FogPlanarVertexShaderTests(TestHost& host, std::string output_dir, const Config& config)
    : FogCustomShaderTests(host, std::move(output_dir), config, "Fog planar vsh") {
  tests_.clear();

  tests_["ZeroPlane_RCP1_Planar"] = [this]() {
    TestPlanarZeroPlane("ZeroPlane_RCP1_Planar", /*rcp=*/true, 1.0f, NV097_SET_FOG_GEN_MODE_V_PLANAR);
  };
  tests_["ZeroPlane_RCP1_FogX"] = [this]() {
    TestPlanarZeroPlane("ZeroPlane_RCP1_FogX", /*rcp=*/true, 1.0f, NV097_SET_FOG_GEN_MODE_V_FOG_X);
  };
  tests_["ZeroPlane_MOV1_Planar"] = [this]() {
    TestPlanarZeroPlane("ZeroPlane_MOV1_Planar", /*rcp=*/false, 1.0f, NV097_SET_FOG_GEN_MODE_V_PLANAR);
  };
  tests_["ZeroPlane_RCP0_Planar"] = [this]() {
    TestPlanarZeroPlane("ZeroPlane_RCP0_Planar", /*rcp=*/true, 0.0f, NV097_SET_FOG_GEN_MODE_V_PLANAR);
  };
  tests_["Sweep_Planar"] = [this]() { TestFogFactorSweep("Sweep_Planar", NV097_SET_FOG_GEN_MODE_V_PLANAR); };
  tests_["Sweep_FogX"] = [this]() { TestFogFactorSweep("Sweep_FogX", NV097_SET_FOG_GEN_MODE_V_FOG_X); };
}

void FogPlanarVertexShaderTests::CreateGeometry() {
  constexpr int kNumTriangles = 4;
  vertex_buffer_ = host_.AllocateVertexBuffer(kNumTriangles * 3);

  // Use 50% gray diffuse so Fog.a values above 1.0 (unclamped) produce brighter pixels
  // (e.g. 1.48 * 0.5 = 0.74) while 1.0 (clamped) produces exactly 0.5.
  Color diffuse = {0.5f, 0.5f, 0.5f, 1.0f};
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

void FogPlanarVertexShaderTests::TestPlanarZeroPlane(const std::string& name, bool rcp, float fog_value,
                                                     uint32_t gen_mode) {
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  auto shader =
      std::make_shared<PerspectiveVertexShaderNoLighting>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  if (rcp) {
    shader->SetShader(kRCPFogVertexShader, sizeof(kRCPFogVertexShader));
  } else {
    shader->SetShader(kVertexShader, sizeof(kVertexShader));
  }
  shader->SetTransposeOnUpload();
  vector_t camera_position{0.0f, 0.0f, -7.0f, 1.0f};
  vector_t look_at{0.0f, 0.0f, 0.0f, 1.0f};
  shader->LookAt(camera_position, look_at);

  static constexpr auto kFogValueIndex = 120 - PerspectiveVertexShader::kShaderUserConstantOffset;
  shader->SetUniformF(kFogValueIndex, fog_value, 0.0f, 0.0f, 0.0f);
  host_.SetVertexShaderProgram(shader);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_FOG_ENABLE, true);
  Pushbuffer::Push(NV097_SET_FOG_COLOR, 0x00000000);
  Pushbuffer::Push(NV097_SET_FOG_GEN_MODE, gen_mode);
  Pushbuffer::Push(NV097_SET_FOG_MODE, NV097_SET_FOG_MODE_V_LINEAR);
  Pushbuffer::PushF(NV097_SET_FOG_PLANE, 0.0f, 0.0f, 0.0f, 0.0f);

  // Tron 2.0 fog params: bias = 2.48148155f, multiplier = -0.0007407407f, quadratic = 0.0f
  Pushbuffer::PushF(NV097_SET_FOG_PARAMS, 2.48148155f, -0.0007407407f, 0.0f);
  Pushbuffer::End();

  // out.rgb = Fog.a * Diffuse.rgb + (1 - Fog.a) * Fog.rgb
  host_.SetFinalCombiner0(TestHost::SRC_FOG, true, false, TestHost::SRC_DIFFUSE, false, false, TestHost::SRC_FOG, false,
                          false);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

  static constexpr uint32_t kBackgroundColor = 0xFF232623;
  host_.PrepareDraw(kBackgroundColor);

  host_.DrawArrays(host_.POSITION | host_.DIFFUSE);

  pb_print("%s\n", name.c_str());
  pb_print("GenMode: %s\n", gen_mode == NV097_SET_FOG_GEN_MODE_V_PLANAR ? "PLANAR" : "FOG_X");
  pb_print("Shader: %s, fog_val: %.2f\n", rcp ? "rcp" : "mov", fog_value);
  pb_draw_text_screen();

  FinishDraw(name);
}

void FogPlanarVertexShaderTests::TestFogFactorSweep(const std::string& name, uint32_t gen_mode) {
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  auto shader =
      std::make_shared<PerspectiveVertexShaderNoLighting>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  shader->SetShader(kVertexShader, sizeof(kVertexShader));
  shader->SetTransposeOnUpload();
  vector_t camera_position{0.0f, 0.0f, -7.0f, 1.0f};
  vector_t look_at{0.0f, 0.0f, 0.0f, 1.0f};
  shader->LookAt(camera_position, look_at);
  host_.SetVertexShaderProgram(shader);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_FOG_ENABLE, true);
  Pushbuffer::Push(NV097_SET_FOG_COLOR, 0x00000000);
  Pushbuffer::Push(NV097_SET_FOG_GEN_MODE, gen_mode);
  Pushbuffer::Push(NV097_SET_FOG_MODE, NV097_SET_FOG_MODE_V_LINEAR);
  Pushbuffer::PushF(NV097_SET_FOG_PLANE, 0.0f, 0.0f, 0.0f, 0.0f);
  Pushbuffer::PushF(NV097_SET_FOG_PARAMS, 2.48148155f, -0.0007407407f, 0.0f);
  Pushbuffer::End();

  // Final combiner: display Fog.a directly as grayscale
  // out.rgb = Fog.a * Diffuse.rgb + (1 - Fog.a) * Zero = Fog.aaa
  host_.SetFinalCombiner0(TestHost::SRC_FOG, true, false, TestHost::SRC_DIFFUSE, false, false, TestHost::SRC_ZERO,
                          false, false);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, false, true);

  static constexpr uint32_t kBackgroundColor = 0xFF202020;
  host_.PrepareDraw(kBackgroundColor);

  static const float kSweepValues[] = {0.0f, 1.0f, 650.0f, 1000.0f, 1325.0f, 1650.0f, 2000.0f, 2500.0f};
  static constexpr int kNumSwatches = sizeof(kSweepValues) / sizeof(kSweepValues[0]);

  static constexpr auto kFogValueIndex = 120 - PerspectiveVertexShader::kShaderUserConstantOffset;

  static constexpr float kQuadWidth = 56.f;
  static constexpr float kQuadHeight = 260.f;
  static constexpr float kSpacing = 16.f;
  static constexpr float kStartX = 36.f;
  static constexpr float kStartY = 90.f;

  auto unproject = [shader](vector_t& world_point, float x, float y, float z) {
    vector_t screen_point{x, y, z, 1.f};
    shader->UnprojectPoint(world_point, screen_point, z);
  };

  for (int i = 0; i < kNumSwatches; ++i) {
    float fog_val = kSweepValues[i];
    shader->SetUniformF(kFogValueIndex, fog_val, 0.0f, 0.0f, 0.0f);
    shader->PrepareDraw();

    float left = kStartX + i * (kQuadWidth + kSpacing);
    float right = left + kQuadWidth;
    float top = kStartY;
    float bottom = top + kQuadHeight;

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(1.0f, 1.0f, 1.0f, 1.0f);

    vector_t p{0.f, 0.f, 0.f, 1.f};
    unproject(p, left, top, 0.0f);
    host_.SetVertex(p);

    unproject(p, right, top, 0.0f);
    host_.SetVertex(p);

    unproject(p, right, bottom, 0.0f);
    host_.SetVertex(p);

    unproject(p, left, bottom, 0.0f);
    host_.SetVertex(p);

    host_.End();
  }

  pb_print("%s\n", name.c_str());
  pb_print("GenMode: %s  (near=650, far=2000)\n", gen_mode == NV097_SET_FOG_GEN_MODE_V_PLANAR ? "PLANAR" : "FOG_X");
  pb_print("Swatches: 0, 1, 650, 1000, 1325, 1650, 2000, 2500\n");
  pb_draw_text_screen();

  FinishDraw(name);
}
