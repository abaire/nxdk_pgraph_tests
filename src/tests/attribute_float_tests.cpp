#include "attribute_float_tests.h"

#include <pbkit/pbkit.h>

#include <vector>

#include "pbkit_ext.h"
#include "shaders/passthrough_vertex_shader.h"

// clang-format off
static constexpr uint32_t kPassthroughMulColorsShader[] = {
#include "passthrough_mul_color_by_constant.vshinc"

};
// clang-format on

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

static float f(uint32_t v) { return *((float *)&v); }

static constexpr char kColorTestName[] = "Colors";

struct TestConfig {
  const char *fileName;  // must be valid for a filename
  const char *description;
  float attribute_value[2];
};

static const TestConfig testConfigs[]{
    {"0_1", "0 to 1", {0.f, 1.f}},
    {"-1_1", "-1 to 1", {-1.f, 1.f}},
    {"-8_1", "-8 to 1", {-8.f, 1.f}},
    {"0_8", "0 to 8", {0.f, 8.f}},
    {"-NaNq_NaNq", "-NaN to +NaN (quiet)", {f(negNanQ), f(posNanQ)}},
    // TODO: It appears that the handling of the signaling NaN is nondeterministic.
    // Sometimes it is converted to quiet NaN. As pgraph operates on integers and does a conversion from float back to
    // int, it may be better to fall back into setting raw values without doing the float conversions.
    {"-NaNs_NaNs", "-NaN to +NaN (signalling)", {f(negNanS), f(posNanS)}},
    {"-INF_INF", "-INF to +INF", {f(negInf), f(posInf)}},
    {"-Max_Max", "-Max (normal) to +Max (normal)", {f(negMax), f(posMax)}},
    {"-MinN_MinN", "-Min (normal) to +Min (normal)", {f(negMinNorm), f(posMinNorm)}},
    {"-MaxSN_MaxSN", "-Max (subnormal) to +Max (subnormal)", {f(negMaxSubNorm), f(posMaxSubNorm)}},
    {"-Min_Min", "-Min (subnormal) to +Min (subnormal)", {f(negMinSubNorm), f(posMinSubNorm)}},
};

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc 0_1
 *  Ranges values from 0 to 1.
 *
 * @tc 0_8
 *  Ranges values from 0 to 8.
 *
 * @tc -1_1
 *  Ranges values from -1 to 1.
 *
 * @tc -8_1
 *  Ranges values from -8 to 1.
 *
 * @tc -INF_INF
 *  Ranges values from -infinity to infinity.
 *
 * @tc -Max_Max
 *  Ranges values from -Max (normal) to +Max (normal).
 *
 * @tc -MaxSN_MaxSN
 *  Ranges values from -Max (subnormal) to +Max (subnormal).
 *
 * @tc -Min_Min
 *  Ranges values from -Min (subnormal) to +Min (subnormal).
 *
 * @tc -MinN_MinN
 *  Ranges values from -Min (normal) to +Min (normal).
 *
 * @tc -NaNq_NaNq
 *  Ranges values from -NaN to +NaN (quiet).
 *
 * @tc -NaNs_NaNs
 *  Ranges values from -NaN to +NaN (signaling).
 *
 * @tc colors
 *   Tests behavior of the color channels when given -NaN and NaN.
 */
AttributeFloatTests::AttributeFloatTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Attrib float", config) {
  for (auto testConfig : testConfigs) {
    tests_[testConfig.description] = [this, testConfig]() { Test(testConfig); };
  }

  tests_[kColorTestName] = [this]() { TestColors(kColorTestName); };
}

static void CreateGeometry(float x, float y, float width, float height, float from, float to, std::vector<float> &vb) {
  float right = x + width;
  float bottom = y + height;

  // Define a quad with a colour gradient
  vb.insert(vb.end(), {x, y, 1, 1});
  vb.insert(vb.end(), {from, from, from, 1});
  vb.insert(vb.end(), {right, y, 1, 1});
  vb.insert(vb.end(), {from, from, from, 1});
  vb.insert(vb.end(), {right, bottom, 1, 1});
  vb.insert(vb.end(), {to, to, to, 1});
  vb.insert(vb.end(), {x, bottom, 1, 1});
  vb.insert(vb.end(), {to, to, to, 1});
}

void AttributeFloatTests::Test(const TestConfig &tConfig) {
  std::vector<float> vb;

  auto fb_width = static_cast<float>(host_.GetFramebufferWidth());
  auto fb_height = static_cast<float>(host_.GetFramebufferHeight());

  static constexpr uint32_t kBackgroundColor = 0xFF6495ED;
  host_.PrepareDraw(kBackgroundColor);

  // Multipliers for mul shader...
  const std::vector<std::pair<const char *, float>> mulVals = {
      {"1.0", 1.f}, {"0.0", 0.f}, {"-INF", f(negInf)}, {"INF", f(posInf)}, {"-NaNq", f(negNanQ)}, {"NaNq", f(posNanQ)}};
  std::vector<std::shared_ptr<VertexShaderProgram>> vss;

  float inset = 0.2f;
  float totalWidth = (1.f - 2 * inset);
  float colWidth = totalWidth / static_cast<float>(mulVals.size());

  for (int i = 0; i < 1 + mulVals.size(); ++i) {
    vb.clear();

    float left = inset + static_cast<float>(i) * colWidth;

    // Create geometry
    CreateGeometry(left * fb_width, (inset + 0.1f) * fb_height, (colWidth * 0.8f) * fb_width,
                   (1 - 2 * inset) * fb_height, tConfig.attribute_value[0], tConfig.attribute_value[1], vb);

    // Set VS
    // First use a passthrough shader
    // Then use a shader that multiplies the colour
    auto vs = std::make_shared<PassthroughVertexShader>();
    bool mulShader = i != 0;
    if (mulShader) {
      vs->SetShader(kPassthroughMulColorsShader, sizeof(kPassthroughMulColorsShader));
      float mul = mulVals[i - 1].second;
      vs->SetUniformF(120 - PassthroughVertexShader::kShaderUserConstantOffset, mul, mul, mul, 1);
      vs->SetUniformF(121 - PassthroughVertexShader::kShaderUserConstantOffset, mul, mul, mul, 1);
      vs->SetUniformF(122 - PassthroughVertexShader::kShaderUserConstantOffset, mul, mul, mul, 1);
      vs->SetUniformF(123 - PassthroughVertexShader::kShaderUserConstantOffset, mul, mul, mul, 1);
    }
    vs->Activate();
    vs->PrepareDraw();

    // Draw geometry
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    for (int v = 0; v < vb.size(); v += 8) {
      host_.SetDiffuse(vb[v + 4], vb[v + 5], vb[v + 6], vb[v + 7]);
      host_.SetVertex(&vb[v]);
    }
    host_.End();
  }

  // Draw descriptive text
  pb_print("%s\n%s\n0x%08x to 0x%08x\n", tConfig.fileName, tConfig.description,
           *(uint32_t *)&tConfig.attribute_value[0], *(uint32_t *)&tConfig.attribute_value[1]);
  pb_print("Multiplier: n/a");
  for (auto &m : mulVals) {
    pb_print(" | %s", m.first);
  }
  pb_draw_text_screen();

  FinishDraw(tConfig.fileName);
}

void AttributeFloatTests::TestColors(const std::string &test_name) {
  std::vector<float> vb;

  static constexpr auto kQuadSize = 128;

  static constexpr auto kTop = 64.f;
  static constexpr auto kQuadPaddingV = 16.f;

  static constexpr auto kNumQuads = 2;
  const auto kQuadPaddingH = floor((host_.GetFramebufferWidthF() - (kQuadSize * kNumQuads)) / (kNumQuads + 1.f));

  static constexpr uint32_t kBackgroundColor = 0xFF333333;
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  host_.PrepareDraw(kBackgroundColor);

  host_.DrawCheckerboardUnproject(0xFF333333, 0xFF444444);

  auto vs = std::make_shared<PassthroughVertexShader>();
  vs->SetShader(kPassthroughMulColorsShader, sizeof(kPassthroughMulColorsShader));
  host_.SetVertexShaderProgram(vs);

  static constexpr vector_t kDiffuse{1.f, 0.f, 0.5f, 1.f};
  static constexpr vector_t kSpecular{0.f, 1.f, 0.5f, 1.f};
  static constexpr vector_t kBackDiffuse{1.f, 0.5f, 1.f, 1.f};
  static constexpr vector_t kBackSpecular{0.5f, 1.f, 1.f, 1.f};

  host_.SetDiffuse(kDiffuse);
  host_.SetSpecular(kSpecular);
  host_.SetBackDiffuse(kBackDiffuse);
  host_.SetBackSpecular(kBackSpecular);

  auto draw_quad = [this](float left, float top) {
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetVertex(left, top, 0.1f, 1.0f);
    host_.SetVertex(left + kQuadSize, top, 0.1f, 1.0f);
    host_.SetVertex(left + kQuadSize, top + kQuadSize * 0.5f, 0.1f, 1.0f);
    host_.SetVertex(left, top + kQuadSize * 0.5f, 0.1f, 1.0f);
    host_.End();
  };

  // Renders a row of quads using various color channels. Each quad is split into a top and bottom half, with the top
  // using alpha from the color channel and the bottom forced opaque.
  auto draw_row = [this, &draw_quad, &kQuadPaddingH](float top) {
    auto draw_half_row = [this, &draw_quad, &kQuadPaddingH](float top, bool use_alpha) {
      if (!use_alpha) {
        host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
      }

      float left = kQuadPaddingH;

      host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
      if (use_alpha) {
        host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
      }
      draw_quad(left, top);

      left += kQuadSize + kQuadPaddingH;
      host_.SetFinalCombiner0Just(TestHost::SRC_SPECULAR);
      if (use_alpha) {
        host_.SetFinalCombiner1Just(TestHost::SRC_SPECULAR, true);
      }
      draw_quad(left, top);
    };

    draw_half_row(top, false);
    draw_half_row(top + kQuadSize * 0.5f, true);
  };

  auto mul = f(posNanQ);
  vs->SetUniformF(120 - PassthroughVertexShader::kShaderUserConstantOffset, mul, mul, mul, mul);
  vs->SetUniformF(121 - PassthroughVertexShader::kShaderUserConstantOffset, mul, mul, mul, mul);
  vs->SetUniformF(122 - PassthroughVertexShader::kShaderUserConstantOffset, mul, mul, mul, mul);
  vs->SetUniformF(123 - PassthroughVertexShader::kShaderUserConstantOffset, mul, mul, mul, mul);
  vs->Activate();
  vs->PrepareDraw();

  draw_row(kTop);
  pb_printat(4, 0, "NaN");

  mul = f(negNanQ);
  vs->SetUniformF(120 - PassthroughVertexShader::kShaderUserConstantOffset, mul, mul, mul, mul);
  vs->SetUniformF(121 - PassthroughVertexShader::kShaderUserConstantOffset, mul, mul, mul, mul);
  vs->SetUniformF(122 - PassthroughVertexShader::kShaderUserConstantOffset, mul, mul, mul, mul);
  vs->SetUniformF(123 - PassthroughVertexShader::kShaderUserConstantOffset, mul, mul, mul, mul);
  vs->Activate();
  vs->PrepareDraw();

  draw_row(kTop + kQuadSize + kQuadPaddingV);
  pb_printat(9, 0, "-NaN");

  host_.SetVertexShaderProgram(nullptr);

  pb_printat(0, 0, "%s\n", test_name.c_str());
  pb_draw_text_screen();

  FinishDraw(test_name);
}
