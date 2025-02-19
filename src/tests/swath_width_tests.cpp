#include "swath_width_tests.h"

#include <texture_generator.h>

#include <memory>

#include "shaders/precalculated_vertex_shader.h"

// From pbkit.c, DMA_A is set to channel 3 by default
// NV097_SET_CONTEXT_DMA_A == NV20_TCL_PRIMITIVE_3D_SET_OBJECT1
static constexpr uint32_t kDefaultDMAChannelA = 3;
// From pbkit.c, DMA_COLOR is set to channel 9 by default.
// NV097_SET_CONTEXT_DMA_COLOR == NV20_TCL_PRIMITIVE_3D_SET_OBJECT3
static constexpr uint32_t kDefaultDMAColorChannel = 9;
// NV097_SET_CONTEXT_DMA_ZETA is set to channel 10 by default.
static constexpr uint32_t kDefaultDMAZetaChannel = 10;

static constexpr uint32_t kCheckerboardA = 0x33FFFFFF;
static constexpr uint32_t kCheckerboardB = 0xD0000000;
static constexpr uint32_t kCheckerboardSize = 16;

static constexpr uint32_t kTextureSize = 256;
static constexpr uint32_t kTexturePitch = kTextureSize * 4;

struct TestConfig {
  const char *name;
  uint32_t swath_width;
};

static const TestConfig testConfigs[]{
    {"SwathWidth00", NV097_SET_SWATH_WIDTH_V_00}, {"SwathWidth01", NV097_SET_SWATH_WIDTH_V_01},
    {"SwathWidth02", NV097_SET_SWATH_WIDTH_V_02}, {"SwathWidth03", NV097_SET_SWATH_WIDTH_V_03},
    {"SwathWidth04", NV097_SET_SWATH_WIDTH_V_04}, {"SwathWidth0F", NV097_SET_SWATH_WIDTH_V_OFF},
    // {"SwathWidth05", 0x05},   // Crashes with invalid data error
    // {"SwathWidth10", 0x10},   // Crashes with invalid data error
    // {"SwathWidthFF", 0xFF},   // Crashes with invalid data error
};

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc SwathWidth00
 *  Sets NV097_SET_SWATH_WIDTH to 0x00.
 *
 * @tc SwathWidth01
 *  Sets NV097_SET_SWATH_WIDTH to 0x01.
 *
 * @tc SwathWidth02
 *  Sets NV097_SET_SWATH_WIDTH to 0x02.
 *
 * @tc SwathWidth03
 *  Sets NV097_SET_SWATH_WIDTH to 0x03.
 *
 * @tc SwathWidth04
 *  Sets NV097_SET_SWATH_WIDTH to 0x04.
 *
 * @tc SwathWidth0F
 *  Sets NV097_SET_SWATH_WIDTH to 0x0F (correlated with turning antialiasing off).
 */
SwathWidthTests::SwathWidthTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Swath width", config) {
  for (auto testConfig : testConfigs) {
    tests_[testConfig.name] = [this, testConfig]() { this->Test(testConfig.name, testConfig.swath_width); };
  }
}

void SwathWidthTests::Initialize() {
  TestSuite::Initialize();

  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);

  host_.SetBlend(true);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, false);
  p = pb_push1(p, NV097_SET_DEPTH_MASK, false);
  p = pb_push1(p, NV097_SET_STENCIL_TEST_ENABLE, false);
  p = pb_push1(p, NV097_SET_STENCIL_MASK, false);
  pb_end(p);
}

static void RenderGeometry(TestHost &host) {
  host.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

  const auto kTop = 65.f;

  auto top = kTop;
  host.Begin(TestHost::PRIMITIVE_LINE_LOOP);
  host.SetDiffuse(0xCF0000FF);
  host.SetVertex(10.0f, top, 1.0f);
  host.SetDiffuse(0xFF00FFFF);
  host.SetVertex(100.0f, top - 10.0f, 1.0f);
  host.SetDiffuse(0xFFFFFF00);
  host.SetVertex(300.0f, top, 1.0f);
  host.SetDiffuse(0xFFFFFFFF);
  host.SetVertex(600.0f, top + 10.0f, 1.0f);
  host.SetDiffuse(0x6FFFFFFF);
  host.SetVertex(200.0f, top + 30.0f, 1.0f);
  host.End();

  top += 50.f;
  host.Begin(TestHost::PRIMITIVE_TRIANGLE_STRIP);
  host.SetDiffuse(0xCF0000FF);
  host.SetVertex(20.f, top, 2.f);
  host.SetDiffuse(0xFF00FFFF);
  host.SetVertex(120.f, top - 10.f, 2.f);
  host.SetDiffuse(0xFFFFFF00);
  host.SetVertex(80.f, top + 10.f, 2.f);
  host.SetDiffuse(0xFFFFFFFF);
  host.SetVertex(280.f, top, 2.f);
  host.End();

  top += 50.f;
  host.Begin(TestHost::PRIMITIVE_POINTS);
  host.SetDiffuse(0xCF0000FF);
  host.SetVertex(20.f, top, 2.f);
  host.SetDiffuse(0xFF00FFFF);
  host.SetVertex(120.f, top - 10.f, 2.f);
  host.SetDiffuse(0xFFFFFF00);
  host.SetVertex(80.f, top + 10.f, 2.f);
  host.SetDiffuse(0xFFFFFFFF);
  host.SetVertex(280.f, top, 2.f);
  host.End();

  top += 50.f;
  host.Begin(TestHost::PRIMITIVE_POLYGON);
  host.SetDiffuse(0xCF0000FF);
  host.SetVertex(20.f, top, 2.f);
  host.SetDiffuse(0xFF00FFFF);
  host.SetVertex(120.f, top - 10.f, 2.f);
  host.SetDiffuse(0xFFFFFFFF);
  host.SetVertex(280.f, top + 12.f, 2.f);
  host.SetDiffuse(0xFFFF00FF);
  host.SetVertex(580.f, top + 45.f, 2.f);
  host.SetDiffuse(0xFFFFFF00);
  host.SetVertex(80.f, top + 30.f, 2.f);
  host.End();
}

static void RenderTexturedQuad(TestHost &host, float left, float top, float right, float bottom) {
  constexpr uint32_t kStage = 3;
  constexpr auto kCombinerSource = TestHost::SRC_TEX3;
  GenerateSwizzledRGBACheckerboard(host.GetTextureMemoryForStage(kStage), 0, 0, kTextureSize, kTextureSize,
                                   kTexturePitch * 4, kCheckerboardA, kCheckerboardB, kCheckerboardSize);

  host.SetFinalCombiner0Just(kCombinerSource);
  host.SetFinalCombiner1Just(kCombinerSource, true);

  host.SetTextureStageEnabled(kStage, true);
  // Keep in sync with kStage.
  host.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_NONE, TestHost::STAGE_NONE,
                             TestHost::STAGE_2D_PROJECTIVE);

  auto &texture_stage = host.GetTextureStage(kStage);
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
  texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
  host.SetupTextureStages();

  const auto half_width = (right - left) * 0.5f;
  const auto half_height = (bottom - top) * 0.5f;

  host.SetBlend(true);
  host.Begin(TestHost::PRIMITIVE_QUADS);
  // Keep tex coord in sync with kCombinerSource.
  host.SetTexCoord3(0.0f, 0.0f);
  host.SetVertex(left + half_width, top, 0.1f, 1.0f);
  host.SetTexCoord3(1.0f, 0.0f);
  host.SetVertex(right, top + half_height, 0.1f, 1.0f);
  host.SetTexCoord3(1.0f, 1.0f);
  host.SetVertex(left + half_width, bottom, 0.1f, 1.0f);
  host.SetTexCoord3(0.0f, 1.0f);
  host.SetVertex(left, top + half_height, 0.1f, 1.0f);
  host.End();

  host.SetTextureStageEnabled(kStage, false);
  host.SetShaderStageProgram(TestHost::STAGE_NONE);
  host.SetupTextureStages();
}

/**
 * Redirect output to TEX0 with antialiasing enabled and initialize the texture with a translucent checkerboard panel.
 */
static void RenderToAntialiasedTextureBegin(TestHost &host) {
  const auto kFramebufferWidth = host.GetFramebufferWidth();
  const auto kFramebufferHeight = host.GetFramebufferHeight();
  const uint32_t kAAFramebufferPitch = kFramebufferWidth * 4 * 2;
  const auto kTextureMemory = reinterpret_cast<uint32_t>(host.GetTextureMemoryForStage(0));

  GenerateRGBACheckerboard(host.GetTextureMemoryForStage(0), 0, 0, kFramebufferWidth * 2, kFramebufferHeight,
                           kAAFramebufferPitch, 0x3322AAAA, 0x33AA22AA, 64);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_SURFACE_PITCH,
               SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kAAFramebufferPitch) |
                   SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kAAFramebufferPitch));
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
  p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(kTextureMemory));

  pb_end(p);

  host.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, kFramebufferWidth,
                                 host.GetFramebufferHeight(), false, 0, 0, 0, 0, TestHost::AA_CENTER_CORNER_2);
}

/**
 * Redirect output back to the framebuffer with antialiasing disabled, then render a quad textured with TEX0.
 */
static void RenderToAntialiasedTextureEnd(TestHost &host) {
  const auto kFramebufferWidth = host.GetFramebufferWidth();
  const auto kFramebufferHeight = host.GetFramebufferHeight();
  const uint32_t kFramebufferPitch = kFramebufferWidth * 4;
  const uint32_t kAAFramebufferWidth = kFramebufferWidth << 1;

  // Direct output to the framebuffer.
  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, 0);
    pb_end(p);
    host.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, kFramebufferWidth, kFramebufferHeight);
  }

  // Render a textured quad using the previous render surface.
  {
    auto &texture_stage = host.GetTextureStage(0);
    texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8));
    texture_stage.SetTextureDimensions(1, 1, 1);
    texture_stage.SetImageDimensions(kAAFramebufferWidth, kFramebufferHeight);
    texture_stage.SetFilter(0, TextureStage::K_QUINCUNX, TextureStage::MIN_TENT_LOD0, TextureStage::MAG_TENT_LOD0);
    host.SetTextureStageEnabled(0, true);
    host.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
    host.SetupTextureStages();

    host.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

    host.Begin(TestHost::PRIMITIVE_QUADS);

    host.SetTexCoord0(0.f, 0.f);
    host.SetVertex(0.f, 0.f, 0.f);

    host.SetTexCoord0(static_cast<float>(kAAFramebufferWidth), 0.f);
    host.SetVertex(640.f, 0.f, 0.f);

    host.SetTexCoord0(static_cast<float>(kAAFramebufferWidth), static_cast<float>(kFramebufferHeight));
    host.SetVertex(640.f, 480.f, 0.f);

    host.SetTexCoord0(0.f, static_cast<float>(kFramebufferHeight));
    host.SetVertex(0.f, 480.f, 0.f);

    host.End();

    host.SetTextureStageEnabled(0, false);
    host.SetShaderStageProgram(TestHost::STAGE_NONE);
  }
}

/**
 * Sets the NV097_SET_SWATH_WIDTH value, renders some geometry into TEX0 with antialiasing enabled, then renders TEX0 to
 * the framebuffer.
 */
void SwathWidthTests::Test(const std::string &name, uint32_t swath_width) {
  host_.PrepareDraw(0xFF222322);

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SWATH_WIDTH, swath_width);
    p = pb_push1(p, NV097_SET_SMOOTHING_CONTROL, 0xFFFF0001);
    p = pb_push1(p, NV097_SET_LINE_SMOOTH_ENABLE, true);
    p = pb_push1(p, NV097_SET_POLY_SMOOTH_ENABLE, true);
    pb_end(p);
  }

  RenderToAntialiasedTextureBegin(host_);

  RenderGeometry(host_);

  constexpr auto kQuadWidth = static_cast<float>(kTextureSize) * 1.4f;
  constexpr auto kQuadHeight = kQuadWidth;

  const float kLeft = (host_.GetFramebufferWidthF() - kQuadWidth) * 0.5f;
  const float kTop = (host_.GetFramebufferHeightF() - kQuadHeight) * 0.5f;

  RenderTexturedQuad(host_, kLeft, kTop, kLeft + kQuadWidth, kTop + kQuadHeight);

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SMOOTHING_CONTROL, 0xFFFF0000);
    p = pb_push1(p, NV097_SET_SWATH_WIDTH, NV097_SET_SWATH_WIDTH_V_OFF);
    p = pb_push1(p, NV097_SET_LINE_SMOOTH_ENABLE, false);
    p = pb_push1(p, NV097_SET_POLY_SMOOTH_ENABLE, false);
    pb_end(p);
  }

  RenderToAntialiasedTextureEnd(host_);

  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}
