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

static const std::vector<uint32_t> mulByOne {
#include "shaders/mul_by_one_vertex_shader.inl"
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

struct ShaderConfig {
  const char *shaderName;
  const std::vector<uint32_t> vertexShader;
};

static const TestConfig testConfigs[] {
    { "0_1",  "0 to 1", {  0.f, 1.f }},
    {"-1_1", "-1 to 1", { -1.f, 1.f }},
    {"-8_1", "-8 to 1", { -8.f, 1.f }},
    { "0_8",  "0 to 8", { -1.f, 8.f }},
    {"-NaNq_NaNq", "-NaN to +NaN (quiet)", {f(negNanQ), f(posNanQ)}},
    {"-NaNs_NaNs", "-NaN to +NaN (signalling)", {f(negNanS), f(posNanS)}},
    {"-INF_INF", "-INF to +INF", {f(negInf), f(posInf)}},
    {"-Max_Max", "-Max to +Max", {f(negMax), f(posMax)}},
    {"-Min_Min", "-Min to +Min", {f(negSubNorm), f(posSubNorm)}},
};

static const ShaderConfig shaderConfigs[]{
    {"passthrough", passthrough},
    {"mul by one", mulByOne}, // This affects NaN handling
};

AttributeFloatTests::AttributeFloatTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Attrib float") {
  for (auto shaderConfig : shaderConfigs) {
    for (auto testConfig : testConfigs) {
      std::string testName = std::string(testConfig.description) + " (" + std::string(shaderConfig.shaderName) + ")";
      tests_[testName] = [this, testConfig, shaderConfig, testName]() { this->Test(testName, testConfig, shaderConfig); };
    }
  }
}

void AttributeFloatTests::Initialize() {
  TestSuite::Initialize();
}

void AttributeFloatTests::Deinitialize() {
  TestSuite::Deinitialize();
}

void CreateGeometry(float x, float y, float width, float height, float from, float to, std::vector<float> &vb) {
  vb.clear();

  float right = x + width;
  float bottom = y + height;

  // Draw a quad
  vb.clear();
  vb.insert(vb.end(), {x, y, 1, 1});
  vb.insert(vb.end(), {from, from, from, 1});
  vb.insert(vb.end(), {right, y, 1, 1});
  vb.insert(vb.end(), {to, to, to, 1});
  vb.insert(vb.end(), {right, bottom, 1, 1});
  vb.insert(vb.end(), {to, to, to, 1});
  vb.insert(vb.end(), {x, bottom, 1, 1});
  vb.insert(vb.end(), {from, from, from, 1});
}

void AttributeFloatTests::Test(std::string testName, const TestConfig &tConfig, const ShaderConfig &sConfig) {
  std::vector<float> vb;

  auto fb_width = static_cast<float>(host_.GetFramebufferWidth());
  auto fb_height = static_cast<float>(host_.GetFramebufferHeight());

  float inset = 0.2f;

  // Set VS
  auto vs = std::make_shared<PrecalculatedVertexShader>();
  vs->SetShaderOverride(sConfig.vertexShader.data(), sConfig.vertexShader.size() * sizeof(uint32_t));
  vs->SetUniformF(0, 1, 1, 1, 1);  // See mul_by_one_vertex_shader.inl
  host_.SetVertexShaderProgram(vs);

  static constexpr uint32_t kBackgroundColor = 0xFF6495ED;
  host_.PrepareDraw(kBackgroundColor);

  // Create geometry
  CreateGeometry(
      fb_width * inset,
      fb_height * inset,
      fb_width - 2 * inset * fb_width,
      fb_height - 2 * inset * fb_height,
      tConfig.attribute_value[0],
      tConfig.attribute_value[1],
      vb
  );

  // Draw geometry
  uint32_t attributes = host_.POSITION | host_.DIFFUSE;
  host_.Begin(TestHost::PRIMITIVE_QUADS);
  for (int i = 0; i < vb.size(); i += 8) {
    host_.SetDiffuse(vb[i + 4], vb[i + 5], vb[i + 6], vb[i + 7]);
    host_.SetVertex(&vb[i]);
  }
  host_.End();

  // Draw descriptive text
  std::string fileName = std::string(tConfig.fileName) + "_" + std::string(sConfig.shaderName);
  pb_print("%s\n%s\n0x%08x to 0x%08x",
      fileName.c_str(),
      testName.c_str(),
      *(uint32_t*)&tConfig.attribute_value[0],
      *(uint32_t*)&tConfig.attribute_value[1]
  );
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, fileName);
}
