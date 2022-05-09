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

static constexpr uint32_t kImageWidth = 256;
static constexpr uint32_t kImageHeight = 256;

static constexpr char kClipOrderingTest[] = "ClipOrdering";

static constexpr WindowClipTests::ClipRect kClipOne[] = {
    {0, 0, 0, 0},
    {0, 0, kImageWidth, kImageHeight},
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

WindowClipTests::WindowClipTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Window clip") {
  for (auto exclusive : {false, true}) {
    for (auto &c2 : kClipTwo) {
      for (auto &c1 : kClipOne) {
        tests_[MakeTestName(exclusive, false, c1, c2)] = [this, exclusive, &c1, &c2]() {
          Test(exclusive, false, c1, c2);
        };
      }
    }

    ClipRect c1{60, 60, 64, 64};
    ClipRect c2{0, 0, 64, 64};
    tests_[MakeTestName(exclusive, true, c1, c2)] = [this, exclusive, c1, c2]() { Test(exclusive, true, c1, c2); };
  }
}

void WindowClipTests::Test(bool clip_exclusive, bool swap_order, const ClipRect &c1, const ClipRect &c2) {
  std::string name = MakeTestName(clip_exclusive, swap_order, c1, c2);
  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);

  const auto kImageWidthF = static_cast<float>(kImageWidth);
  const auto kImageHeightF = static_cast<float>(kImageHeight);
  const float kLeft = (host_.GetFramebufferWidthF() - kImageWidthF) * 0.5f;
  const float kRight = kLeft + kImageWidthF;
  const float kTop = (host_.GetFramebufferHeightF() - kImageHeightF) * 0.5f;
  const float kBottom = kTop + kImageHeightF;

  auto c1_left = c1.x + static_cast<uint32_t>(kLeft);
  auto c1_top = c1.y + static_cast<uint32_t>(kTop);
  auto c1_right = c1_left + c1.width;
  auto c1_bottom = c1_top + c1.height;
  auto c2_left = c2.x + static_cast<uint32_t>(kLeft);
  auto c2_top = c2.y + static_cast<uint32_t>(kTop);
  auto c2_right = c2_left + c2.width;
  auto c2_bottom = c2_top + c2.height;

  host_.SetWindowClipExclusive(false);
  host_.SetWindowClip(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());

  host_.PrepareDraw(0xFE111213);

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
  host_.ClearWindowClip(1);

  pb_print("%s\n", name.c_str());
  pb_print("%d,%d - %d,%d\n", c1_left, c1_top, c1_right, c1_bottom);
  pb_print("%d,%d - %d,%d\n", c2_left, c2_top, c2_right, c2_bottom);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, name);
}
