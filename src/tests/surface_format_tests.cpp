#include "surface_format_tests.h"

#include <shaders/passthrough_vertex_shader.h>
#include <texture_generator.h>

#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

// From pbkit.c, DMA_A is set to channel 3 by default
// NV097_SET_CONTEXT_DMA_A == NV20_TCL_PRIMITIVE_3D_SET_OBJECT1
static constexpr uint32_t kDefaultDMAChannelA = 3;

// From pbkit.c, DMA_COLOR is set to channel 9 by default
static constexpr uint32_t kDefaultDMAColorChannel = 9;

static constexpr uint32_t kTextureSize = 128;
static constexpr uint32_t kTestTextureFormat = NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8;

static constexpr uint32_t kCheckerboardA = 0xFF202020;
static constexpr uint32_t kCheckerboardB = 0xFF000000;

struct NamedSurfaceFormat {
  const char *name;
  TestHost::SurfaceColorFormat format;
};

static constexpr NamedSurfaceFormat kSurfaceFormats[]{
    {"Fmt_X1R5G5B5_Z1R5G5B5", TestHost::SCF_X1R5G5B5_Z1R5G5B5},
    {"Fmt_X1R5G5B5_O1R5G5B5", TestHost::SCF_X1R5G5B5_O1R5G5B5},
    {"Fmt_R5G6B5", TestHost::SCF_R5G6B5},
    {"Fmt_X8R8G8B8_Z8R8G8B8", TestHost::SCF_X8R8G8B8_Z8R8G8B8},
    {"Fmt_X8R8G8B8_O8R8G8B8", TestHost::SCF_X8R8G8B8_O8R8G8B8},
    {"Fmt_X1A7R8G8B8_Z1A7R8G8B8", TestHost::SCF_X1A7R8G8B8_Z1A7R8G8B8},
    {"Fmt_X1A7R8G8B8_O1A7R8G8B8", TestHost::SCF_X1A7R8G8B8_O1A7R8G8B8},
    {"Fmt_A8R8G8B8", TestHost::SCF_A8R8G8B8},
    {"Fmt_B8", TestHost::SCF_B8},
    {"Fmt_G8B8", TestHost::SCF_G8B8},
};

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc Fmt_A8R8G8B8
 *   Tests the A8R8G8B8 surface format.
 *   Quads appear the same as if they were rendered directly into the framebuffer.
 *
 * @tc Fmt_B8
 *   Tests the B8 surface format.
 *   Quads appear as alpha+luma ramps and are distorted and repeated due to the difference in bytes per pixel.
 *
 * @tc Fmt_G8B8
 *   Tests the G8B8 surface format.
 *   Quads set the red channel equal to the blue channel and alpha equal to the green channel. Quads are once again
 *   repeated due to the difference in bytes per pixel.
 *
 * @tc Fmt_R5G6B5
 *   Tests the R5G6B5 surface format.
 *
 * @tc Fmt_X1A7R8G8B8_O1A7R8G8B8
 *   Tests the X1A7R8G8B8_O1A7R8G8B8 surface format.
 *   Quads appear similar to A8R8G8B8, but the high bit of the alpha is set to 1, causing all quads to be less
 *   translucent.
 *
 * @tc Fmt_X1A7R8G8B8_Z1A7R8G8B8
 *   Tests the X1A7R8G8B8_Z1A7R8G8B8 surface format.
 *   Quads appear similar to A8R8G8B8, but the high bit of the alpha is set to 0, causing all quads to be translucent.
 *
 * @tc Fmt_X1R5G5B5_O1R5G5B5
 *   Tests the X1R5G5B5_O1R5G5B5 surface format.
 *
 * @tc Fmt_X1R5G5B5_Z1R5G5B5
 *   Tests the X1R5G5B5_Z1R5G5B5 surface format.
 *
 * @tc Fmt_X8R8G8B8_O8R8G8B8
 *   Tests the X8R8G8B8_O8R8G8B8 surface format.
 *   Quads appear similar to A8R8G8B8, but the alpha is set to fully opaque.
 *
 * @tc Fmt_X8R8G8B8_Z8R8G8B8
 *   Tests the X8R8G8B8_Z8R8G8B8 surface format.
 *   Quads appear similar to A8R8G8B8, but the alpha is set to fully transparent (no quads should appear in the top
 *   half of the image).
 *
 */
SurfaceFormatTests::SurfaceFormatTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Surface format", config) {
  for (auto &format : kSurfaceFormats) {
    std::string name = format.name;
    tests_[name] = [this, name, &format]() { Test(name, format.format); };
  }
}

void SurfaceFormatTests::Initialize() {
  TestSuite::Initialize();

  // Place the test pattern into tex3.
  auto texture_memory = host_.GetTextureMemoryForStage(3);
  GenerateRGBRadialATestPattern(texture_memory, kTextureSize, kTextureSize);
}

static void DrawQuads(TestHost &host) {
  host.SetTextureStageEnabled(3, true);
  host.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_NONE, TestHost::STAGE_NONE,
                             TestHost::STAGE_2D_PROJECTIVE);
  auto &texture_stage = host.GetTextureStage(3);
  texture_stage.SetFormat(GetTextureFormatInfo(kTestTextureFormat));
  texture_stage.SetImageDimensions(kTextureSize, kTextureSize);
  host.SetupTextureStages();

  static constexpr float kTop = 96.f;
  static constexpr float kSpacing = 16.f;
  const auto left = (host.GetFramebufferWidthF() - (kTextureSize * 2.f + kSpacing)) * 0.5f;

  auto draw_quad = [&host](float left, float top) {
    host.Begin(TestHost::PRIMITIVE_QUADS);

    host.SetTexCoord3(0.f, 0.f);
    host.SetVertex(left, top, 10.f);

    host.SetTexCoord3(kTextureSize, 0.f);
    host.SetVertex(left + kTextureSize, top, 10.f);

    host.SetTexCoord3(kTextureSize, kTextureSize);
    host.SetVertex(left + kTextureSize, top + kTextureSize, 10.f);

    host.SetTexCoord3(0.f, kTextureSize);
    host.SetVertex(left, top + kTextureSize, 10.f);

    host.End();
  };

  host.SetFinalCombiner0Just(TestHost::SRC_TEX3);
  host.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
  draw_quad(left, kTop);

  host.SetFinalCombiner1Just(TestHost::SRC_TEX3, true);
  draw_quad(left + kSpacing + kTextureSize, kTop);

  host.SetTextureStageEnabled(3, false);
  host.SetShaderStageProgram(TestHost::STAGE_NONE);
}

void SurfaceFormatTests::Test(const std::string &name, TestHost::SurfaceColorFormat color_format) {
  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  host_.PrepareDraw(0xFF111111);

  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
  GenerateRGBACheckerboard(pb_back_buffer(), 0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight(),
                           kFramebufferPitch, kCheckerboardA, kCheckerboardB);

  // TODO: Verify that this is true for HW and not just a pbkit limitation.
  // Certain color formats are not allowed as framebuffers, so the test image is first rendered to a non-fb surface.
  RenderToTextureStart(color_format);
  DrawQuads(host_);
  RenderToTextureEnd();

  // Render the top half of the resulting surface to the framebuffer as RGBA8 data, then a second time below with alpha
  // forced to opaque.
  {
    host_.SetTextureFormat(GetTextureFormatInfo(kTestTextureFormat));
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetImageDimensions(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
    host_.SetTextureStageEnabled(0, true);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
    host_.SetupTextureStages();

    const auto framebuffer_half_height = host_.GetFramebufferHeightF() * 0.5f;

    auto render_result = [this, framebuffer_half_height](float top) {
      const auto framebuffer_width = host_.GetFramebufferWidthF();

      host_.Begin(TestHost::PRIMITIVE_QUADS);

      host_.SetTexCoord0(0.f, 0.f);
      host_.SetVertex(0.f, top, 10.f);

      host_.SetTexCoord0(framebuffer_width, 0.f);
      host_.SetVertex(framebuffer_width, top, 10.f);

      host_.SetTexCoord0(framebuffer_width, framebuffer_half_height);
      host_.SetVertex(framebuffer_width, top + framebuffer_half_height, 10.f);

      host_.SetTexCoord0(0.f, framebuffer_half_height);
      host_.SetVertex(0.f, top + framebuffer_half_height, 10.f);

      host_.End();
    };

    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);
    render_result(0.f);

    host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
    render_result(framebuffer_half_height);

    host_.SetTextureStageEnabled(0, false);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE);
  }

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

  pb_print("%s", name.c_str());
  pb_printat(8, 0, "With alpha");
  pb_printat(12, 0, "Opaque");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

void SurfaceFormatTests::RenderToTextureStart(TestHost::SurfaceColorFormat color_format) const {
  const auto kTextureMemory = reinterpret_cast<uint32_t>(host_.GetTextureMemoryForStage(0));
  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;

  // Forcibly clear all texture memory so that formats that do not use it all won't leave garbage.
  memset(host_.GetTextureMemoryForStage(0), 0, host_.GetFramebufferWidth() * host_.GetFramebufferHeight() * 4);

  const uint32_t texture_pitch = TestHost::GetSurfaceColorPitch(color_format, host_.GetFramebufferWidth());

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
  Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, texture_pitch) |
                                                SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
  Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(kTextureMemory));
  Pushbuffer::End();

  // Failing to disable alpha blending on B8 and G8B8 will trigger a hardware exception.
  host_.SetBlend(TestHost::SurfaceSupportsAlpha(color_format));

  host_.SetSurfaceFormat(color_format, TestHost::SZF_Z16, host_.GetFramebufferWidth(), host_.GetFramebufferHeight(),
                         false);
  host_.CommitSurfaceFormat();
}

void SurfaceFormatTests::RenderToTextureEnd() const {
  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
  Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, 0);
  Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                                                SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
  Pushbuffer::End();

  host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                         host_.GetFramebufferHeight(), false);
  host_.CommitSurfaceFormat();

  host_.SetBlend(true);
}
