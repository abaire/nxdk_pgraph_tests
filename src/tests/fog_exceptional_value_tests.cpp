#include "fog_exceptional_value_tests.h"

#include <light.h>
#include <models/flat_mesh_grid_model.h>
#include <pbkit/pbkit.h>
#include <shaders/perspective_vertex_shader.h>

#include "debug_output.h"
#include "shaders/precalculated_vertex_shader.h"
#include "test_host.h"

// clang-format off
static constexpr uint32_t kVertexShader[] = {
#include "projection_vertex_shader_no_lighting_explicit_fog.vshinc"

};
// clang-format on

static constexpr float kFogStart = 1.0f;
static constexpr float kFogEnd = 200.0f;

static constexpr uint32_t kFogColor = 0xDD0000FF;

static constexpr vector_t kDiffuseUL{0.25f, 0.5f, 0.5f, 1.f};
static constexpr vector_t kDiffuseUR{0.25f, 0.25f, 0.75f, 1.f};
static constexpr vector_t kDiffuseLR{0.25f, 0.f, 1.f, 1.f};
static constexpr vector_t kDiffuseLL{0.25f, 0.75f, 0.25f, 1.f};

// Specular alpha is used for NV097_SET_FOG_GEN_MODE_V_SPEC_ALPHA
static constexpr vector_t kSpecularUL{0.f, 0.f, 0.f, 0.f};
static constexpr vector_t kSpecularUR{0.f, 0.f, 0.f, 1.f};
static constexpr vector_t kSpecularLR{0.f, 0.f, 0.f, 0.75f};
static constexpr vector_t kSpecularLL{0.f, 0.f, 0.f, 0.25f};

// Infinities
static constexpr uint32_t posInf = 0x7F800000;
static constexpr uint32_t negInf = 0xFF800000;
// Quiet NaN on x86
static constexpr uint32_t posNanQ = 0x7FC00000;
static constexpr uint32_t negNanQ = 0xFFC00000;
// Signalling NaN on x86
static constexpr uint32_t posNanS = 0x7F800001;
static constexpr uint32_t negNanS = 0xFF800001;
// Max normal
static constexpr uint32_t posMax = 0x7F7FFFFF;
static constexpr uint32_t negMax = 0xFF7FFFFF;
// Min normal
static constexpr uint32_t posMinNorm = 0x00800000;
static constexpr uint32_t negMinNorm = 0x80800000;
// Max subnormal
static constexpr uint32_t posMaxSubNorm = 0x007FFFFF;
static constexpr uint32_t negMaxSubNorm = 0x807FFFFF;
// Min subnormal
static constexpr uint32_t posMinSubNorm = 0x00000001;
static constexpr uint32_t negMinSubNorm = 0x80000001;

struct FogEntry {
  std::string name;
  uint32_t int_value;
};

static const FogEntry kFogValues[] = {
    {"0", 0},
    {"1", 0x3f800000},
    {"10", 0x41200000},
    {"100", 0x42c80000},
    {"1000", 0x447a0000},
    {"-1.24", 0xbf9eb852},
    {"-10.75", 0xc12c0000},
    {"Inf", posInf},
    {"-Inf", negInf},
    {"NanQ", posNanQ},
    {"-NanQ", negNanQ},
    {"NanS", posNanS},
    {"-NanS", negNanS},
    {"Max", posMax},
    {"-Max", negMax},
    {"MinNorm", posMinNorm},
    {"-MinNorm", negMinNorm},
    {"MaxSubn", posMaxSubNorm},
    {"-MaxSubn", negMaxSubNorm},
    {"MinSubn", posMinSubNorm},
    {"-MinSubn", negMinSubNorm},
};

static constexpr uint32_t kFogModes[] = {
    NV097_SET_FOG_MODE_V_LINEAR,  NV097_SET_FOG_MODE_V_EXP,      NV097_SET_FOG_MODE_V_EXP2,
    NV097_SET_FOG_MODE_V_EXP_ABS, NV097_SET_FOG_MODE_V_EXP2_ABS, NV097_SET_FOG_MODE_V_LINEAR_ABS,
};

static constexpr uint32_t kFogGenModes[] = {
    NV097_SET_FOG_GEN_MODE_V_SPEC_ALPHA,
    // TODO(214): Radial cases are non-deterministic
    // NV097_SET_FOG_GEN_MODE_V_RADIAL,
    NV097_SET_FOG_GEN_MODE_V_PLANAR,
    NV097_SET_FOG_GEN_MODE_V_ABS_PLANAR,
    NV097_SET_FOG_GEN_MODE_V_FOG_X,
};

static std::string MakeTestName(uint32_t fog_mode, uint32_t gen_mode) {
  std::string ret = "FogExc";

  switch (fog_mode) {
    case NV097_SET_FOG_MODE_V_LINEAR:
      ret += "-linear";
      break;
    case NV097_SET_FOG_MODE_V_EXP:
      ret += "-exp";
      break;
    case NV097_SET_FOG_MODE_V_EXP2:
      ret += "-exp2";
      break;
    case NV097_SET_FOG_MODE_V_EXP_ABS:
      ret += "-exp_abs";
      break;
    case NV097_SET_FOG_MODE_V_EXP2_ABS:
      ret += "-exp2_abs";
      break;
    case NV097_SET_FOG_MODE_V_LINEAR_ABS:
      ret += "-linear_abs";
      break;
    default:
      ASSERT(!"Unhandled fog mode");
  }

  switch (gen_mode) {
    case NV097_SET_FOG_GEN_MODE_V_SPEC_ALPHA:
      ret += "-spec_alpha";
      break;
    case NV097_SET_FOG_GEN_MODE_V_RADIAL:
      ret += "-radial";
      break;
    case NV097_SET_FOG_GEN_MODE_V_PLANAR:
      ret += "-planar";
      break;
    case NV097_SET_FOG_GEN_MODE_V_ABS_PLANAR:
      ret += "-abs_planar";
      break;
    case NV097_SET_FOG_GEN_MODE_V_FOG_X:
      ret += "-fog_x";
      break;
    default:
      ASSERT(!"Unhandled fog gen mode");
  }

  return std::move(ret);
}

/**
 * @tc FogExc-exp-abs_planar
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_EXP and fog
 *     generation mode NV097_SET_FOG_GEN_MODE_V_ABS_PLANAR.
 *
 * @tc FogExc-exp-fog_x
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_EXP and fog
 *     generation mode NV097_SET_FOG_GEN_MODE_V_FOG_X.
 *
 * @tc FogExc-exp-planar
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_EXP and fog
 *     generation mode NV097_SET_FOG_GEN_MODE_V_PLANAR.
 *
 * @tc FogExc-exp-radial
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_EXP and fog
 *     generation mode NV097_SET_FOG_GEN_MODE_V_RADIAL.
 *
 * @tc FogExc-exp-spec_alpha
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_EXP and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_SPEC_ALPHA.
 *
 * @tc FogExc-exp2-abs_planar
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_EXP2 and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_ABS_PLANAR.
 *
 * @tc FogExc-exp2-fog_x
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_EXP2 and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_FOG_X.
 *
 * @tc FogExc-exp2-planar
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_EXP2 and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_PLANAR.
 *
 * @tc FogExc-exp2-radial
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_EXP2 and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_RADIAL.
 *
 * @tc FogExc-exp2-spec_alpha
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_EXP2 and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_SPEC_ALPHA.
 *
 * @tc FogExc-exp2_abs-abs_planar
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_EXP2_ABS and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_ABS_PLANAR.
 *
 * @tc FogExc-exp2_abs-fog_x
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_EXP2_ABS and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_FOG_X.
 *
 * @tc FogExc-exp2_abs-planar
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_EXP2_ABS and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_PLANAR.
 *
 * @tc FogExc-exp2_abs-radial
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_EXP2_ABS and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_RADIAL.
 *
 * @tc FogExc-exp2_abs-spec_alpha
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_EXP2_ABS and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_SPEC_ALPHA.
 *
 * @tc FogExc-exp_abs-abs_planar
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_EXP_ABS and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_ABS_PLANAR.
 *
 * @tc FogExc-exp_abs-fog_x
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_EXP_ABS and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_FOG_X.
 *
 * @tc FogExc-exp_abs-planar
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_EXP_ABS and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_PLANAR.
 *
 * @tc FogExc-exp_abs-radial
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_EXP_ABS and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_RADIAL.
 *
 * @tc FogExc-exp_abs-spec_alpha
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_EXP_ABS and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_SPEC_ALPHA.
 *
 * @tc FogExc-linear-abs_planar
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_LINEAR and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_ABS_PLANAR.
 *
 * @tc FogExc-linear-fog_x
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_LINEAR and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_FOG_X.
 *
 * @tc FogExc-linear-planar
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_LINEAR and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_PLANAR.
 *
 * @tc FogExc-linear-radial
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_LINEAR and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_RADIAL.
 *
 * @tc FogExc-linear-spec_alpha
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_LINEAR and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_SPEC_ALPHA.
 *
 * @tc FogExc-linear_abs-abs_planar
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_LINEAR_ABS and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_ABS_PLANAR.
 *
 * @tc FogExc-linear_abs-fog_x
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_LINEAR_ABS and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_FOG_X.
 *
 * @tc FogExc-linear_abs-planar
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_LINEAR_ABS and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_PLANAR.
 *
 * @tc FogExc-linear_abs-radial
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_LINEAR_ABS and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_RADIAL.
 *
 * @tc FogExc-linear_abs-spec_alpha
 *     Tests various fog values with fog mode NV097_SET_FOG_MODE_V_LINEAR_ABS and fog generation mode
 *     NV097_SET_FOG_GEN_MODE_V_SPEC_ALPHA.
 */
FogExceptionalValueTests::FogExceptionalValueTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Fog exceptional value", config) {
  for (auto fog_gen_mode : kFogGenModes) {
    for (auto fog_mode : kFogModes) {
      std::string name = MakeTestName(fog_mode, fog_gen_mode);
      tests_[name] = [this, name, fog_mode, fog_gen_mode]() { Test(name, fog_mode, fog_gen_mode); };
    }
  }
}

void FogExceptionalValueTests::Initialize() {
  TestSuite::Initialize();

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_FOG_ENABLE, true);
    p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, true);
    p = pb_push1(p, NV097_SET_LIGHT_CONTROL, NV097_SET_LIGHT_CONTROL_V_SEPARATE_SPECULAR);
    pb_end(p);
  }

  // Just pass through the diffuse color.
  host_.SetCombinerControl(1);
  host_.SetInputColorCombiner(0, TestHost::SRC_DIFFUSE, false, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                              false, TestHost::MAP_UNSIGNED_INVERT);
  host_.SetInputAlphaCombiner(0, TestHost::SRC_DIFFUSE, true, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                              false, TestHost::MAP_UNSIGNED_INVERT);

  // Mix between the output pixel color and the fog color using the fog alpha value.
  host_.SetFinalCombiner0(TestHost::SRC_FOG, true, false, TestHost::SRC_DIFFUSE, false, false, TestHost::SRC_FOG, false,
                          false);
  host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_DIFFUSE,
                          true, false, /*specular_add_invert_r0*/ false, /* specular_add_invert_v1*/ false,
                          /* specular_clamp */ true);
}

static std::shared_ptr<PerspectiveVertexShader> SetupVertexShader(TestHost& host) {
  // Note: This could just as easily use the PassthroughVertexShader and specify all values via vertex vals, but a
  // transforming shader is used to better reproduce observed issues with Otogi:
  // https://github.com/xemu-project/xemu/issues/365
  float depth_buffer_max_value = host.GetMaxDepthBufferValue();
  auto shader = std::make_shared<PerspectiveVertexShader>(host.GetFramebufferWidth(), host.GetFramebufferHeight(), 0.0f,
                                                          depth_buffer_max_value, M_PI * 0.25f, 1.0f, 200.0f);
  shader->SetShaderOverride(kVertexShader, sizeof(kVertexShader));
  shader->SetLightingEnabled(false);
  shader->SetTransposeOnUpload();
  vector_t camera_position = {0.0f, 0.0f, -7.0f, 1.0f};
  vector_t camera_look_at = {0.0f, 0.0f, 0.0f, 1.0f};
  shader->LookAt(camera_position, camera_look_at);

  host.SetVertexShaderProgram(shader);

  return shader;
}

static void SetupFogParams(uint32_t fog_mode, uint32_t fog_gen_mode) {
  auto p = pb_begin();
  // Note: Fog color is ABGR and not ARGB
  p = pb_push1(p, NV097_SET_FOG_COLOR, kFogColor);

  p = pb_push1(p, NV097_SET_FOG_GEN_MODE, fog_gen_mode);
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
    case NV097_SET_FOG_MODE_V_LINEAR:
    case NV097_SET_FOG_MODE_V_LINEAR_ABS:
      multiplier_param = -1.0f / (kFogEnd - kFogStart);
      bias_param = 1.0f + -kFogEnd * multiplier_param;
      break;

    case NV097_SET_FOG_MODE_V_EXP:
    case NV097_SET_FOG_MODE_V_EXP_ABS:
      bias_param = 1.5f;
      multiplier_param = -fog_density / (2.0f * LN_256);
      break;

    case NV097_SET_FOG_MODE_V_EXP2:
    case NV097_SET_FOG_MODE_V_EXP2_ABS:
      bias_param = 1.5f;
      multiplier_param = -fog_density / (2.0f * SQRT_LN_256);
      break;

    default:
      break;
  }

  // TODO: Figure out what the third parameter is. In all examples I've seen it's always been 0.
  p = pb_push3f(p, NV097_SET_FOG_PARAMS, bias_param, multiplier_param, 0.0f);

  pb_end(p);
}

void FogExceptionalValueTests::Test(const std::string& name, uint32_t fog_mode, uint32_t fog_gen_mode) {
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  auto shader = SetupVertexShader(host_);
  static constexpr auto kFogValueIndex = 120 - PerspectiveVertexShader::kShaderUserConstantOffset;

  SetupFogParams(fog_mode, fog_gen_mode);

  static constexpr uint32_t kBackgroundColor = 0xFF232623;
  host_.PrepareDraw(kBackgroundColor);

  static constexpr float kQuadWidth = 64.f;
  static constexpr float kQuadHeight = 64.f;
  static constexpr float kQuadZ = 0.f;

  auto unproject = [shader](vector_t& world_point, float x, float y, float z) {
    vector_t screen_point{x, y, z, 1.f};
    shader->UnprojectPoint(world_point, screen_point, z);
  };

  auto draw_quad = [this, unproject](float left, float top) {
    const auto right = left + kQuadWidth;
    const auto bottom = top + kQuadHeight;

    host_.Begin(TestHost::PRIMITIVE_QUADS);

    vector_t world_point{0.f, 0.f, 0.f, 1.f};

    host_.SetDiffuse(kDiffuseUL);
    host_.SetSpecular(kSpecularUL);
    host_.SetNormal(-0.099014754297667f, -0.099014754297667, -0.990147542976674f);
    unproject(world_point, left, top, kQuadZ);
    host_.SetVertex(world_point);

    host_.SetDiffuse(kDiffuseUR);
    host_.SetSpecular(kSpecularUR);
    host_.SetNormal(0.099014754297667f, -0.099014754297667, -0.990147542976674f);
    unproject(world_point, right, top, kQuadZ + 2.f);
    host_.SetVertex(world_point);

    host_.SetDiffuse(kDiffuseLR);
    host_.SetSpecular(kSpecularLR);
    host_.SetNormal(0.099014754297667f, 0.099014754297667, -0.990147542976674f);
    unproject(world_point, right, bottom, kQuadZ + 2.f);
    host_.SetVertex(world_point);

    host_.SetDiffuse(kDiffuseLL);
    host_.SetSpecular(kSpecularLL);
    host_.SetNormal(-0.099014754297667f, 0.099014754297667, -0.990147542976674f);
    unproject(world_point, left, bottom, kQuadZ);
    host_.SetVertex(world_point);

    host_.End();
  };

  static constexpr float kTopPadding = 70.f;
  static constexpr float kSidePadding = 48.f;
  static constexpr float kQuadSpacingH = 24.f;
  static constexpr float kQuadSpacingV = 38.f;

  const float right = host_.GetFramebufferWidthF() - (kSidePadding + kQuadWidth);
  float left = kSidePadding;
  float top = kTopPadding;
  int text_row = 1;
  int text_col = 2;

  for (auto& fog_entry : kFogValues) {
    // Set #fog_value
    shader->SetUniformF(kFogValueIndex, *(float*)(&fog_entry.int_value), 0.f, 0.f, 0.f);
    shader->PrepareDraw();

    draw_quad(left, top);

    pb_printat(text_row, text_col, "%s", fog_entry.name.c_str());

    left += kQuadWidth + kQuadSpacingH;
    text_col += 9;
    if (left >= right) {
      left = kSidePadding;
      top += kQuadHeight + kQuadSpacingV;
      text_row += 4;
      text_col = 2;
    }
  }

  pb_printat(0, 0, "%s\n", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}
