#include "fog_param_tests.h"

#include <light.h>
#include <models/flat_mesh_grid_model.h>
#include <pbkit/pbkit.h>
#include <shaders/perspective_vertex_shader.h>

#include "debug_output.h"
#include "test_host.h"

// In ABGR format.
static constexpr uint32_t kFogColor = 0xFF0000FF;

static constexpr vector_t kDiffuse{0.f, 0.f, 1.f, 1.f};
static constexpr vector_t kBiasZeroDiffuse{0.f, 0.5f, 0.5f, 1.f};

static constexpr float kTestValues[] = {1.f, 2.f, -1.f, -2.f, 0.5f, -0.5f, -0.25f, 0.25f};

static constexpr uint32_t kFogModes[] = {
    NV097_SET_FOG_MODE_V_LINEAR,  NV097_SET_FOG_MODE_V_EXP,      NV097_SET_FOG_MODE_V_EXP2,
    NV097_SET_FOG_MODE_V_EXP_ABS, NV097_SET_FOG_MODE_V_EXP2_ABS, NV097_SET_FOG_MODE_V_LINEAR_ABS,
};

static std::string FogModeSuffix(uint32_t fog_mode) {
  switch (fog_mode) {
    case NV097_SET_FOG_MODE_V_LINEAR:
      return "-lin";
    case NV097_SET_FOG_MODE_V_EXP:
      return "-exp";
    case NV097_SET_FOG_MODE_V_EXP2:
      return "-exp2";
    case NV097_SET_FOG_MODE_V_EXP_ABS:
      return "-exp_abs";
    case NV097_SET_FOG_MODE_V_EXP2_ABS:
      return "-exp2_abs";
    case NV097_SET_FOG_MODE_V_LINEAR_ABS:
      return "-lin_abs";
    default:
      ASSERT(!"Unhandled fog mode");
  }
}

static std::string MakeTestName(uint32_t fog_mode, const float linear) {
  std::string ret = "FogParam";

  char buf[32];
  snprintf(buf, sizeof(buf), "%.2f", linear);
  ret += buf;

  ret += FogModeSuffix(fog_mode);

  return std::move(ret);
}

static std::string MakeZeroBiasTestName(uint32_t fog_mode) { return "FogZeroBias" + FogModeSuffix(fog_mode); }

//! Returns the bias parameter needed to remove all fog.
static float ZeroBias(uint32_t fog_mode) {
  switch (fog_mode) {
    case NV097_SET_FOG_MODE_V_LINEAR:
      return 2.f;
    case NV097_SET_FOG_MODE_V_EXP:
      return 1.51f;
    case NV097_SET_FOG_MODE_V_EXP2:
      return 1.5f;
    case NV097_SET_FOG_MODE_V_EXP_ABS:
      return 1.5075f;
    case NV097_SET_FOG_MODE_V_EXP2_ABS:
      return 1.5f;
    case NV097_SET_FOG_MODE_V_LINEAR_ABS:
      return 2.f;
    default:
      ASSERT(!"Unhandled fog mode");
  }
}

/**
 * @tc FogZeroBias-exp2_abs
 *  Tests the bias parameter for NV097_SET_FOG_MODE_V_EXP2_ABS. Renders a series of quads and increments the bias fog
 * param by 0.005 for each, starting at 1.0. The multiplier params are set to 0 so the fog coord (1.0) is irrelevant.
 *
 * Indicates that the zero point is 1.5
 *
 * @tc FogZeroBias-exp2
 *  Tests the bias parameter for NV097_SET_FOG_MODE_V_EXP2. Renders a series of quads and increments the bias fog param
 * by 0.005 for each, starting at 1.0. The multiplier params are set to 0 so the fog coord (1.0) is irrelevant.
 *
 * Indicates that the zero point is 1.5
 *
 * @tc FogZeroBias-exp_abs
 *  Tests the bias parameter for NV097_SET_FOG_MODE_V_EXP_ABS. Renders a series of quads and increments the bias fog
 * param by 0.005 for each, starting at 1.0. The multiplier params are set to 0 so the fog coord (1.0) is irrelevant.
 *
 * Indicates that the zero point is ~1.5075
 *
 * @tc FogZeroBias-exp
 *  Tests the bias parameter for NV097_SET_FOG_MODE_V_EXP. Renders a series of quads and increments the bias fog param
 * by 0.005 for each, starting at 1.0. The multiplier params are set to 0 so the fog coord (1.0) is irrelevant.
 *
 * Indicates that the zero point is 1.51
 *
 * @tc FogZeroBias-lin_abs
 *  Tests the bias parameter for NV097_SET_FOG_MODE_V_LINEAR_ABS. Renders a series of quads and increments the bias fog
 * param by 0.005 for each, starting at 1.0. The multiplier params are set to 0 so the fog coord (1.0) is irrelevant.
 *
 * Indicates that the zero point is 2.0
 *
 * @tc FogZeroBias-lin
 *  Tests the bias parameter for NV097_SET_FOG_MODE_V_LINEAR. Renders a series of quads and increments the bias fog
 * param by 0.005 for each, starting at 1.0. The multiplier params are set to 0 so the fog coord (1.0) is irrelevant.
 *
 * Indicates that the zero point is 2.0
 *
 * @tc FogParam-0.25-exp2_abs
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP2_ABS with the bias param set to the zero point for the type and the
 *  multiplier set to -0.25. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam0.25-exp2_abs
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP2_ABS with the bias param set to the zero point for the type and the
 *  multiplier set to 0.25. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam-0.25-exp2
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP2 with the bias param set to the zero point for the type and the multiplier
 *  set to -0.25. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam0.25-exp2
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP2 with the bias param set to the zero point for the type and the multiplier
 *  set to 0.25. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam-0.25-exp_abs
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP_ABS with the bias param set to the zero point for the type and the
 *  multiplier set to -0.25. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam0.25-exp_abs
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP_ABS with the bias param set to the zero point for the type and the
 *  multiplier set to 0.25. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam-0.25-exp
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP with the bias param set to the zero point for the type and the multiplier
 *  set to -0.25. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam0.25-exp
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP with the bias param set to the zero point for the type and the multiplier
 *  set to 0.25. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam-0.25-lin_abs
 *   Tests fog mode NV097_SET_FOG_MODE_V_LINEAR_ABS with the bias param set to the zero point for the type and the
 *  multiplier set to -0.25. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam0.25-lin_abs
 *   Tests fog mode NV097_SET_FOG_MODE_V_LINEAR_ABS with the bias param set to the zero point for the type and the
 *  multiplier set to 0.25. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam-0.25-lin
 *   Tests fog mode NV097_SET_FOG_MODE_V_LINEAR with the bias param set to the zero point for the type and the
 *  multiplier set to -0.25. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam0.25-lin
 *   Tests fog mode NV097_SET_FOG_MODE_V_LINEAR with the bias param set to the zero point for the type and the
 *  multiplier set to 0.25. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam-0.50-exp2_abs
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP2_ABS with the bias param set to the zero point for the type and the
 *  multiplier set to -0.50. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam0.50-exp2_abs
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP2_ABS with the bias param set to the zero point for the type and the
 *  multiplier set to XXXX. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam-0.50-exp2
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP2 with the bias param set to the zero point for the type and the multiplier
 *  set to -0.50. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam0.50-exp2
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP2 with the bias param set to the zero point for the type and the multiplier
 *  set to 0.50. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam-0.50-exp_abs
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP_ABS with the bias param set to the zero point for the type and the
 *  multiplier set to -0.50. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam0.50-exp_abs
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP_ABS with the bias param set to the zero point for the type and the
 *  multiplier set to 0.50. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam-0.50-exp
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP with the bias param set to the zero point for the type and the multiplier
 *  set to -0.50. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam0.50-exp
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP with the bias param set to the zero point for the type and the multiplier
 *  set to 0.50. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam-0.50-lin_abs
 *   Tests fog mode NV097_SET_FOG_MODE_V_LINEAR_ABS with the bias param set to the zero point for the type and the
 *  multiplier set to -0.50. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam0.50-lin_abs
 *   Tests fog mode NV097_SET_FOG_MODE_V_LINEAR_ABS with the bias param set to the zero point for the type and the
 *  multiplier set to 0.50. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam-0.50-lin
 *   Tests fog mode NV097_SET_FOG_MODE_V_LINEAR with the bias param set to the zero point for the type and the
 *  multiplier set to -0.50. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam0.50-lin
 *   Tests fog mode NV097_SET_FOG_MODE_V_LINEAR with the bias param set to the zero point for the type and the
 *  multiplier set to 0.50. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam-1.00-exp2_abs
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP2_ABS with the bias param set to the zero point for the type and the
 *  multiplier set to -1.0. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam1.00-exp2_abs
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP2_ABS with the bias param set to the zero point for the type and the
 *  multiplier set to 1.0. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam-1.00-exp2
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP2 with the bias param set to the zero point for the type and the multiplier
 *  set to -1.0. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam1.00-exp2
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP2 with the bias param set to the zero point for the type and the multiplier
 *  set to 1.0. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam-1.00-exp_abs
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP_ABS with the bias param set to the zero point for the type and the
 *  multiplier set to -1.0. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam1.00-exp_abs
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP_ABS with the bias param set to the zero point for the type and the
 *  multiplier set to 1.0. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam-1.00-exp
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP with the bias param set to the zero point for the type and the multiplier
 *  set to -1.0. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam1.00-exp
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP with the bias param set to the zero point for the type and the multiplier
 *  set to 1.0. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam-1.00-lin_abs
 *   Tests fog mode NV097_SET_FOG_MODE_V_LINEAR_ABS with the bias param set to the zero point for the type and the
 *  multiplier set to -1.0. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam1.00-lin_abs
 *   Tests fog mode NV097_SET_FOG_MODE_V_LINEAR_ABS with the bias param set to the zero point for the type and the
 *  multiplier set to 1.0. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam-1.00-lin
 *   Tests fog mode NV097_SET_FOG_MODE_V_LINEAR with the bias param set to the zero point for the type and the
 *  multiplier set to -1.0. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam1.00-lin
 *   Tests fog mode NV097_SET_FOG_MODE_V_LINEAR with the bias param set to the zero point for the type and the
 *  multiplier set to 1.0. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam-2.00-exp2_abs
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP2_ABS with the bias param set to the zero point for the type and the
 *  multiplier set to -2.0. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam2.00-exp2_abs
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP2_ABS with the bias param set to the zero point for the type and the
 *  multiplier set to 2.0. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam-2.00-exp2
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP2 with the bias param set to the zero point for the type and the multiplier
 *  set to -2.0. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam2.00-exp2
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP2 with the bias param set to the zero point for the type and the multiplier
 *  set to 2.0. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam-2.00-exp_abs
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP_ABS with the bias param set to the zero point for the type and the
 *  multiplier set to -2.0. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam2.00-exp_abs
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP_ABS with the bias param set to the zero point for the type and the
 *  multiplier set to 2.0. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam-2.00-exp
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP with the bias param set to the zero point for the type and the multiplier
 *  set to -2.0. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam2.00-exp
 *   Tests fog mode NV097_SET_FOG_MODE_V_EXP with the bias param set to the zero point for the type and the multiplier
 *  set to 2.0. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam-2.00-lin_abs
 *   Tests fog mode NV097_SET_FOG_MODE_V_LINEAR_ABS with the bias param set to the zero point for the type and the
 *  multiplier set to -2.0. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam2.00-lin_abs
 *   Tests fog mode NV097_SET_FOG_MODE_V_LINEAR_ABS with the bias param set to the zero point for the type and the
 *  multiplier set to 2.0. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam-2.00-lin
 *   Tests fog mode NV097_SET_FOG_MODE_V_LINEAR with the bias param set to the zero point for the type and the
 *  multiplier set to -2.0. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 * @tc FogParam2.00-lin
 *   Tests fog mode NV097_SET_FOG_MODE_V_LINEAR with the bias param set to the zero point for the type and the
 *  multiplier set to 2.0. Quads are rendered with fog coordinates starting at -1.4 and increasing by 0.01 per quad.
 *
 */
FogParamTests::FogParamTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Fog param", config) {
  for (auto fog_mode : kFogModes) {
    for (auto linear : kTestValues) {
      std::string name = MakeTestName(fog_mode, linear);
      tests_[name] = [this, name, fog_mode, linear]() {
        float bias = ZeroBias(fog_mode);
        Test(name, fog_mode, bias, linear);
      };
    }

    {
      auto name = MakeZeroBiasTestName(fog_mode);
      tests_[name] = [this, name, fog_mode]() { TestBiasZeroPoint(name, fog_mode); };
    }
  }
}

void FogParamTests::Initialize() {
  TestSuite::Initialize();

  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_FOG_ENABLE, true);
    // Note: Fog color is ABGR and not ARGB
    Pushbuffer::Push(NV097_SET_FOG_COLOR, kFogColor);
    Pushbuffer::Push(NV097_SET_FOG_GEN_MODE, NV097_SET_FOG_GEN_MODE_V_FOG_X);
    Pushbuffer::PushF(NV097_SET_FOG_PLANE, 0.f, 0.f, 2.f, 0.f);
    Pushbuffer::End();
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

static void UnprojectPoint(const TestHost& host, vector_t& world_point, float x, float y, float z) {
  vector_t screen_point{x, y, z, 1.f};
  host.UnprojectPoint(world_point, screen_point, z);
}

void FogParamTests::Test(const std::string& name, uint32_t fog_mode, float bias, float linear) {
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  host_.SetVertexShaderProgram(nullptr);

  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_FOG_MODE, fog_mode);
    // The last parameter never seems to have an effect.
    Pushbuffer::PushF(NV097_SET_FOG_PARAMS, bias, linear, 0.f);
    Pushbuffer::End();
  }

  static constexpr uint32_t kBackgroundColor = 0xFF231F23;
  host_.PrepareDraw(kBackgroundColor);

  static constexpr float kQuadWidth = 24.f;
  static constexpr float kQuadHeight = 24.f;
  static constexpr float kQuadZRightInc = 1.f;

  auto draw_quad = [this](float left, float top, float z) {
    const auto right = left + kQuadWidth;
    const auto bottom = top + kQuadHeight;

    host_.Begin(TestHost::PRIMITIVE_QUADS);

    vector_t world_point{0.f, 0.f, 0.f, 1.f};
    host_.SetNormal(0.f, 0.f, -1.f);
    host_.SetDiffuse(kDiffuse);

    UnprojectPoint(host_, world_point, left, top, z);
    host_.SetVertex(world_point);

    UnprojectPoint(host_, world_point, right, top, z + kQuadZRightInc);
    host_.SetVertex(world_point);

    UnprojectPoint(host_, world_point, right, bottom, z + kQuadZRightInc);
    host_.SetVertex(world_point);

    UnprojectPoint(host_, world_point, left, bottom, z);
    host_.SetVertex(world_point);

    host_.End();
  };

  static constexpr float kTopPadding = 70.f;
  static constexpr float kSidePadding = 60.f;
  static constexpr float kQuadSpacingH = 1.f;
  static constexpr float kQuadSpacingV = 1.f;

  const float right = host_.GetFramebufferWidthF() - (kSidePadding + kQuadWidth);
  const float bottom = host_.GetFramebufferHeightF() - kQuadHeight;
  float left = kSidePadding;
  float top = kTopPadding;

  float fog_x = -1.4f;
  static constexpr float kFogXInc = 0.01f;

  while (top < bottom) {
    host_.SetFogCoord(fog_x);
    draw_quad(left, top, 0.f);

    left += kQuadWidth + kQuadSpacingH;
    if (left >= right) {
      left = kSidePadding;
      top += kQuadHeight + kQuadSpacingV;
    }

    fog_x += kFogXInc;
  }

  pb_print("%s\n", name.c_str());
  pb_printat(2, 0, "-1.4");
  pb_printat(3, 0, "-1.2");
  pb_printat(4, 0, "-1.0");
  pb_printat(5, 0, "-0.8");
  pb_printat(6, 0, "-0.6");
  pb_printat(7, 0, "-0.4");
  pb_printat(8, 0, "-0.2");
  pb_printat(9, 0, " 0");
  pb_printat(10, 0, " 0.2");
  pb_printat(11, 0, " 0.4");
  pb_printat(12, 0, " 0.6");
  pb_printat(13, 0, " 0.8");
  pb_printat(14, 0, " 1.0");
  pb_printat(15, 0, " 1.2");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

void FogParamTests::TestBiasZeroPoint(const std::string& name, uint32_t fog_mode) {
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  host_.SetVertexShaderProgram(nullptr);

  {
    Pushbuffer::Begin();
    // Note: Fog color is ABGR and not ARGB
    Pushbuffer::Push(NV097_SET_FOG_COLOR, kFogColor);

    Pushbuffer::Push(NV097_SET_FOG_GEN_MODE, NV097_SET_FOG_GEN_MODE_V_FOG_X);
    Pushbuffer::Push(NV097_SET_FOG_MODE, fog_mode);

    Pushbuffer::PushF(NV097_SET_FOG_PLANE, 0.f, 0.f, 2.f, 0.f);
    Pushbuffer::End();
  }

  static constexpr uint32_t kBackgroundColor = 0xFF231F23;
  host_.PrepareDraw(kBackgroundColor);

  static constexpr float kQuadWidth = 24.f;
  static constexpr float kQuadHeight = 24.f;
  static constexpr float kQuadZ = 1.f;

  auto draw_quad = [this](float left, float top) {
    const auto right = left + kQuadWidth;
    const auto bottom = top + kQuadHeight;

    host_.Begin(TestHost::PRIMITIVE_QUADS);

    vector_t world_point{0.f, 0.f, 0.f, 1.f};
    host_.SetNormal(0.f, 0.f, -1.f);
    host_.SetDiffuse(kBiasZeroDiffuse);

    UnprojectPoint(host_, world_point, left, top, kQuadZ);
    host_.SetVertex(world_point);

    UnprojectPoint(host_, world_point, right, top, kQuadZ);
    host_.SetVertex(world_point);

    UnprojectPoint(host_, world_point, right, bottom, kQuadZ);
    host_.SetVertex(world_point);

    UnprojectPoint(host_, world_point, left, bottom, kQuadZ);
    host_.SetVertex(world_point);

    host_.End();
  };
  host_.SetFogCoord(-1.f);

  static constexpr float kTopPadding = 46.f;
  static constexpr float kSidePadding = 48.f;
  static constexpr float kQuadSpacingH = 2.f;
  static constexpr float kQuadSpacingV = 1.f;

  const float right = host_.GetFramebufferWidthF() - (kSidePadding + kQuadWidth);
  const float bottom = host_.GetFramebufferHeightF() - kQuadHeight;
  float left = kSidePadding;
  float top = kTopPadding;

  static constexpr float kBiasInc = 0.005f;

  float bias = 1.f;

  while (top < bottom) {
    {
      Pushbuffer::Begin();
      Pushbuffer::PushF(NV097_SET_FOG_PARAMS, bias, 0.f, 0.f);
      Pushbuffer::End();
    }

    draw_quad(left, top);

    left += kQuadWidth + kQuadSpacingH;
    if (left >= right) {
      left = kSidePadding;
      top += kQuadHeight + kQuadSpacingV;
    }

    bias += kBiasInc;
  }

  pb_print("%s\n", name.c_str());

  for (auto i = 0; i < 15; ++i) {
    pb_printat(i + 1, 0, "%d.%d", 1 + (i / 10), i % 10);
  }

  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}
