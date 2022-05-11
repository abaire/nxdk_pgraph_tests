#include "attribute_clamping_tests.h"

#include <pbkit/pbkit.h>
#include <printf/printf.h>

#include <vector>

#include "shaders/precalculated_vertex_shader.h"

// Infinities
static constexpr uint32_t posInf = 0x7F800000;
static constexpr uint32_t negInf = 0xFF800000;
// Quiet NaN on x86
static constexpr uint32_t posNanQ = 0x7FC00000;
static constexpr uint32_t negNanQ = 0xFFC00000;
// Signalling NaN on x86
static constexpr uint32_t posNanS = 0x7F800001;
static constexpr uint32_t negNanS = 0xFF800001;
// Max
static constexpr uint32_t posMax = 0x7F7FFFFF;
static constexpr uint32_t negMax = 0xFF7FFFFF;
// Min subnormal
static constexpr uint32_t posSubNorm = 0x00000001;
static constexpr uint32_t negSubNorm = 0x80000001;

static constexpr const char *kAttributeNames[] = {
    "wght", "norm", "diff", "spec", "fog", "ptsize", "bdiff", "bspec", "tex0", "tex1", "tex2", "tex3",
};

static constexpr AttributeClampingTests::Attribute kTestAttributes[] = {
    AttributeClampingTests::ATTR_DIFFUSE,
    AttributeClampingTests::ATTR_SPECULAR,
    AttributeClampingTests::ATTR_FOG_COORD,
};

static constexpr float kTestValues[] = {
    -2.0f,
    -1.0f,
    1.0f,
    2.0f,
};

static std::string MakeTestName(AttributeClampingTests::Attribute attrib, float value) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%s_%g", kAttributeNames[attrib], value);
  return buf;
}

AttributeClampingTests::AttributeClampingTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Attrib clamping") {
  for (auto attrib : kTestAttributes) {
    for (auto test_value : kTestValues) {
      auto test_name = MakeTestName(attrib, test_value);
      tests_[test_name] = [this, attrib, test_value, test_name]() {
        float values[4];
        for (float &value : values) {
          value = test_value;
        }
        Test(attrib, values, test_name);
      };
    }
  }
}

void AttributeClampingTests::Initialize() {
  TestSuite::Initialize();

  host_.SetCombinerControl(1);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
}

void AttributeClampingTests::Test(Attribute attribute, const float values[4], const std::string &test_name) {
  host_.PrepareDraw(0xFE444546);

  static constexpr float kLeft = -1.75f;
  static constexpr float kTop = 1.75f;
  static constexpr float kRight = 1.75f;
  static constexpr float kBottom = -1.75f;
  static constexpr float kDepth = 0.0f;
  static constexpr float kSpacing = 0.05f;

  static constexpr float kWidth = kRight - kLeft;
  static constexpr float kHeight = kTop - kBottom;

  static constexpr float kQuadsPerRow = 2.0f;
  static constexpr float kQuadsPerColumn = 1.0f;

  static constexpr float kQuadWidth = (kWidth - (kSpacing * kQuadsPerRow)) / kQuadsPerRow;
  static constexpr float kQuadHeight = (kHeight - (kSpacing * kQuadsPerRow)) / kQuadsPerColumn;

  float left = kLeft;
  float top = kTop;
  float right = left + kQuadWidth;
  float bottom = kTop - kQuadHeight;

  TestHost::CombinerInput src = TestHost::ZeroInput();
  switch (attribute) {
    case ATTR_DIFFUSE:
      src = TestHost::ColorInput(TestHost::SRC_DIFFUSE);
      break;
    case ATTR_SPECULAR:
      src = TestHost::ColorInput(TestHost::SRC_SPECULAR);
      break;
    case ATTR_FOG_COORD:
      src = TestHost::ColorInput(TestHost::SRC_FOG);
      break;
  }
  host_.SetInputColorCombiner(0, src, TestHost::OneInput(), TestHost::OneInput(), TestHost::OneInput());

  auto draw = [this, &left, top, &right, bottom, attribute, values]() {
    host_.Begin(TestHost::PRIMITIVE_QUADS);

    switch (attribute) {
      case ATTR_DIFFUSE:
        host_.SetDiffuse(values[0], values[1], values[2], values[3]);
        break;
      case ATTR_SPECULAR:
        host_.SetSpecular(values[0], values[1], values[2], values[3]);
        break;
      case ATTR_FOG_COORD:
        host_.SetFogCoord(values[0]);
        break;
    }

    host_.SetVertex(left, top, kDepth, 1.0f);
    host_.SetVertex(right, top, kDepth, 1.0f);
    host_.SetVertex(right, bottom, kDepth, 1.0f);
    host_.SetVertex(left, bottom, kDepth, 1.0f);
    host_.End();

    left += kQuadWidth + kSpacing;
    right += kQuadWidth + kSpacing;
  };

  host_.SetFinalCombiner0Just(TestHost::SRC_R0);

  // Render a quad with the raw value (which will be capped [0,1] in the color combiner).
  host_.SetOutputColorCombiner(0, TestHost::DST_R0);
  draw();

  // Render a quad with the value shifted up one and divided by 2 (mapping [-1,1] => [0,1].
  host_.SetOutputColorCombiner(0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0, false, false,
                               TestHost::SM_SUM, TestHost::OP_SHIFT_RIGHT_1);
  draw();

  // Draw some known values as comparisons
  host_.SetInputColorCombiner(0, TestHost::ColorInput(TestHost::SRC_DIFFUSE), TestHost::OneInput());
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);

  static constexpr float kComparatorHeight = 0.1f;
  auto draw_comparison = [this](float y, float color) {
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(color, color, color, 1.0f);
    host_.SetVertex(kLeft - 0.25f, y, kDepth - 0.01f, 1.0f);
    host_.SetVertex(kRight + 0.25f, y, kDepth - 0.01f, 1.0f);
    host_.SetVertex(kRight + 0.25f, y - kComparatorHeight, kDepth - 0.01f, 1.0f);
    host_.SetVertex(kLeft - 0.25f, y - kComparatorHeight, kDepth - 0.01f, 1.0f);
    host_.End();
  };

  float y = kTop - (kQuadHeight / 2) - kComparatorHeight;

  draw_comparison(y, 0.0f);
  y += kComparatorHeight + kSpacing;
  draw_comparison(y, 0.5f);
  y += kComparatorHeight + kSpacing;
  draw_comparison(y, 1.0f);

  pb_print("%s\n", test_name.c_str());
  pb_printat(2, 22, (char *)"x");
  pb_printat(2, 32, (char *)"(x+1)/2");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, test_name);
}
