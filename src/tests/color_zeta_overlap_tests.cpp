#include "color_zeta_overlap_tests.h"

#include <pbkit/pbkit.h>

#include <utility>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "test_host.h"

#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

// From pbkit.c, DMA_A is set to channel 3 by default
// NV097_SET_CONTEXT_DMA_A == NV20_TCL_PRIMITIVE_3D_SET_OBJECT1
static constexpr uint32_t kDefaultDMAChannelA = 3;
// From pbkit.c, DMA_COLOR is set to channel 9 by default.
// NV097_SET_CONTEXT_DMA_COLOR == NV20_TCL_PRIMITIVE_3D_SET_OBJECT3
static constexpr uint32_t kDefaultDMAColorChannel = 9;
// NV097_SET_CONTEXT_DMA_ZETA is set to channel 10 by default.
static constexpr uint32_t kDefaultDMAZetaChannel = 10;

static constexpr char kColorIntoDepthTestName[] = "ColorIntoZeta";
static constexpr char kDepthIntoColorTestName[] = "ZetaIntoColor";
static constexpr char kSwapTestName[] = "Swap";
static constexpr char kXemuAdjacentWithClipOffsetTest[] = "AdjacentWithClipOffset";
static constexpr char kXemuAdjacentWithAATest[] = "AdjacentWithAA";

static constexpr float kLeft = -2.75f;
static constexpr float kRight = 2.75f;
static constexpr float kTop = 1.75f;
static constexpr float kBottom = -1.75f;

ColorZetaOverlapTests::ColorZetaOverlapTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Color zeta overlap", config) {
  tests_[kColorIntoDepthTestName] = [this]() { TestColorIntoDepth(); };
  tests_[kDepthIntoColorTestName] = [this]() { TestDepthIntoColor(); };
  tests_[kSwapTestName] = [this]() { TestSwap(); };

  for (auto swizzle : {false, true}) {
    std::string suffix = swizzle ? "_sz" : "_l";
    tests_[kXemuAdjacentWithClipOffsetTest + suffix] = [this, swizzle]() {
      TestXemuAdjacentSurfaceWithClipOffset(swizzle);
    };
  }
  tests_[kXemuAdjacentWithAATest] = [this]() { TestXemuAdjacentSurfaceWithAA(); };
}

void ColorZetaOverlapTests::Initialize() {
  TestSuite::Initialize();
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  // TODO: Switch to SCF_X8R8G8B8_O8R8G8B8 when supported in xemu since the tests will never properly set alpha.
  host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                         host_.GetFramebufferHeight());

  // Depth test must be enabled or nothing will be written to the depth target.
  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_BLEND_ENABLE, false);
    Pushbuffer::Push(NV097_SET_ALPHA_TEST_ENABLE, false);

    Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, true);
    Pushbuffer::Push(NV097_SET_DEPTH_MASK, true);
    Pushbuffer::Push(NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_ALWAYS);
    Pushbuffer::End();
  }
}

void ColorZetaOverlapTests::TestColorIntoDepth() {
  // Point the color output at the depth/stencil buffer and draw a quad with no Z value but a big color value
  // that would still be under the max depth.

  // The behavior of the hardware when the color and zeta targets are the same is non-deterministic and some mixture of
  // the two will result.

  constexpr uint32_t kColor = 0x00BFFF00;
  constexpr float kZ = 0.0f;

  host_.PrepareDraw(0xFE242424, 0);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAZetaChannel);
  Pushbuffer::End();

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(kColor);
  host_.SetVertex(kLeft, kTop, kZ, 1.0f);
  host_.SetVertex(kRight, kTop, kZ, 1.0f);
  host_.SetVertex(kRight, kBottom, kZ, 1.0f);
  host_.SetVertex(kLeft, kBottom, kZ, 1.0f);
  host_.End();

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
  Pushbuffer::End();

  host_.ClearColorRegion(0xFE242424);

  pb_print("%s\nZB~=0x%X\nNondeterministic!", kColorIntoDepthTestName, kColor);
  pb_draw_text_screen();

  FinishDraw(kColorIntoDepthTestName, true);
}

void ColorZetaOverlapTests::TestDepthIntoColor() {
  // Point the depth output at the color buffer and draw a quad with no color value but a big depth value.

  // The behavior of the hardware when the color and zeta targets are the same is non-deterministic and some mixture of
  // the two will result.

  constexpr uint32_t kColor = 0xFFFF00FF;
  constexpr float kZ = 192.0f;
  constexpr float kScale = 25.0f;

  host_.PrepareDraw(0xFE242424, 0);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_ZETA, kDefaultDMAColorChannel);
  Pushbuffer::End();

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(kColor);
  host_.SetVertex(kLeft * kScale, kTop * kScale, kZ, 1.0f);
  host_.SetVertex(kRight * kScale, kTop * kScale, kZ, 1.0f);
  host_.SetVertex(kRight * kScale, kBottom * kScale, kZ, 1.0f);
  host_.SetVertex(kLeft * kScale, kBottom * kScale, kZ, 1.0f);
  host_.End();

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_ZETA, kDefaultDMAZetaChannel);
  Pushbuffer::End();

  pb_print("%s\nC~=0xFF2454FE\nNondeterministic!", kDepthIntoColorTestName);
  pb_draw_text_screen();

  // The depth buffer is expected to be 0 (as it was set during the PrepareDraw).
  FinishDraw(kDepthIntoColorTestName, true);
}

void ColorZetaOverlapTests::TestSwap() {
  // Point the depth output at the color buffer and draw a quad with no color value but a big depth value.

  constexpr uint32_t kColor = 0x00A8BF00;
  constexpr float kZ = 180.0f;
  constexpr float kScale = 30.0f;

  host_.PrepareDraw(0xFE242424, 0);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAZetaChannel);
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_ZETA, kDefaultDMAColorChannel);
  Pushbuffer::End();

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(kColor);
  host_.SetVertex(kLeft * kScale, kTop * kScale, kZ, 1.0f);
  host_.SetVertex(kRight * kScale, kTop * kScale, kZ, 1.0f);
  host_.SetVertex(kRight * kScale, kBottom * kScale, kZ, 1.0f);
  host_.SetVertex(kLeft * kScale, kBottom * kScale, kZ, 1.0f);
  host_.End();

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_ZETA, kDefaultDMAZetaChannel);
  Pushbuffer::End();

  pb_print("%s\nC=0xFF2416E9\nZB=0x00A8BF00", kSwapTestName);
  pb_draw_text_screen();

  FinishDraw(kSwapTestName, true);
}

void ColorZetaOverlapTests::TestXemuAdjacentSurfaceWithClipOffset(bool swizzle) {
  static constexpr uint32_t kColor = 0x00A8BF00;
  static constexpr float kZ = 1.0f;

  host_.PrepareDraw(0xFE1F1F1F, 0);

  SetSurfaceDMAs();

  // Point the color and zeta surfaces at texture memory with the zeta surface consecutive to the end of color.
  static constexpr uint32_t kSurfaceWidth = 128;
  static constexpr uint32_t kSurfaceHeight = 128;
  static constexpr uint32_t kSurfacePitch = kSurfaceWidth * 4;

  static constexpr uint32_t kClipX = 1;
  static constexpr uint32_t kClipY = 1;
  static constexpr uint32_t kClipW = kSurfaceWidth - 2;
  static constexpr uint32_t kClipH = kSurfaceHeight - 2;
  {
    const uint32_t texture_memory = reinterpret_cast<uint32_t>(host_.GetTextureMemory()) & 0x03FFFFFF;
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kSurfacePitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kSurfacePitch));
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, texture_memory);
    Pushbuffer::Push(NV097_SET_SURFACE_ZETA_OFFSET, texture_memory + kSurfacePitch * kSurfaceHeight);
    Pushbuffer::End();

    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, kSurfaceWidth, kSurfaceHeight, false);

    // This will force xemu to re-check the surface.
    SetSurfaceDMAs();

    host_.Clear();

    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_CLIP_HORIZONTAL, (kClipW << 16) + kClipX);
    Pushbuffer::Push(NV097_SET_SURFACE_CLIP_VERTICAL, (kClipH << 16) + kClipY);
    Pushbuffer::End();
    // Set the surface clip to 1, 1. This will cause xemu to erroneously treat the color surface as kSurfaceWidth + 1,
    // kSurfaceHeight + 1.
    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, kSurfaceWidth, kSurfaceHeight, swizzle,
                                    kClipX, kClipY, kClipW, kClipH);
  }

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(kColor);
  host_.SetVertex(kLeft, kTop, kZ, 1.0f);
  host_.SetVertex(kRight, kTop, kZ, 1.0f);
  host_.SetVertex(kRight, kBottom, kZ, 1.0f);
  host_.SetVertex(kLeft, kBottom, kZ, 1.0f);
  host_.End();

  RestoreSurfaceDMAs();

  {
    const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, 0);
    Pushbuffer::Push(NV097_SET_SURFACE_ZETA_OFFSET, 0);
    Pushbuffer::End();
    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                                    host_.GetFramebufferHeight());
  }

  pb_print("%s\n%s\nSuccessful", kXemuAdjacentWithClipOffsetTest, swizzle ? "Swizzle" : "Linear");
  pb_draw_text_screen();

  std::string output = kXemuAdjacentWithClipOffsetTest;
  output += swizzle ? "_sz" : "_l";
  FinishDraw(output);
}

void ColorZetaOverlapTests::TestXemuAdjacentSurfaceWithAA() {
  constexpr uint32_t kColor = 0x00A8BF00;
  constexpr float kZ = 1.0f;

  host_.PrepareDraw(0xFE1F1F1F, 0);

  SetSurfaceDMAs();

  // Point the color and zeta surfaces at texture memory with the zeta surface consecutive to the end of color.
  static constexpr uint32_t kSurfaceWidth = 64;
  static constexpr uint32_t kSurfaceHeight = 64;
  static constexpr uint32_t kSurfacePitch = kSurfaceWidth * 4;
  {
    const uint32_t texture_memory = reinterpret_cast<uint32_t>(host_.GetTextureMemory()) & 0x03FFFFFF;
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kSurfacePitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kSurfacePitch));
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, texture_memory);
    Pushbuffer::Push(NV097_SET_SURFACE_ZETA_OFFSET, texture_memory + kSurfacePitch * kSurfaceHeight);
    Pushbuffer::End();
    // Enable anti aliasing.
    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, kSurfaceWidth, kSurfaceHeight, false,
                                    0, 0, 0, 0, TestHost::AA_SQUARE_OFFSET_4);
  }

  // This will force xemu to re-check the surface.
  SetSurfaceDMAs();

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(kColor);
  host_.SetVertex(kLeft, kTop, kZ, 1.0f);
  host_.SetVertex(kRight, kTop, kZ, 1.0f);
  host_.SetVertex(kRight, kBottom, kZ, 1.0f);
  host_.SetVertex(kLeft, kBottom, kZ, 1.0f);
  host_.End();

  RestoreSurfaceDMAs();

  {
    const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, 0);
    Pushbuffer::Push(NV097_SET_SURFACE_ZETA_OFFSET, 0);
    Pushbuffer::End();
    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                                    host_.GetFramebufferHeight());
  }

  pb_print("%s\nSuccessful", kXemuAdjacentWithAATest);
  pb_draw_text_screen();

  FinishDraw(kXemuAdjacentWithAATest);
}

void ColorZetaOverlapTests::SetSurfaceDMAs() const {
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_ZETA, kDefaultDMAChannelA);
  Pushbuffer::End();
}

void ColorZetaOverlapTests::RestoreSurfaceDMAs() const {
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_ZETA, kDefaultDMAZetaChannel);
  Pushbuffer::End();
}
