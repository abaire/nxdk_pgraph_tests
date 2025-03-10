#include "texture_perspective_enable_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "shaders/precalculated_vertex_shader.h"
#include "texture_generator.h"

static constexpr auto kWUpperLeft = INFINITY;
static constexpr auto kWUpperRight = 1.f;
static constexpr auto kWLowerRight = 0.980578f;
static constexpr auto kWLowerLeft = 0.f;

static constexpr auto kWUpperLeftTwo = INFINITY;
static constexpr auto kWUpperRightTwo = 1.f;
static constexpr auto kWLowerRightTwo = 2.980578f;
static constexpr auto kWLowerLeftTwo = -0.5f;

static std::string MakeTestName(bool texture_enabled) {
  std::string ret = "TexPerspective_";
  if (texture_enabled) {
    ret += "Textured";
  } else {
    ret += "Untextured";
  }
  return ret;
}

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc TexPerspective_Untextured
 *   Renders two pairs of quads, each of which have interesting W parameters.
 *   The quads are not textured.
 *   The left quads have bit 20 unset. The right have it set.
 *
 * @tc TexPerspective_Textured
 *   Renders two pairs of quads, each of which have interesting W parameters.
 *   The quads are textured with a checkerboard pattern.
 *   The left quads have bit 20 unset. The right have it set.
 */
TexturePerspectiveEnableTests::TexturePerspectiveEnableTests(TestHost& host, std::string output_dir,
                                                             const Config& config)
    : TestSuite(host, std::move(output_dir), "Texture perspective enable", config) {
  for (auto texture_enabled : {false, true}) {
    auto name = MakeTestName(texture_enabled);
    tests_[name] = [this, name, texture_enabled]() { this->Test(name, texture_enabled); };
  }
}

void TexturePerspectiveEnableTests::Initialize() {
  TestSuite::Initialize();

  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);
}

void TexturePerspectiveEnableTests::Test(const std::string& name, bool texture_enabled) {
  host_.PrepareDraw(0xFE323232);

  const auto fb_width = host_.GetFramebufferWidthF();
  const auto fb_height = host_.GetFramebufferHeightF();

  const auto quad_width = fb_width / 5.f;
  const auto quad_height = fb_height / 5.f;
  const auto quad_top = quad_height;

  auto draw_bitri = [this, quad_width, quad_height](float left, float top, float wUL, float wUR, float wLR, float wLL) {
    const auto right = left + quad_width;
    const auto bottom = top + quad_height;
    const auto quad_z = 1.f;

    host_.Begin(TestHost::PRIMITIVE_TRIANGLES);

    host_.SetDiffuse(1.f, 0.f, 0.f, 1.f);
    host_.SetTexCoord0(0.f, 0.f);
    host_.SetVertex(left, top, quad_z, wUL);

    host_.SetDiffuse(0.f, 0.f, 1.f, 1.f);
    host_.SetTexCoord0(1.f, 1.f);
    host_.SetVertex(right, bottom, quad_z, wLR);

    host_.SetDiffuse(0.f, 1.f, 0.f, 1.f);
    host_.SetTexCoord0(0.f, 1.f);
    host_.SetVertex(left, bottom, quad_z, wLL);

    host_.SetDiffuse(1.f, 0.f, 0.f, 1.f);
    host_.SetTexCoord0(0.f, 0.f);
    host_.SetVertex(left, top, quad_z, wUL);

    host_.SetDiffuse(0.5f, 0.5f, 0.5f, 1.f);
    host_.SetTexCoord0(1.f, 0.f);
    host_.SetVertex(right, top, quad_z, wUR);

    host_.SetDiffuse(0.f, 0.f, 1.f, 1.f);
    host_.SetTexCoord0(1.f, 1.f);
    host_.SetVertex(right, bottom, quad_z, wLR);

    host_.End();
  };

  if (texture_enabled) {
    constexpr auto kStage = 0;
    constexpr uint32_t kTextureSize = 256;
    GenerateSwizzledRGBACheckerboard(host_.GetTextureMemoryForStage(kStage), 0, 0, kTextureSize, kTextureSize,
                                     kTextureSize * 4);

    host_.SetTextureStageEnabled(kStage, true);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
    host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
    auto& texture_stage = host_.GetTextureStage(kStage);
    texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
    host_.SetupTextureStages();

    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);
  }

  // Disable texture perspective.
  host_.SetupControl0(false, false, false);

  draw_bitri(quad_width, quad_top, kWUpperLeft, kWUpperRight, kWLowerRight, kWLowerLeft);
  draw_bitri(quad_width, quad_top * 3.f, kWUpperLeftTwo, kWUpperRightTwo, kWLowerRightTwo, kWLowerLeftTwo);

  // Enable texture perspective.
  host_.SetupControl0(false, false, true);

  draw_bitri(quad_width * 3.f, quad_top, kWUpperLeft, kWUpperRight, kWLowerRight, kWLowerLeft);
  draw_bitri(quad_width * 3.f, quad_top * 3.f, kWUpperLeftTwo, kWUpperRightTwo, kWLowerRightTwo, kWLowerLeftTwo);

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

  pb_print("%s\n", name.c_str());
  pb_printat(2, 12, "Disabled");
  pb_printat(2, 40, "Enabled");

  char buf[64];
  snprintf(buf, sizeof(buf), "%f", kWUpperLeft);
  pb_printat(3, 7, "%s", buf);

  snprintf(buf, sizeof(buf), "%f", kWUpperRight);
  pb_printat(3, 25, "%s", buf);

  snprintf(buf, sizeof(buf), "%f", kWLowerLeft);
  pb_printat(6, 3, "%s", buf);

  snprintf(buf, sizeof(buf), "%f", kWLowerRight);
  pb_printat(6, 25, "%s", buf);

  snprintf(buf, sizeof(buf), "%f", kWUpperLeftTwo);
  pb_printat(10, 7, "%s", buf);

  snprintf(buf, sizeof(buf), "%f", kWUpperRightTwo);
  pb_printat(10, 25, "%s", buf);

  snprintf(buf, sizeof(buf), "%f", kWLowerLeftTwo);
  pb_printat(14, 2, "%s", buf);

  snprintf(buf, sizeof(buf), "%f", kWLowerRightTwo);
  pb_printat(14, 25, "%s", buf);

  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}
