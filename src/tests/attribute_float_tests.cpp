#include "attribute_float_tests.h"

#include <pbkit/pbkit.h>
#include <vector>

#include "../test_host.h"
#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "shaders/pixel_shader_program.h"

// clang format off
static const std::vector<uint32_t> passthrough {
#include "shaders/precalculated_vertex_shader_4c_texcoords.inl"
};

static const std::vector<uint32_t> mulColour {
#include "shaders/mul_col0_by_const0_vertex_shader.inl"
};
// clang format on

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
static constexpr uint32_t posSubNorm= 0x00000001;
static constexpr uint32_t negSubNorm = 0x80000001;

float f(uint32_t v) {
    return *((float *)&v);
}

struct TestConfig {
  const char *fileName; // must be valid for a filename
  const char *description;
  float attribute_value[2];
};

static const TestConfig testConfigs[] {
    { "0_1",  "0 to 1", {  0.f, 1.f }},
    {"-1_1", "-1 to 1", { -1.f, 1.f }},
    {"-8_1", "-8 to 1", { -8.f, 1.f }},
    { "0_8",  "0 to 8", {  0.f, 8.f }},
    {"-NaNq_NaNq", "-NaN to +NaN (quiet)", {f(negNanQ), f(posNanQ)}},
    {"-NaNs_NaNs", "-NaN to +NaN (signalling)", {f(negNanS), f(posNanS)}},
    {"-INF_INF", "-INF to +INF", {f(negInf), f(posInf)}},
    {"-Max_Max", "-Max to +Max", {f(negMax), f(posMax)}},
    {"-Min_Min", "-Min to +Min", {f(negSubNorm), f(posSubNorm)}},
};

AttributeFloatTests::AttributeFloatTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Attrib float") {
  for (auto testConfig : testConfigs) {
    tests_[testConfig.description] = [this, testConfig]() { this->Test(testConfig); };
  }
}

void CreateGeometry(float x, float y, float width, float height, float from, float to, std::vector<float> &vb) {

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
    {"1.0", 1.f},
    {"0.0", 0.f},
    {"-INF", f(negInf)},
    {"INF", f(posInf)},
    {"-NaNq", f(negNanQ)},
    {"NaNq", f(posNanQ)}
  };
  std::vector<std::shared_ptr<VertexShaderProgram>> vss;

  float inset = 0.2f;
  float totalWidth = (1.f - 2 * inset);
  float colWidth = totalWidth / mulVals.size();

  for (int i = 0; i < 1 + mulVals.size(); i++) {
    vb.clear();

    float left = inset + i * colWidth;

    // Create geometry
    CreateGeometry(
      left * fb_width,
      (inset + 0.1f) * fb_height,
      (colWidth * 0.8) * fb_width,
      (1 - 2 * inset) * fb_height,
      tConfig.attribute_value[0],
      tConfig.attribute_value[1],
      vb
    );

    // Set VS
    // First use a passthrough shader
    // Then use a shader that multiplies the colour
    auto vs = std::make_shared<PrecalculatedVertexShader>();
    bool mulShader = i != 0;
    float mul;
    const std::vector<uint32_t>* shader;
    if (!mulShader) {
      shader = &passthrough;
    } else {
      shader = &mulColour;
      float mul = mulVals[i - 1].second;
      vs->SetUniformF(0, mul, mul, mul, 1);  // See mul_col0_by_const0_vertex_shader.inl
    }
    vs->SetShaderOverride(shader->data(), shader->size() * sizeof(uint32_t));
    vs->Activate();
    vs->PrepareDraw();

    // Draw geometry
    uint32_t attributes = host_.POSITION | host_.DIFFUSE;
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    for (int v = 0; v < vb.size(); v += 8) {
      host_.SetDiffuse(vb[v + 4], vb[v + 5], vb[v + 6], vb[v + 7]);
      host_.SetVertex(&vb[v]);
    }
    host_.End();

  }

  // Draw descriptive text
  std::string fileName = std::string(tConfig.fileName);
  pb_print("%s\n%s\n0x%08x to 0x%08x\n",
    fileName.c_str(),
    tConfig.description,
    *(uint32_t*)&tConfig.attribute_value[0],
    *(uint32_t*)&tConfig.attribute_value[1]
  );
  pb_print("Multiplier: n/a");
  for (auto &m : mulVals) {
    pb_print(" | %s", m.first);
  }
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, fileName);
}
