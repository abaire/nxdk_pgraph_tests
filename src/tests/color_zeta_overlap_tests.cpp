#include "color_zeta_overlap_tests.h"

#include <pbkit/pbkit.h>

#include <utility>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "test_host.h"
#include "texture_format.h"

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
static constexpr char kZetaOntoColorRebindTestName[] = "ZetaOntoColorRebind";
static constexpr char kColorOntoZetaRebindTestName[] = "ColorOntoZetaRebind";
static constexpr char kColorMaskedZetaOntoColorTestName[] = "ColorMaskedZetaOntoColor";
static constexpr char kReadOnlyZetaOntoColorTestName[] = "ReadOnlyZetaOntoColor";

static constexpr float kLeft = -2.75f;
static constexpr float kRight = 2.75f;
static constexpr float kTop = 1.75f;
static constexpr float kBottom = -1.75f;

static constexpr uint32_t kAllColorChannels =
    NV097_SET_COLOR_MASK_ALPHA_WRITE_ENABLE | NV097_SET_COLOR_MASK_RED_WRITE_ENABLE |
    NV097_SET_COLOR_MASK_GREEN_WRITE_ENABLE | NV097_SET_COLOR_MASK_BLUE_WRITE_ENABLE;

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc ColorIntoZeta
 *   Clears the framebuffer color to 0xFE242424 and zeta to 0, then sets the color context DMA to the zeta DMA object so
 *   that both targets share the zeta buffer. A quad with diffuse 0x00BFFF00 at Z=0 is drawn with depth test ALWAYS and
 *   depth writes enabled. The color DMA is restored and the color buffer is cleared again, so the color capture shows
 *   only the label text on the dark gray background. The result is non-deterministic: within the quad, the zeta capture
 *   mostly holds the drawn color (labeled ZB~=0x00BFFF00), with a speckled stair-stepped band of other values that
 *   varies between runs. The zeta buffer outside the quad remains 0.
 *
 * @tc ZetaIntoColor
 *   Clears the framebuffer color to 0xFE242424 and zeta to 0, then sets the zeta context DMA to the color DMA object so
 *   that both targets share the color buffer. A large quad with diffuse 0xFFFF00FF at Z=192 is drawn with depth test
 *   ALWAYS and depth writes enabled. The result is non-deterministic: the quad area is mostly light magenta (labeled
 *   C~=0xFF2454FE) with speckled and striped regions of the pure magenta diffuse color that vary between runs. The zeta
 *   capture is entirely 0.
 *
 * @tc Swap
 *   Clears the framebuffer color to 0xFE242424 and zeta to 0, then swaps the color and zeta context DMA objects and
 *   draws a large quad with diffuse 0x00A8BF00 at Z=180 with depth test ALWAYS and depth writes enabled. Both DMA
 *   objects are then restored. The color capture shows a solid red quad (labeled C=0xFF2416E9) and the zeta capture
 *   holds the drawn color (labeled ZB=0x00A8BF00) within the quad and 0 elsewhere.
 *
 * @tc AdjacentWithClipOffset_l
 *   Points a 128x128 linear A8R8G8B8 color surface at texture memory, with the Z24S8 zeta surface placed immediately
 *   after the end of the color surface. The surfaces are cleared, the surface clip is set to (1, 1) with a size of
 *   126x126, and a quad with diffuse 0x00A8BF00 is drawn into them with depth writes enabled. The surfaces are then
 *   restored to the framebuffer. Nothing is drawn to the framebuffer, so the capture shows only the label text on a
 *   dark gray background.
 *
 * @tc AdjacentWithClipOffset_sz
 *   Identical to AdjacentWithClipOffset_l, except that the surfaces are swizzled when the clip offset is applied.
 *   The capture shows only the label text on a dark gray background.
 *
 * @tc AdjacentWithAA
 *   Points a 64x64 A8R8G8B8 color surface at texture memory with 4x square-offset antialiasing enabled, with the Z24S8
 *   zeta surface placed immediately after the end of the color surface (based on the non-antialiased size). A quad with
 *   diffuse 0x00A8BF00 is drawn into them with depth writes enabled and the surfaces are then restored to the
 *   framebuffer. Nothing is drawn to the framebuffer, so the capture shows only the label text on a dark gray
 *   background.
 *
 * @tc ZetaOntoColorRebind
 *   Uses a 128x128 A8R8G8B8 color surface (C) and a Z24S8 zeta surface (Z), both linear and located in texture memory.
 *   C is filled with a red backdrop at far depth, then the left half of Z is set to near depth with color writes
 *   disabled. The color target is moved to Z's address while the zeta target is moved to an unused address, then the
 *   zeta target is moved back to Z's address so that both targets share it. Each of these steps issues a zero-area
 *   draw with color writes, depth test and depth writes enabled, so the surface bindings are evaluated without any
 *   pixels being written. The color target is then moved back to C without modifying the zeta target and a green quad
 *   covering the whole surface is drawn at a depth between near and far with depth func LESS. C is rendered to the
 *   framebuffer at 2x scale. The left (near) half shows the red backdrop and the right (far) half is green.
 *
 * @tc ColorOntoZetaRebind
 *   Identical to ZetaOntoColorRebind, except that the zeta target remains at Z's address throughout and the color
 *   target is moved onto Z's address directly (a single zero-area draw is issued while both targets share it). The
 *   left (near) half shows the red backdrop and the right (far) half is green.
 *
 * @tc ColorMaskedZetaOntoColor
 *   Uses a 128x128 A8R8G8B8 color surface (C) and a Z24S8 zeta surface (Z), both linear and located in texture memory.
 *   C is filled with a red backdrop with color writes, depth test (ALWAYS) and depth writes enabled. The zeta target is
 *   then moved onto C's address without modifying the color target, color writes are disabled, and a quad covering the
 *   left half of the surface is drawn with depth test ALWAYS and depth writes enabled. C is rendered to the framebuffer
 *   at 2x scale, and the CPU-read values of one pixel from the middle of the left half and one from the middle of the
 *   right half are printed below it. The left half is yellow-green and reads 0xDC87FC33: the upper 24 bits hold the
 *   written depth and the low byte (stencil in Z24S8) retains the backdrop's 0x33. The right half retains the red
 *   backdrop and reads 0xFFAA3333.
 *
 * @tc ReadOnlyZetaOntoColor
 *   Uses the same surfaces as ColorMaskedZetaOntoColor. The left half of C is filled with 0x00000000 and the right half
 *   with 0xFFFFFFFF (the minimum and maximum Z24S8 depth values when the memory is interpreted as zeta). The zeta
 *   target is then moved onto C's address, depth writes are disabled, and a green quad covering the whole surface is
 *   drawn at a depth between near and far with depth test LESS and color writes enabled. C is rendered to the
 *   framebuffer at 2x scale, and the CPU-read values of one pixel from the middle of each half are printed below it.
 *   The depth test reads the shared memory: the left half fails the test and remains black (0x00000000) and the right
 *   half passes and is green (0xFF33FF33).
 */
ColorZetaOverlapTests::ColorZetaOverlapTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Color zeta overlap", config) {
  tests_[kColorIntoDepthTestName] = [this]() { TestColorIntoDepth(); };
  tests_[kDepthIntoColorTestName] = [this]() { TestDepthIntoColor(); };
  tests_[kSwapTestName] = [this]() { TestSwap(); };
  tests_[kZetaOntoColorRebindTestName] = [this]() { TestZetaRebindAfterColorMove(false); };
  tests_[kColorOntoZetaRebindTestName] = [this]() { TestZetaRebindAfterColorMove(true); };
  tests_[kColorMaskedZetaOntoColorTestName] = [this]() { TestColorMaskedZetaOntoColor(); };
  tests_[kReadOnlyZetaOntoColorTestName] = [this]() { TestReadOnlyZetaOntoColor(); };

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

  host_.SetBlend(false);

  // Depth test must be enabled or nothing will be written to the depth target.
  {
    Pushbuffer::Begin();
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

void ColorZetaOverlapTests::TestZetaRebindAfterColorMove(bool color_onto_zeta) {
  static constexpr uint32_t kSurfaceSize = 128;
  static constexpr float kSurfaceSizeF = static_cast<float>(kSurfaceSize);
  static constexpr uint32_t kBackdropColor = 0xFF3333AA;
  static constexpr uint32_t kTestColor = 0xFF33FF33;
  static constexpr float kNearZ = -1.f;
  static constexpr float kMidZ = 0.f;
  static constexpr float kFarZ = 1.f;

  host_.PrepareDraw(0xFE242424);

  uint8_t* color_surface = host_.GetTextureMemoryForStage(0);
  uint8_t* zeta_surface = host_.GetTextureMemoryForStage(1);
  uint8_t* unused_surface = host_.GetTextureMemoryForStage(2);

  host_.RenderToSurfaceStart(color_surface, TestHost::SCF_A8R8G8B8, zeta_surface, TestHost::SZF_Z24S8, kSurfaceSize,
                             kSurfaceSize);
  host_.SetBlend(false);
  host_.SetDepthClip(0.f, 16777216.f);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_STENCIL_TEST_ENABLE, false);
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, true);
  Pushbuffer::Push(NV097_SET_DEPTH_MASK, true);
  Pushbuffer::Push(NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_ALWAYS);
  Pushbuffer::End();

  host_.SetDiffuse(kBackdropColor);
  host_.DrawScreenQuad(0.f, 0.f, kSurfaceSizeF, kSurfaceSizeF, kFarZ);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_COLOR_MASK, 0);
  Pushbuffer::End();
  host_.DrawScreenQuad(0.f, 0.f, kSurfaceSizeF * 0.5f, kSurfaceSizeF, kNearZ);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_COLOR_MASK, kAllColorChannels);
  Pushbuffer::End();

  auto draw_zero_area = [this]() { host_.DrawScreenQuad(0.f, 0.f, 0.f, 0.f, kMidZ); };

  if (color_onto_zeta) {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(zeta_surface));
    Pushbuffer::End();
    draw_zero_area();
  } else {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(zeta_surface));
    Pushbuffer::Push(NV097_SET_SURFACE_ZETA_OFFSET, VRAM_ADDR(unused_surface));
    Pushbuffer::End();
    draw_zero_area();

    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_ZETA_OFFSET, VRAM_ADDR(zeta_surface));
    Pushbuffer::End();
    draw_zero_area();
  }

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(color_surface));
  Pushbuffer::Push(NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_LESS);
  Pushbuffer::End();

  host_.SetDiffuse(kTestColor);
  host_.DrawScreenQuad(0.f, 0.f, kSurfaceSizeF, kSurfaceSizeF, kMidZ);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_ALWAYS);
  Pushbuffer::End();

  host_.RenderToSurfaceEnd();

  DrawOverlapSurfaceToFramebuffer(color_surface);

  const char* test_name = color_onto_zeta ? kColorOntoZetaRebindTestName : kZetaOntoColorRebindTestName;
  pb_printat(0, 0, "%s", test_name);
  pb_printat(1, 0, "Zeta: left near, right far");
  pb_printat(2, 0, "Green drawn at mid depth with LESS");
  pb_printat(15, 18, "Near");
  pb_printat(15, 38, "Far");
  pb_draw_text_screen();

  FinishDraw(test_name);
}

void ColorZetaOverlapTests::TestColorMaskedZetaOntoColor() {
  static constexpr uint32_t kSurfaceSize = 128;
  static constexpr float kSurfaceSizeF = static_cast<float>(kSurfaceSize);
  static constexpr uint32_t kBackdropColor = 0xFF3333AA;
  static constexpr float kMidZ = 0.f;
  static constexpr float kFarZ = 1.f;

  host_.PrepareDraw(0xFE242424);

  uint8_t* color_surface = host_.GetTextureMemoryForStage(0);
  uint8_t* zeta_surface = host_.GetTextureMemoryForStage(1);

  host_.RenderToSurfaceStart(color_surface, TestHost::SCF_A8R8G8B8, zeta_surface, TestHost::SZF_Z24S8, kSurfaceSize,
                             kSurfaceSize);
  host_.SetBlend(false);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_STENCIL_TEST_ENABLE, false);
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, true);
  Pushbuffer::Push(NV097_SET_DEPTH_MASK, true);
  Pushbuffer::Push(NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_ALWAYS);
  Pushbuffer::End();

  host_.SetDiffuse(kBackdropColor);
  host_.DrawScreenQuad(0.f, 0.f, kSurfaceSizeF, kSurfaceSizeF, kFarZ);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_COLOR_MASK, 0);
  Pushbuffer::Push(NV097_SET_SURFACE_ZETA_OFFSET, VRAM_ADDR(color_surface));
  Pushbuffer::End();
  host_.DrawScreenQuad(0.f, 0.f, kSurfaceSizeF * 0.5f, kSurfaceSizeF, kMidZ);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_COLOR_MASK, kAllColorChannels);
  Pushbuffer::Push(NV097_SET_SURFACE_ZETA_OFFSET, VRAM_ADDR(zeta_surface));
  Pushbuffer::End();

  host_.RenderToSurfaceEnd();

  host_.PBKitBusyWait();
  auto pixels = reinterpret_cast<volatile uint32_t*>(color_surface);
  const uint32_t middle_row = (kSurfaceSize / 2) * kSurfaceSize;
  const uint32_t left_pixel = pixels[middle_row + kSurfaceSize / 4];
  const uint32_t right_pixel = pixels[middle_row + (kSurfaceSize * 3) / 4];

  DrawOverlapSurfaceToFramebuffer(color_surface);

  pb_printat(0, 0, "%s", kColorMaskedZetaOntoColorTestName);
  pb_printat(1, 0, "Zeta moved onto bound color, color masked");
  pb_printat(2, 0, "Left half drawn with depth writes only");
  pb_printat(15, 18, "0x%08X", left_pixel);
  pb_printat(15, 32, "0x%08X", right_pixel);
  pb_draw_text_screen();

  FinishDraw(kColorMaskedZetaOntoColorTestName);
}

void ColorZetaOverlapTests::TestReadOnlyZetaOntoColor() {
  static constexpr uint32_t kSurfaceSize = 128;
  static constexpr float kSurfaceSizeF = static_cast<float>(kSurfaceSize);
  static constexpr uint32_t kMinDepthColor = 0x00000000;
  static constexpr uint32_t kMaxDepthColor = 0xFFFFFFFF;
  static constexpr uint32_t kTestColor = 0xFF33FF33;
  static constexpr float kMidZ = 0.f;
  static constexpr float kFarZ = 1.f;

  host_.PrepareDraw(0xFE242424);

  uint8_t* color_surface = host_.GetTextureMemoryForStage(0);
  uint8_t* zeta_surface = host_.GetTextureMemoryForStage(1);

  host_.RenderToSurfaceStart(color_surface, TestHost::SCF_A8R8G8B8, zeta_surface, TestHost::SZF_Z24S8, kSurfaceSize,
                             kSurfaceSize);
  host_.SetBlend(false);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_STENCIL_TEST_ENABLE, false);
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, true);
  Pushbuffer::Push(NV097_SET_DEPTH_MASK, true);
  Pushbuffer::Push(NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_ALWAYS);
  Pushbuffer::End();

  host_.SetDiffuse(kMinDepthColor);
  host_.DrawScreenQuad(0.f, 0.f, kSurfaceSizeF * 0.5f, kSurfaceSizeF, kFarZ);
  host_.SetDiffuse(kMaxDepthColor);
  host_.DrawScreenQuad(kSurfaceSizeF * 0.5f, 0.f, kSurfaceSizeF, kSurfaceSizeF, kFarZ);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_SURFACE_ZETA_OFFSET, VRAM_ADDR(color_surface));
  Pushbuffer::Push(NV097_SET_DEPTH_MASK, false);
  Pushbuffer::Push(NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_LESS);
  Pushbuffer::End();

  host_.SetDiffuse(kTestColor);
  host_.DrawScreenQuad(0.f, 0.f, kSurfaceSizeF, kSurfaceSizeF, kMidZ);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DEPTH_MASK, true);
  Pushbuffer::Push(NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_ALWAYS);
  Pushbuffer::Push(NV097_SET_SURFACE_ZETA_OFFSET, VRAM_ADDR(zeta_surface));
  Pushbuffer::End();

  host_.RenderToSurfaceEnd();

  host_.PBKitBusyWait();
  auto pixels = reinterpret_cast<volatile uint32_t*>(color_surface);
  const uint32_t middle_row = (kSurfaceSize / 2) * kSurfaceSize;
  const uint32_t left_pixel = pixels[middle_row + kSurfaceSize / 4];
  const uint32_t right_pixel = pixels[middle_row + (kSurfaceSize * 3) / 4];

  DrawOverlapSurfaceToFramebuffer(color_surface);

  pb_printat(0, 0, "%s", kReadOnlyZetaOntoColorTestName);
  pb_printat(1, 0, "Zeta moved onto color, depth writes off");
  pb_printat(2, 0, "Green drawn at mid depth with LESS");
  pb_printat(15, 18, "0x%08X", left_pixel);
  pb_printat(15, 32, "0x%08X", right_pixel);
  pb_draw_text_screen();

  FinishDraw(kReadOnlyZetaOntoColorTestName);
}

void ColorZetaOverlapTests::DrawOverlapSurfaceToFramebuffer(uint8_t* surface) {
  static constexpr uint32_t kSurfaceSize = 128;
  static constexpr float kDisplaySize = static_cast<float>(kSurfaceSize) * 2.f;

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  auto& texture_stage = host_.GetTextureStage(0);
  texture_stage.SetEnabled();
  texture_stage.SetFormat(GetTextureFormatInfo(TextureFormatForSurfaceFormat(TestHost::SCF_A8R8G8B8, true)));
  texture_stage.SetImageDimensions(kSurfaceSize, kSurfaceSize);
  host_.SetupTextureStages();

  const float left = host_.CenterX(kDisplaySize);
  const float top = host_.CenterY(kDisplaySize);
  host_.DrawTexturedScreenQuad(left, top, left + kDisplaySize, top + kDisplaySize, 0.f, kSurfaceSize, kSurfaceSize);

  texture_stage.SetEnabled(false);
  host_.SetupTextureStages();
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  host_.SetFinalCombiner0(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false,
                          false, TestHost::SRC_R0);
  host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_R0, true,
                          false, false, false, true);
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
