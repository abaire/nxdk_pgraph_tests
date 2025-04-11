#include "surface_format_tests.h"

#include <texture_generator.h>

#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

// From pbkit.c, DMA_A is set to channel 3 by default
// NV097_SET_CONTEXT_DMA_A == NV20_TCL_PRIMITIVE_3D_SET_OBJECT1
static constexpr uint32_t kDefaultDMAChannelA = 3;

// From pbkit.c, DMA_COLOR is set to channel 9 by default
static constexpr uint32_t kDefaultDMAColorChannel = 9;

static constexpr uint32_t kTextureSize = 256;
static constexpr uint32_t kRenderBufferSize = 768;

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

  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  // Place the test pattern into tex3.
  auto texture_memory = host_.GetTextureMemoryForStage(3);
  GenerateRGBRadialATestPattern(texture_memory, kTextureSize, kTextureSize);
}

static void DrawQuads(TestHost &host) {
  auto draw_quad = [&host](float left, float top) {
    host.Begin(TestHost::PRIMITIVE_QUADS);

    host.SetTexCoord3(0.f, 0.f);
    host.SetScreenVertex(left, top);

    host.SetTexCoord3(kTextureSize, 0.f);
    host.SetScreenVertex(left + kTextureSize, top);

    host.SetTexCoord3(kTextureSize, kTextureSize);
    host.SetScreenVertex(left + kTextureSize, top + kTextureSize);

    host.SetTexCoord3(0.f, kTextureSize);
    host.SetScreenVertex(left, top + kTextureSize);

    host.End();
  };

  host.SetTextureStageEnabled(3, true);
  host.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_NONE, TestHost::STAGE_NONE,
                             TestHost::STAGE_2D_PROJECTIVE);
  auto &texture_stage = host.GetTextureStage(3);
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8B8G8R8));
  texture_stage.SetImageDimensions(kTextureSize, kTextureSize);
  host.SetupTextureStages();

  host.SetFinalCombiner0Just(TestHost::SRC_TEX3);
  host.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  static constexpr float kTop = 96.f;
  static constexpr float kSpacing = 16.f;
  const auto left = (host.GetFramebufferWidthF() - (kTextureSize * 2.f + kSpacing)) * 0.5f;
  draw_quad(left, kTop);

  host.SetFinalCombiner1Just(TestHost::SRC_TEX3, true);
  draw_quad(left + kSpacing + kTextureSize, kTop);

  host.SetTextureStageEnabled(3, false);
  host.SetShaderStageProgram(TestHost::STAGE_NONE);
}

void SurfaceFormatTests::Test(const std::string &name, TestHost::SurfaceColorFormat color_format) {
  host_.PrepareDraw(0xFF111111);

  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
  GenerateRGBACheckerboard(pb_back_buffer(), 0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight(),
                           kFramebufferPitch, kCheckerboardA, kCheckerboardB);

  // TODO: Verify that this is true for HW and not just a pbkit limitation.
  // Certain color formats are not allowed as framebuffers, so the test image is first rendered to a non-fb surface.
  RenderToTextureStart(color_format);
  DrawQuads(host_);
  RenderToTextureEnd();

  // Render the resulting surface to the framebuffer as RGBA8 data.
  {
    host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_R8G8B8A8));
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetImageDimensions(kRenderBufferSize, kRenderBufferSize);
    host_.SetTextureStageEnabled(0, true);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
    host_.SetupTextureStages();

    const auto framebuffer_width = host_.GetFramebufferWidthF();
    const auto framebuffer_height = host_.GetFramebufferHeightF();
    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

    host_.Begin(TestHost::PRIMITIVE_QUADS);

    host_.SetTexCoord0(0.f, 0.f);
    host_.SetScreenVertex(0.f, 0.f);

    host_.SetTexCoord0(framebuffer_width, 0.f);
    host_.SetScreenVertex(framebuffer_width, 0.f);

    host_.SetTexCoord0(framebuffer_width, framebuffer_height);
    host_.SetScreenVertex(framebuffer_width, framebuffer_height);

    host_.SetTexCoord0(0.f, framebuffer_height);
    host_.SetScreenVertex(0.f, framebuffer_height);

    host_.End();

    host_.SetTextureStageEnabled(0, false);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE);
  }

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

  pb_print("%s", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

void SurfaceFormatTests::RenderToTextureStart(TestHost::SurfaceColorFormat color_format) const {
  const auto kTextureMemory = reinterpret_cast<uint32_t>(host_.GetTextureMemoryForStage(0));
  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;

  // Forcibly clear all texture memory so that formats that do not use it all won't leave garbage.
  memset(host_.GetTextureMemoryForStage(0), 0, kRenderBufferSize * kRenderBufferSize * 4);

  const uint32_t texture_pitch = TestHost::GetSurfaceColorPitch(color_format, kRenderBufferSize);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
  p = pb_push1(p, NV097_SET_SURFACE_PITCH,
               SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, texture_pitch) |
                   SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
  p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(kTextureMemory));
  pb_end(p);

  host_.SetSurfaceFormat(color_format, TestHost::SZF_Z24S8, kRenderBufferSize, kRenderBufferSize, false);
  host_.CommitSurfaceFormat();
}

void SurfaceFormatTests::RenderToTextureEnd() const {
  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
  p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, 0);
  p = pb_push1(p, NV097_SET_SURFACE_PITCH,
               SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                   SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
  pb_end(p);

  host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                         host_.GetFramebufferHeight(), false);
  host_.CommitSurfaceFormat();
}
