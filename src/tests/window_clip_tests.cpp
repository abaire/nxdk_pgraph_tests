#include "window_clip_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "test_host.h"

#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

#define NV2A_MMIO_BASE 0xFD000000
#define BLOCKPGRAPH_ADDR 0x400000
#define PGRAPH_ADDR(a) (NV2A_MMIO_BASE + BLOCKPGRAPH_ADDR + (a))

#define NVPGRAPH_ADDR_WINDOWCLIPX0 0x00001A44
#define NVPGRAPH_ADDR_WINDOWCLIPY0 0x00001A64

// From pbkit.c, DMA_A is set to channel 3 by default
// NV097_SET_CONTEXT_DMA_A == NV20_TCL_PRIMITIVE_3D_SET_OBJECT1
const uint32_t kDefaultDMAChannelA = 3;

// From pbkit.c, DMA_COLOR is set to channel 9 by default
const uint32_t kDefaultDMAColorChannel = 9;

static constexpr uint32_t kImageWidth = 256;
static constexpr uint32_t kImageHeight = 256;
static constexpr float kLeft = 0;
static constexpr float kTop = 0;

static constexpr WindowClipTests::ClipRect kClipOne[] = {
    {0, 0, 0, 0},
    {0, 0, kImageWidth, kImageHeight},
    {8, 16, 0xF00, 0xF00},  // According to xemu the max value is 0xFFF.
    {0, 0, kImageWidth - 1, kImageHeight - 1},
    {1, 1, kImageWidth - 2, kImageHeight - 2},
    {1, 1, kImageWidth - 3, kImageHeight - 3},
    {0, 0, kImageWidth >> 1, kImageHeight >> 1},
    {0, 0, 1, 1},
    {kImageWidth - 1, kImageHeight - 1, 1, 1},
    {(kImageWidth - 64) >> 1, (kImageHeight - 64) >> 1, 64, 64},
};

static constexpr WindowClipTests::ClipRect kClipTwo[] = {
    {0, 0, 0, 0},
    {16, 16, 8, 8},
};

static std::string MakeTestName(bool clip_exclusive, bool swap_order, const WindowClipTests::ClipRect &c1,
                                const WindowClipTests::ClipRect &c2) {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%s%c_x%dy%d_w%dh%d-x%dy%d_w%dh%d", swap_order ? "s" : "",
           clip_exclusive ? 'E' : 'I', c1.x, c1.y, c1.width, c1.height, c2.x, c2.y, c2.width, c2.height);
  return buffer;
}

static std::string MakeRenderTargetTestName(bool clip_exclusive, bool swap_order, const WindowClipTests::ClipRect &c1,
                                            const WindowClipTests::ClipRect &c2) {
  return std::string("r") + MakeTestName(clip_exclusive, swap_order, c1, c2);
}

WindowClipTests::WindowClipTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Window clip") {
  for (auto exclusive : {false, true}) {
    for (auto &c2 : kClipTwo) {
      for (auto &c1 : kClipOne) {
        tests_[MakeTestName(exclusive, false, c1, c2)] = [this, exclusive, &c1, &c2]() {
          Test(exclusive, false, c1, c2);
        };
        tests_[MakeRenderTargetTestName(exclusive, false, c1, c2)] = [this, exclusive, &c1, &c2]() {
          TestRenderTarget(exclusive, false, c1, c2);
        };
      }
    }

    ClipRect c1{60, 60, 64, 64};
    ClipRect c2{0, 0, 64, 64};
    tests_[MakeTestName(exclusive, true, c1, c2)] = [this, exclusive, c1, c2]() { Test(exclusive, true, c1, c2); };
    tests_[MakeRenderTargetTestName(exclusive, true, c1, c2)] = [this, exclusive, c1, c2]() {
      TestRenderTarget(exclusive, true, c1, c2);
    };
  }
}

void WindowClipTests::Initialize() {
  TestSuite::Initialize();
  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);
}

void WindowClipTests::Draw(bool clip_exclusive, bool swap_order, uint32_t c1_left, uint32_t c1_top, uint32_t c1_right,
                           uint32_t c1_bottom, uint32_t c2_left, uint32_t c2_top, uint32_t c2_right,
                           uint32_t c2_bottom) {
  host_.SetWindowClipExclusive(false);
  host_.SetWindowClip(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());

  const auto kImageWidthF = static_cast<float>(kImageWidth);
  const auto kImageHeightF = static_cast<float>(kImageHeight);
  const float kRight = kLeft + kImageWidthF;
  const float kBottom = kTop + kImageHeightF;

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(0xFF887733);
  host_.SetVertex(kLeft, kTop, 0.1f, 1.0f);
  host_.SetVertex(kRight, kTop, 0.1f, 1.0f);
  host_.SetVertex(kRight, kBottom, 0.1f, 1.0f);
  host_.SetVertex(kLeft, kBottom, 0.1f, 1.0f);
  host_.End();

  host_.SetWindowClipExclusive(clip_exclusive);
  if (!swap_order) {
    host_.SetWindowClip(c1_right, c1_bottom, c1_left, c1_top, 0);
    host_.SetWindowClip(c2_right, c2_bottom, c2_left, c2_top, 1);
  } else {
    PrintMsg("Setting clip region 1 then region 0.\n");
    host_.SetWindowClip(c2_right, c2_bottom, c2_left, c2_top, 1);
    host_.SetWindowClip(c1_right, c1_bottom, c1_left, c1_top, 0);
  }

  for (auto i = 0; i < 8; ++i) {
    uint32_t wcx = *reinterpret_cast<uint32_t *>(PGRAPH_ADDR(NVPGRAPH_ADDR_WINDOWCLIPX0 + 4 * i));
    uint32_t wcy = *reinterpret_cast<uint32_t *>(PGRAPH_ADDR(NVPGRAPH_ADDR_WINDOWCLIPY0 + 4 * i));
    PrintMsg("UPDATED[%d]: X: 0x%X Y: 0x%X\n", i, wcx, wcy);
  }

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(0xFF3377FF);
  host_.SetVertex(kLeft, kTop, 0.1f, 1.0f);
  host_.SetVertex(kRight, kTop, 0.1f, 1.0f);
  host_.SetVertex(kRight, kBottom, 0.1f, 1.0f);
  host_.SetVertex(kLeft, kBottom, 0.1f, 1.0f);
  host_.End();

  host_.SetWindowClipExclusive(false);
  host_.SetWindowClip(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
}

void WindowClipTests::Test(bool clip_exclusive, bool swap_order, const ClipRect &c1, const ClipRect &c2) {
  std::string name = MakeTestName(clip_exclusive, swap_order, c1, c2);

  auto c1_left = c1.x + static_cast<uint32_t>(kLeft);
  auto c1_top = c1.y + static_cast<uint32_t>(kTop);
  auto c1_right = c1_left + c1.width;
  auto c1_bottom = c1_top + c1.height;
  auto c2_left = c2.x + static_cast<uint32_t>(kLeft);
  auto c2_top = c2.y + static_cast<uint32_t>(kTop);
  auto c2_right = c2_left + c2.width;
  auto c2_bottom = c2_top + c2.height;

  host_.PrepareDraw(0xFE111213);

  Draw(clip_exclusive, swap_order, c1_left, c1_top, c1_right, c1_bottom, c2_left, c2_top, c2_right, c2_bottom);

  pb_printat(0, 25, (char *)"%s", name.c_str());
  pb_printat(1, 25, (char *)"%d,%d - %d,%d", c1_left, c1_top, c1_right, c1_bottom);
  pb_printat(2, 25, (char *)"%d,%d - %d,%d", c2_left, c2_top, c2_right, c2_bottom);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, name);
}

void WindowClipTests::TestRenderTarget(bool clip_exclusive, bool swap_order, const ClipRect &c1, const ClipRect &c2) {
  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
  std::string name = MakeRenderTargetTestName(clip_exclusive, swap_order, c1, c2);

  auto c1_left = c1.x + static_cast<uint32_t>(kLeft);
  auto c1_top = c1.y + static_cast<uint32_t>(kTop);
  auto c1_right = c1_left + c1.width;
  auto c1_bottom = c1_top + c1.height;
  auto c2_left = c2.x + static_cast<uint32_t>(kLeft);
  auto c2_top = c2.y + static_cast<uint32_t>(kTop);
  auto c2_right = c2_left + c2.width;
  auto c2_bottom = c2_top + c2.height;

  host_.PrepareDraw(0xFE131314);

  // Point the color buffer at the texture memory.
  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, reinterpret_cast<uint32_t>(host_.GetTextureMemory()) & 0x03FFFFFF);

    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kImageWidth * 4) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));

    // TODO: Investigate if this is actually necessary. Morrowind does this after changing offsets.
    p = pb_push1(p, NV097_NO_OPERATION, 0);
    p = pb_push1(p, NV097_WAIT_FOR_IDLE, 0);
    pb_end(p);
    host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, kImageWidth, kImageHeight, false);
  }

  Draw(clip_exclusive, swap_order, c1_left, c1_top, c1_right, c1_bottom, c2_left, c2_top, c2_right, c2_bottom);

  // Restore the color buffer and render the texture to a quad.
  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, 0);
    pb_end(p);
    host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                           host_.GetFramebufferHeight(), false);
  }

  host_.Clear(0xFE332233);

  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8));
  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetTextureDimensions(kImageWidth, kImageHeight);
  texture_stage.SetImageDimensions(kImageWidth, kImageHeight);

  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
  host_.SetupTextureStages();

  const float left = (host_.GetFramebufferWidthF() - kImageWidth) * 0.5f;
  const float top = (host_.GetFramebufferHeightF() - kImageHeight) * 0.5f;

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  {
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(0.0f, 0.0f);
    host_.SetVertex(left, top, 0.1f, 1.0f);

    host_.SetTexCoord0(kImageWidth, 0.0f);
    host_.SetVertex(left + kImageWidth, top, 0.1f, 1.0f);

    host_.SetTexCoord0(kImageWidth, kImageHeight);
    host_.SetVertex(left + kImageWidth, top + kImageHeight, 0.1f, 1.0f);

    host_.SetTexCoord0(0.0f, kImageHeight);
    host_.SetVertex(left, top + kImageHeight, 0.1f, 1.0f);
    host_.End();
  }

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

  pb_print("%s\n", name.c_str());
  pb_print("%d,%d - %d,%d\n", c1_left, c1_top, c1_right, c1_bottom);
  pb_print("%d,%d - %d,%d\n", c2_left, c2_top, c2_right, c2_bottom);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, name);
}
