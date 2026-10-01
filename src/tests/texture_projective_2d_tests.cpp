#include "texture_projective_2d_tests.h"

#include <pbkit/pbkit.h>

#include <vector>

#include "debug_output.h"
#include "shaders/passthrough_vertex_shader.h"
#include "test_host.h"
#include "texture_format.h"
#include "xbox-swizzle/swizzle.h"

static constexpr char kTestMixedCornersQuad[] = "ExplicitQ_MixedCorners_Quad";
static constexpr char kTestMixedCornersBitri[] = "ExplicitQ_MixedCorners_Bitri";
static constexpr char kTestHorizontalCrossQuad[] = "ExplicitQ_HorizontalCross_Quad";
static constexpr char kTestHorizontalCrossBitri[] = "ExplicitQ_HorizontalCross_Bitri";
static constexpr char kTestVerticalCrossQuad[] = "ExplicitQ_VerticalCross_Quad";
static constexpr char kTestVerticalCrossBitri[] = "ExplicitQ_VerticalCross_Bitri";
static constexpr char kTestOpposingCornersQuad[] = "ExplicitQ_OpposingCorners_Quad";
static constexpr char kTestOpposingCornersBitri[] = "ExplicitQ_OpposingCorners_Bitri";
static constexpr char kTestPositiveGradientQuad[] = "ExplicitQ_PositiveGradient_Quad";
static constexpr char kTestPositiveGradientBitri[] = "ExplicitQ_PositiveGradient_Bitri";
static constexpr char kTestNegativeGradientQuad[] = "ExplicitQ_NegativeGradient_Quad";
static constexpr char kTestNegativeGradientBitri[] = "ExplicitQ_NegativeGradient_Bitri";
static constexpr char kTestZeroCornerQuad[] = "ExplicitQ_ZeroCorner_Quad";
static constexpr char kTestZeroCornerBitri[] = "ExplicitQ_ZeroCorner_Bitri";
static constexpr char kTestUniformSigns[] = "ExplicitQ_UniformSigns";

static constexpr uint32_t kBorderColor = 0xFFFF00FF;

static constexpr TextureProjective2DTests::TestConfig kExplicitQTests[] = {
    {
        kTestMixedCornersQuad,
        "Mixed positive, negative, fraction, and zero Q corners",
        {
            {-1.0f, 0.0f, 0.0f, -1.0f},
            {1.0f, 0.0f, 0.0f, 1.0f},
            {1.0f, 1.0f, 0.0f, -0.25f},
            {-1.0f, 1.0f, 0.0f, 0.0f},
        },
        true,
        "TL(q=-1), TR(q=1), BR(q=-0.25), BL(q=0)",
    },
    {
        kTestMixedCornersBitri,
        "Mixed corner Q values (triangles)",
        {
            {-1.0f, 0.0f, 0.0f, -1.0f},
            {1.0f, 0.0f, 0.0f, 1.0f},
            {1.0f, 1.0f, 0.0f, -0.25f},
            {-1.0f, 1.0f, 0.0f, 0.0f},
        },
        false,
        "TL(q=-1), TR(q=1), BR(q=-0.25), BL(q=0)",
    },
    {
        kTestHorizontalCrossQuad,
        "Horizontal Q gradient crossing zero (+1.0 -> -1.0)",
        {
            {0.0f, 0.0f, 0.0f, 1.0f},
            {-1.0f, 0.0f, 0.0f, -1.0f},
            {-1.0f, -1.0f, 0.0f, -1.0f},
            {0.0f, 1.0f, 0.0f, 1.0f},
        },
        true,
        "Left edge q=+1.0, Right edge q=-1.0 (crossing q=0)",
    },
    {
        kTestHorizontalCrossBitri,
        "Horizontal Q crossing zero (triangles)",
        {
            {0.0f, 0.0f, 0.0f, 1.0f},
            {-1.0f, 0.0f, 0.0f, -1.0f},
            {-1.0f, -1.0f, 0.0f, -1.0f},
            {0.0f, 1.0f, 0.0f, 1.0f},
        },
        false,
        "Left edge q=+1.0, Right edge q=-1.0 (crossing q=0)",
    },
    {
        kTestVerticalCrossQuad,
        "Vertical Q gradient crossing zero (+1.0 -> -1.0)",
        {
            {0.0f, 0.0f, 0.0f, 1.0f},
            {1.0f, 0.0f, 0.0f, 1.0f},
            {-1.0f, -1.0f, 0.0f, -1.0f},
            {0.0f, -1.0f, 0.0f, -1.0f},
        },
        true,
        "Top edge q=+1.0, Bottom edge q=-1.0 (crossing q=0)",
    },
    {
        kTestVerticalCrossBitri,
        "Vertical Q crossing zero (triangles)",
        {
            {0.0f, 0.0f, 0.0f, 1.0f},
            {1.0f, 0.0f, 0.0f, 1.0f},
            {-1.0f, -1.0f, 0.0f, -1.0f},
            {0.0f, -1.0f, 0.0f, -1.0f},
        },
        false,
        "Top edge q=+1.0, Bottom edge q=-1.0 (crossing q=0)",
    },
    {
        kTestOpposingCornersQuad,
        "Alternating signs at corners (TL=+1, TR=-1, BR=+1, BL=-1)",
        {
            {0.0f, 0.0f, 0.0f, 1.0f},
            {-1.0f, 0.0f, 0.0f, -1.0f},
            {1.0f, 1.0f, 0.0f, 1.0f},
            {0.0f, -1.0f, 0.0f, -1.0f},
        },
        true,
        "Saddle Q surface crossing zero along diagonals",
    },
    {
        kTestOpposingCornersBitri,
        "Alternating corner signs (triangles)",
        {
            {0.0f, 0.0f, 0.0f, 1.0f},
            {-1.0f, 0.0f, 0.0f, -1.0f},
            {1.0f, 1.0f, 0.0f, 1.0f},
            {0.0f, -1.0f, 0.0f, -1.0f},
        },
        false,
        "Saddle Q surface crossing zero along diagonals",
    },
    {
        kTestPositiveGradientQuad,
        "Positive Q gradient (+0.25 -> +2.0)",
        {
            {0.0f, 0.0f, 0.0f, 0.25f},
            {2.0f, 0.0f, 0.0f, 2.0f},
            {2.0f, 2.0f, 0.0f, 2.0f},
            {0.0f, 0.25f, 0.0f, 0.25f},
        },
        true,
        "Left edge q=+0.25, Right edge q=+2.0",
    },
    {
        kTestPositiveGradientBitri,
        "Positive Q gradient (triangles)",
        {
            {0.0f, 0.0f, 0.0f, 0.25f},
            {2.0f, 0.0f, 0.0f, 2.0f},
            {2.0f, 2.0f, 0.0f, 2.0f},
            {0.0f, 0.25f, 0.0f, 0.25f},
        },
        false,
        "Left edge q=+0.25, Right edge q=+2.0",
    },
    {
        kTestNegativeGradientQuad,
        "Negative Q gradient (-0.25 -> -2.0)",
        {
            {0.0f, 0.0f, 0.0f, -0.25f},
            {-2.0f, 0.0f, 0.0f, -2.0f},
            {-2.0f, -2.0f, 0.0f, -2.0f},
            {0.0f, -0.25f, 0.0f, -0.25f},
        },
        true,
        "Left edge q=-0.25, Right edge q=-2.0",
    },
    {
        kTestNegativeGradientBitri,
        "Negative Q gradient (triangles)",
        {
            {0.0f, 0.0f, 0.0f, -0.25f},
            {-2.0f, 0.0f, 0.0f, -2.0f},
            {-2.0f, -2.0f, 0.0f, -2.0f},
            {0.0f, -0.25f, 0.0f, -0.25f},
        },
        false,
        "Left edge q=-0.25, Right edge q=-2.0",
    },
    {
        kTestZeroCornerQuad,
        "Single zero corner (BL q=0.0, others q=1.0)",
        {
            {0.0f, 0.0f, 0.0f, 1.0f},
            {1.0f, 0.0f, 0.0f, 1.0f},
            {1.0f, 1.0f, 0.0f, 1.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
        },
        true,
        "BL vertex is (q=0.0)",
    },
    {
        kTestZeroCornerBitri,
        "Single zero corner (triangles)",
        {
            {0.0f, 0.0f, 0.0f, 1.0f},
            {1.0f, 0.0f, 0.0f, 1.0f},
            {1.0f, 1.0f, 0.0f, 1.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
        },
        false,
        "BL vertex is (q=0.0)",
    },
};

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc ExplicitQ_MixedCorners_Quad
 *   Evaluates projective coordinate division on a quad with mixed corner Q values:
 *     - TL: s=-1, t=0, q=-1   -> s/q = 1.0,  t/q = 0.0
 *     - TR: s=1,  t=0, q=1    -> s/q = 1.0,  t/q = 0.0
 *     - BR: s=1,  t=1, q=-0.25 -> s/q = -4.0, t/q = -4.0
 *     - BL: s=-1, t=1, q=0    -> s/q = -inf, t/q = inf
 *   Tests hardware division across negative, positive, fractional, and zero Q values.
 *
 * @tc ExplicitQ_MixedCorners_Bitri
 *   Evaluates the mixed corner Q configuration triangulated as two triangles.
 *
 * @tc ExplicitQ_HorizontalCross_Quad
 *   Evaluates projective coordinate division across a horizontal zero-crossing on a quad:
 *     - Left: q = +1.0, Right: q = -1.0.
 *     - Coordinates: TL(0,0,q=1), TR(-1,0,q=-1), BR(-1,-1,q=-1), BL(0,1,q=1).
 *   Tests continuous interpolation and sign preservation across the q = 0 singularity.
 *
 * @tc ExplicitQ_HorizontalCross_Bitri
 *   Evaluates the horizontal zero-crossing test triangulated as two triangles.
 *
 * @tc ExplicitQ_VerticalCross_Quad
 *   Evaluates projective coordinate division across a vertical zero-crossing on a quad:
 *     - Top: q = +1.0, Bottom: q = -1.0.
 *     - Coordinates: TL(0,0,q=1), TR(1,0,q=1), BR(-1,-1,q=-1), BL(0,-1,q=-1).
 *   Tests vertical interpolation across the q = 0 singularity.
 *
 * @tc ExplicitQ_VerticalCross_Bitri
 *   Evaluates the vertical zero-crossing test triangulated as two triangles.
 *
 * @tc ExplicitQ_OpposingCorners_Quad
 *   Evaluates projective coordinate division on a saddle-shaped Q surface:
 *     - TL(0,0,q=1), TR(-1,0,q=-1), BR(1,1,q=1), BL(0,-1,q=-1).
 *   Tests zero-crossings along both diagonals of the quad.
 *
 * @tc ExplicitQ_OpposingCorners_Bitri
 *   Evaluates the saddle-shaped Q surface triangulated as two triangles.
 *
 * @tc ExplicitQ_PositiveGradient_Quad
 *   Evaluates projective division with all-positive Q gradient (q in [0.25, 2.0]):
 *     - TL(0,0,q=0.25), TR(2,0,q=2.0), BR(2,2,q=2.0), BL(0,0.25,q=0.25).
 *   Tests non-linear perspective warping purely in the positive Q domain.
 *
 * @tc ExplicitQ_PositiveGradient_Bitri
 *   Evaluates the all-positive Q gradient triangulated as two triangles.
 *
 * @tc ExplicitQ_NegativeGradient_Quad
 *   Evaluates projective division with all-negative Q gradient (q in [-0.25, -2.0]):
 *     - TL(0,0,q=-0.25), TR(-2,0,q=-2.0), BR(-2,-2,q=-2.0), BL(0,-0.25,q=-0.25).
 *   Tests projective interpolation purely in the negative Q domain.
 *
 * @tc ExplicitQ_NegativeGradient_Bitri
 *   Evaluates the all-negative Q gradient triangulated as two triangles.
 *
 * @tc ExplicitQ_ZeroCorner_Quad
 *   Evaluates projective division with three corners at q = 1.0 and BL corner at q = 0.0:
 *     - TL(0,0,q=1), TR(1,0,q=1), BR(1,1,q=1), BL(0,1,q=0).
 *   Tests interpolation behavior when a single corner is at the singularity.
 *
 * @tc ExplicitQ_ZeroCorner_Bitri
 *   Evaluates the single zero corner configuration triangulated as two triangles.
 *
 * @tc ExplicitQ_UniformSigns
 *   Evaluates constant Q values across 5 quads per wrap mode:
 *     - q = +1.0, q = -1.0, q = +0.5, q = -0.5, q = 0.0.
 *   Compares algebraic sign preservation (-s/-q == s/q) and zero divisor handling.
 */
TextureProjective2DTests::TextureProjective2DTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Texture projective 2D", config) {
  for (const auto &test_config : kExplicitQTests) {
    tests_[test_config.test_name] = [this, &test_config]() { TestExplicitQ(test_config); };
  }
  tests_[kTestUniformSigns] = [this]() { TestUniformSigns(); };
}

void TextureProjective2DTests::Initialize() {
  TestSuite::Initialize();
  GenerateTestTexture();
}

void TextureProjective2DTests::GenerateTestTexture() {
  std::vector<uint32_t> linear_texture(kTextureSize * kTextureSize);

  static constexpr uint32_t kColorFrame = 0xFFFFFFFF;
  static constexpr uint32_t kColorNotch = 0xFF00FFFF;
  static constexpr uint32_t kColorA = 0xFFFFFF00;
  static constexpr uint32_t kColorB = 0xFF0000FF;

  for (uint32_t y = 0; y < kTextureSize; ++y) {
    for (uint32_t x = 0; x < kTextureSize; ++x) {
      uint32_t color;
      if (x < 2 || x >= kTextureSize - 2 || y < 2 || y >= kTextureSize - 2) {
        color = kColorFrame;
      } else if (x < 16 && y < 16) {
        color = kColorNotch;
      } else {
        bool checker = ((x / 16) + (y / 16)) % 2 == 0;
        color = checker ? kColorA : kColorB;
      }
      linear_texture[y * kTextureSize + x] = color;
    }
  }

  swizzle_rect(reinterpret_cast<const uint8_t *>(linear_texture.data()), kTextureSize, kTextureSize,
               host_.GetTextureMemoryForStage(0), kTextureSize * sizeof(uint32_t), 4);
}

void TextureProjective2DTests::TestExplicitQ(const TestConfig &config) {
  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  host_.PrepareDraw(0xFF181818);

  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
  texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
  texture_stage.SetBorderColor(kBorderColor);

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

  static constexpr float kStripWidth = 480.f;
  static constexpr float kStripHeight = 32.f;
  static constexpr float kStartX = 110.f;

  struct WrapModeConfig {
    TextureStage::WrapMode mode;
    const char *name;
    int text_row;
    float strip_y;
  };
  static constexpr WrapModeConfig kWrapModes[] = {
      {TextureStage::WRAP_BORDER, "BORDER", 3, 92.f},
      {TextureStage::WRAP_CLAMP_TO_EDGE, "CLAMP", 5, 142.f},
      {TextureStage::WRAP_REPEAT, "REPEAT", 7, 192.f},
      {TextureStage::WRAP_MIRROR, "MIRROR", 9, 242.f},
  };

  auto draw_strip = [this, &config](float left, float top, float width, float height) {
    const float right = left + width;
    const float bottom = top + height;

    if (config.draw_quad) {
      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetTexCoord0(config.corners[0].s, config.corners[0].t, config.corners[0].r, config.corners[0].q);
      host_.SetVertex(left, top, 1.f);

      host_.SetTexCoord0(config.corners[1].s, config.corners[1].t, config.corners[1].r, config.corners[1].q);
      host_.SetVertex(right, top, 1.f);

      host_.SetTexCoord0(config.corners[2].s, config.corners[2].t, config.corners[2].r, config.corners[2].q);
      host_.SetVertex(right, bottom, 1.f);

      host_.SetTexCoord0(config.corners[3].s, config.corners[3].t, config.corners[3].r, config.corners[3].q);
      host_.SetVertex(left, bottom, 1.f);
      host_.End();
    } else {
      host_.Begin(TestHost::PRIMITIVE_TRIANGLES);
      host_.SetTexCoord0(config.corners[0].s, config.corners[0].t, config.corners[0].r, config.corners[0].q);
      host_.SetVertex(left, top, 1.f);

      host_.SetTexCoord0(config.corners[1].s, config.corners[1].t, config.corners[1].r, config.corners[1].q);
      host_.SetVertex(right, top, 1.f);

      host_.SetTexCoord0(config.corners[3].s, config.corners[3].t, config.corners[3].r, config.corners[3].q);
      host_.SetVertex(left, bottom, 1.f);

      host_.SetTexCoord0(config.corners[1].s, config.corners[1].t, config.corners[1].r, config.corners[1].q);
      host_.SetVertex(right, top, 1.f);

      host_.SetTexCoord0(config.corners[2].s, config.corners[2].t, config.corners[2].r, config.corners[2].q);
      host_.SetVertex(right, bottom, 1.f);

      host_.SetTexCoord0(config.corners[3].s, config.corners[3].t, config.corners[3].r, config.corners[3].q);
      host_.SetVertex(left, bottom, 1.f);
      host_.End();
    }
  };

  for (const auto &wrap_mode : kWrapModes) {
    texture_stage.SetUWrap(wrap_mode.mode, false);
    texture_stage.SetVWrap(wrap_mode.mode, false);
    host_.SetupTextureStages();

    draw_strip(kStartX, wrap_mode.strip_y, kStripWidth, kStripHeight);
  }

  pb_printat(0, 0, "%s", config.test_name);
  if (config.description) {
    pb_printat(1, 1, "%s", config.description);
  }
  for (const auto &wrap_mode : kWrapModes) {
    pb_printat(wrap_mode.text_row, 1, "%s", wrap_mode.name);
  }
  if (config.notes_line1) {
    pb_printat(11, 1, "%s", config.notes_line1);
  }
  pb_draw_text_screen();

  FinishDraw(config.test_name);

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
}

void TextureProjective2DTests::TestUniformSigns() {
  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  host_.PrepareDraw(0xFF181818);

  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
  texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
  texture_stage.SetBorderColor(kBorderColor);

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

  struct WrapModeConfig {
    TextureStage::WrapMode mode;
    const char *name;
    int text_row;
    float strip_y;
  };
  static constexpr WrapModeConfig kWrapModes[] = {
      {TextureStage::WRAP_BORDER, "BORDER", 3, 92.f},
      {TextureStage::WRAP_CLAMP_TO_EDGE, "CLAMP", 5, 142.f},
      {TextureStage::WRAP_REPEAT, "REPEAT", 7, 192.f},
      {TextureStage::WRAP_MIRROR, "MIRROR", 9, 242.f},
  };

  struct QuadCase {
    float x;
    TexCoord4 corners[4];
  };

  static constexpr float kQuadWidth = 80.f;
  static constexpr float kQuadHeight = 32.f;

  const QuadCase quad_cases[] = {
      {110.f, {{0.f, 0.f, 0.f, 1.f}, {1.f, 0.f, 0.f, 1.f}, {1.f, 1.f, 0.f, 1.f}, {0.f, 1.f, 0.f, 1.f}}},
      {205.f, {{0.f, 0.f, 0.f, -1.f}, {-1.f, 0.f, 0.f, -1.f}, {-1.f, -1.f, 0.f, -1.f}, {0.f, -1.f, 0.f, -1.f}}},
      {300.f, {{0.f, 0.f, 0.f, 0.5f}, {0.5f, 0.f, 0.f, 0.5f}, {0.5f, 0.5f, 0.f, 0.5f}, {0.f, 0.5f, 0.f, 0.5f}}},
      {395.f, {{0.f, 0.f, 0.f, -0.5f}, {-0.5f, 0.f, 0.f, -0.5f}, {-0.5f, -0.5f, 0.f, -0.5f}, {0.f, -0.5f, 0.f, -0.5f}}},
      {490.f, {{0.f, 0.f, 0.f, 0.f}, {1.f, 0.f, 0.f, 0.f}, {1.f, 1.f, 0.f, 0.f}, {0.f, 1.f, 0.f, 0.f}}},
  };

  auto draw_quad = [this](float left, float top, const TexCoord4 c[4]) {
    const float right = left + kQuadWidth;
    const float bottom = top + kQuadHeight;

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(c[0].s, c[0].t, c[0].r, c[0].q);
    host_.SetVertex(left, top, 1.f);

    host_.SetTexCoord0(c[1].s, c[1].t, c[1].r, c[1].q);
    host_.SetVertex(right, top, 1.f);

    host_.SetTexCoord0(c[2].s, c[2].t, c[2].r, c[2].q);
    host_.SetVertex(right, bottom, 1.f);

    host_.SetTexCoord0(c[3].s, c[3].t, c[3].r, c[3].q);
    host_.SetVertex(left, bottom, 1.f);
    host_.End();
  };

  for (const auto &wrap_mode : kWrapModes) {
    texture_stage.SetUWrap(wrap_mode.mode, false);
    texture_stage.SetVWrap(wrap_mode.mode, false);
    host_.SetupTextureStages();

    for (const auto &qc : quad_cases) {
      draw_quad(qc.x, wrap_mode.strip_y, qc.corners);
    }
  }

  pb_printat(0, 0, "%s", kTestUniformSigns);
  pb_printat(1, 1, "Uniform constant Q per quad");
  pb_printat(2, 11, "q=+1.0");
  pb_printat(2, 20, "q=-1.0");
  pb_printat(2, 30, "q=+0.5");
  pb_printat(2, 39, "q=-0.5");
  pb_printat(2, 49, "q=0.0");

  for (const auto &wrap_mode : kWrapModes) {
    pb_printat(wrap_mode.text_row, 1, "%s", wrap_mode.name);
  }
  pb_printat(11, 1, "Cols 0-3 evaluate to in-bounds [0, 1] texture coordinates.");
  pb_printat(12, 1, "Col 4 divides by zero (q=0.0).");
  pb_draw_text_screen();

  FinishDraw(kTestUniformSigns);

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
}
