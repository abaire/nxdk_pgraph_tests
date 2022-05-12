#include "color_zeta_overlap_tests.h"

#include <pbkit/pbkit.h>

#include <utility>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "test_host.h"

#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

// From pbkit.c, DMA_A is set to channel 3 by default
// NV097_SET_CONTEXT_DMA_A == NV20_TCL_PRIMITIVE_3D_SET_OBJECT1
const uint32_t kDefaultDMAChannelA = 3;
// From pbkit.c, DMA_COLOR is set to channel 9 by default.
// NV097_SET_CONTEXT_DMA_COLOR == NV20_TCL_PRIMITIVE_3D_SET_OBJECT3
const uint32_t kDefaultDMAColorChannel = 9;
// NV097_SET_CONTEXT_DMA_ZETA is set to channel 10 by default.
const uint32_t kDefaultDMAZetaChannel = 10;

static constexpr char kColorIntoDepthTestName[] = "ColorIntoZeta";
static constexpr char kDepthIntoColorTestName[] = "ZetaIntoColor";
static constexpr char kSwapTestName[] = "Swap";

static constexpr float kLeft = -2.75f;
static constexpr float kRight = 2.75f;
static constexpr float kTop = 1.75f;
static constexpr float kBottom = -1.75f;

ColorZetaOverlapTests::ColorZetaOverlapTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Color zeta overlap") {
  tests_[kColorIntoDepthTestName] = [this]() { TestColorIntoDepth(); };
  tests_[kDepthIntoColorTestName] = [this]() { TestDepthIntoColor(); };
  tests_[kSwapTestName] = [this]() { TestSwap(); };
}

void ColorZetaOverlapTests::Initialize() {
  TestSuite::Initialize();
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  // TODO: Switch to SCF_X8R8G8B8_O8R8G8B8 when supported in xemu since the tests will never properly set alpha.
  host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                         host_.GetFramebufferHeight());

  // Depth test must be enabled or nothing will be written to the depth target.
  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_BLEND_ENABLE, false);
    p = pb_push1(p, NV097_SET_ALPHA_TEST_ENABLE, false);

    p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, true);
    p = pb_push1(p, NV097_SET_DEPTH_MASK, true);
    p = pb_push1(p, NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_ALWAYS);
    pb_end(p);
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

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAZetaChannel);
  pb_end(p);

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(kColor);
  host_.SetVertex(kLeft, kTop, kZ, 1.0f);
  host_.SetVertex(kRight, kTop, kZ, 1.0f);
  host_.SetVertex(kRight, kBottom, kZ, 1.0f);
  host_.SetVertex(kLeft, kBottom, kZ, 1.0f);
  host_.End();

  p = pb_begin();
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
  pb_end(p);

  host_.ClearColorRegion(0xFE242424);

  pb_print("%s\nZB~=0x%X\nNondeterministic!", kColorIntoDepthTestName, kColor);
  pb_draw_text_screen();

  std::string z_name = kColorIntoDepthTestName;
  z_name += "_ZB";
  host_.FinishDraw(allow_saving_, output_dir_, kColorIntoDepthTestName, z_name);
}

void ColorZetaOverlapTests::TestDepthIntoColor() {
  // Point the depth output at the color buffer and draw a quad with no color value but a big depth value.

  // The behavior of the hardware when the color and zeta targets are the same is non-deterministic and some mixture of
  // the two will result.

  constexpr uint32_t kColor = 0xFFFF00FF;
  constexpr float kZ = 192.0f;
  constexpr float kScale = 25.0f;

  host_.PrepareDraw(0xFE242424, 0);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_ZETA, kDefaultDMAColorChannel);
  pb_end(p);

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(kColor);
  host_.SetVertex(kLeft * kScale, kTop * kScale, kZ, 1.0f);
  host_.SetVertex(kRight * kScale, kTop * kScale, kZ, 1.0f);
  host_.SetVertex(kRight * kScale, kBottom * kScale, kZ, 1.0f);
  host_.SetVertex(kLeft * kScale, kBottom * kScale, kZ, 1.0f);
  host_.End();

  p = pb_begin();
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_ZETA, kDefaultDMAZetaChannel);
  pb_end(p);

  pb_print("%s\nC~=0xFF2454FE\nNondeterministic!", kDepthIntoColorTestName);
  pb_draw_text_screen();

  // The depth buffer is expected to be 0 (as it was set during the PrepareDraw).
  std::string z_name = kDepthIntoColorTestName;
  z_name += "_ZB";
  host_.FinishDraw(allow_saving_, output_dir_, kDepthIntoColorTestName, z_name);
}

void ColorZetaOverlapTests::TestSwap() {
  // Point the depth output at the color buffer and draw a quad with no color value but a big depth value.

  constexpr uint32_t kColor = 0x00A8BF00;
  constexpr float kZ = 180.0f;
  constexpr float kScale = 30.0f;

  host_.PrepareDraw(0xFE242424, 0);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAZetaChannel);
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_ZETA, kDefaultDMAColorChannel);
  pb_end(p);

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(kColor);
  host_.SetVertex(kLeft * kScale, kTop * kScale, kZ, 1.0f);
  host_.SetVertex(kRight * kScale, kTop * kScale, kZ, 1.0f);
  host_.SetVertex(kRight * kScale, kBottom * kScale, kZ, 1.0f);
  host_.SetVertex(kLeft * kScale, kBottom * kScale, kZ, 1.0f);
  host_.End();

  p = pb_begin();
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_ZETA, kDefaultDMAZetaChannel);
  pb_end(p);

  pb_print("%s\nC=0xFF2416E9\nZB=0x00A8BF00", kSwapTestName);
  pb_draw_text_screen();

  std::string z_name = kSwapTestName;
  z_name += "_ZB";
  host_.FinishDraw(allow_saving_, output_dir_, kSwapTestName, z_name);
}
