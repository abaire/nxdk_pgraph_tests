#include "fog_carryover_tests.h"

#include <light.h>
#include <models/flat_mesh_grid_model.h>
#include <pbkit/pbkit.h>
#include <shaders/passthrough_vertex_shader.h>

#include "debug_output.h"
#include "test_host.h"

// clang-format off
static constexpr uint32_t kShader[] = {
#include "passthrough_just_position_and_color.vshinc"

};
// clang-format on

// In ABGR format.
static constexpr uint32_t kFogColor = 0xFF0000FF;

static constexpr char kTestName[] = "FogCarryover";
static constexpr vector_t kDiffuse{0.f, 0.f, 1.f, 1.f};

static constexpr uint32_t kFogModes[] = {
    NV097_SET_FOG_MODE_V_LINEAR,  NV097_SET_FOG_MODE_V_EXP,      NV097_SET_FOG_MODE_V_EXP2,
    NV097_SET_FOG_MODE_V_EXP_ABS, NV097_SET_FOG_MODE_V_EXP2_ABS, NV097_SET_FOG_MODE_V_LINEAR_ABS,
};

static std::string FogModeName(uint32_t fog_mode) {
  switch (fog_mode) {
    case NV097_SET_FOG_MODE_V_LINEAR:
      return "lin";
    case NV097_SET_FOG_MODE_V_EXP:
      return "exp";
    case NV097_SET_FOG_MODE_V_EXP2:
      return "exp2";
    case NV097_SET_FOG_MODE_V_EXP_ABS:
      return "exp_abs";
    case NV097_SET_FOG_MODE_V_EXP2_ABS:
      return "exp2_abs";
    case NV097_SET_FOG_MODE_V_LINEAR_ABS:
      return "lin_abs";
    default:
      ASSERT(!"Unhandled fog mode");
  }
}

//! Returns the bias parameter needed to remove all fog.
static float ZeroBias(uint32_t fog_mode) {
  switch (fog_mode) {
    case NV097_SET_FOG_MODE_V_LINEAR:
    case NV097_SET_FOG_MODE_V_LINEAR_ABS:
      return 2.f;
    case NV097_SET_FOG_MODE_V_EXP:
    case NV097_SET_FOG_MODE_V_EXP_ABS:
      return 1.51f;
    case NV097_SET_FOG_MODE_V_EXP2:
    case NV097_SET_FOG_MODE_V_EXP2_ABS:
      return 1.5f;
    default:
      ASSERT(!"Unhandled fog mode");
  }
}

/**
 * @tc FogCarryover
 *   The left column triangles are rendered with the fog coordinate explicitly passed through from the vertex by the
 *   shader. The right column triangles do not specify the fog coordinate at all.
 */
FogCarryoverTests::FogCarryoverTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Fog carryover", config) {
  tests_[kTestName] = [this]() { Test(); };
}

void FogCarryoverTests::Initialize() {
  TestSuite::Initialize();

  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_FOG_ENABLE, true);
    // Note: Fog color is ABGR and not ARGB
    Pushbuffer::Push(NV097_SET_FOG_COLOR, kFogColor);
    // Fog gen mode is set to just use the fog coordinate from the shader.
    Pushbuffer::Push(NV097_SET_FOG_GEN_MODE, NV097_SET_FOG_GEN_MODE_V_SPEC_ALPHA);
    Pushbuffer::PushF(NV097_SET_FOG_PLANE, 0.f, 0.f, 2.f, 0.f);
    Pushbuffer::End();
  }

  // Just pass through the diffuse color.
  host_.SetCombinerControl(1);
  // Mix between the output pixel color and the fog color using the fog alpha value.
  host_.SetFinalCombiner0(TestHost::SRC_FOG, true, false, TestHost::SRC_DIFFUSE, false, false, TestHost::SRC_FOG, false,
                          false);
  host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_DIFFUSE,
                          true, false, /*specular_add_invert_r0*/ false, /* specular_add_invert_v1*/ false,
                          /* specular_clamp */ true);
}

void FogCarryoverTests::Test() {
  host_.PrepareDraw(0xFF201F20);

  static constexpr float kTriWidth = 96.f;
  static constexpr float kTriHeight = 42.f;
  static constexpr float kTriVSpacing = 8.f;
  static constexpr float kTriZ = 0.f;

  const float start_x = (host_.GetFramebufferWidthF() - (kTriWidth * 2.f)) / 3.f;
  const float y_inc = kTriHeight + kTriVSpacing;
  float top = 62.f;
  int text_row = 2;
  static constexpr int kTextRowInc = 2;

  for (auto fog_mode : kFogModes) {
    auto draw_tri = [this, fog_mode](float left, float top) {
      const auto right = left + kTriWidth;
      const auto bottom = top + kTriHeight;

      host_.Begin(TestHost::PRIMITIVE_TRIANGLES);

      host_.SetDiffuse(kDiffuse);
      host_.SetSpecular(kDiffuse);

      float fog_coord = 1.f;
      switch (fog_mode) {
        case NV097_SET_FOG_MODE_V_LINEAR:
        case NV097_SET_FOG_MODE_V_LINEAR_ABS:
          fog_coord = 0.6f;
          break;
        case NV097_SET_FOG_MODE_V_EXP:
        case NV097_SET_FOG_MODE_V_EXP_ABS:
          fog_coord = 0.1f;
          break;
        case NV097_SET_FOG_MODE_V_EXP2:
        case NV097_SET_FOG_MODE_V_EXP2_ABS:
          fog_coord = 0.2f;
          break;
        default:
          break;
      }

      host_.SetFogCoord(fog_coord);

      host_.SetVertex(left, top, kTriZ);
      host_.SetVertex(right, top, kTriZ);
      host_.SetVertex(left + (right - left) * 0.5f, bottom, kTriZ);

      host_.End();
    };

    auto shader = std::make_shared<PassthroughVertexShader>();
    host_.SetVertexShaderProgram(shader);
    shader->PrepareDraw();

    {
      Pushbuffer::Begin();
      Pushbuffer::Push(NV097_SET_FOG_MODE, fog_mode);
      // The last parameter never seems to have an effect.
      Pushbuffer::PushF(NV097_SET_FOG_PARAMS, ZeroBias(fog_mode), -1.f, 0.f);
      Pushbuffer::End();
    }

    // Draw with the fog coordinates explicitly set to put the hardware into a consistent state.
    // This is done repeatedly to fix an apparent issue with parallelism, drawing only once can lead to a situation
    // where one or more of the vertices in the "unset" draw case below still have arbitrary values from previous
    // operations and are non-hermetic, leading to test results that are dependent on previous draws.
    for (uint32_t i = 0; i < 2; ++i) {
      draw_tri(start_x, top);
    }

    shader->SetShaderOverride(kShader, sizeof(kShader));
    shader->PrepareDraw();
    shader->Activate();

    draw_tri(start_x * 2.f, top);

    pb_printat(text_row, 4, "%8s", FogModeName(fog_mode).c_str());
    text_row += kTextRowInc;

    top += y_inc;
  }

  pb_printat(0, 0, "%s\n", kTestName);

  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kTestName);
}
