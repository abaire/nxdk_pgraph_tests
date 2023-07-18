#include "surface_clip_tests.h"

#include "shaders/precalculated_vertex_shader.h"

#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

// From pbkit.c, DMA_A is set to channel 3 by default
// NV097_SET_CONTEXT_DMA_A == NV20_TCL_PRIMITIVE_3D_SET_OBJECT1
static constexpr uint32_t kDefaultDMAChannelA = 3;

// From pbkit.c, DMA_COLOR is set to channel 9 by default
static constexpr uint32_t kDefaultDMAColorChannel = 9;

static constexpr float kTestRectThickness = 4.0f;

// clang-format off
static constexpr SurfaceClipTests::ClipRect kTestRects[] = {
    {0, 0, 0, 0},
    // Using a zero sized clip that is offset leads to a buffer limit error while clearing on HW but is ignored on xemu.
    //{16, 8, 0, 0},
    {0, 0, 512, 384},
    {16, 8, 512, 384},
    // Using a clip size larger than the surface size leads to a buffer limit error on HW but is ignored on xemu.
    // {0, 0, 1024, 1024},
    //{16, 8, 1024, 1024},
    {0, 0, 640, 480},
    // The extents must stay within the surface size to avoid a buffer limit error on HW.
    {8, 16, 640 - 8, 480 - 16},
};
// clang-format on

static constexpr const char kXemuBug420Test[] = "XemuBug420";

static std::string MakeTestName(bool render_target, const SurfaceClipTests::ClipRect &rect) {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%sx%dy%d_w%dh%d", render_target ? "rt_" : "", rect.x, rect.y, rect.width,
           rect.height);
  return buffer;
}

SurfaceClipTests::SurfaceClipTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Surface clip") {
  for (auto &rect : kTestRects) {
    tests_[MakeTestName(false, rect)] = [this, &rect]() { Test(rect); };
    tests_[MakeTestName(true, rect)] = [this, &rect]() { TestRenderTarget(rect); };
  }

  tests_[kXemuBug420Test] = [this]() { TestXemuBug420(); };
}

void SurfaceClipTests::Initialize() {
  TestSuite::Initialize();
  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);
}

void SurfaceClipTests::Test(const ClipRect &rect) {
  std::string name = MakeTestName(false, rect);

  host_.PrepareDraw(0xFC111155);

  host_.SetSurfaceFormatImmediate(TestHost::SCF_R5G6B5, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight(), false, rect.x, rect.y, rect.width, rect.height);
  host_.ClearDepthStencilRegion(0xFFFFFF, 0x0, rect.x, rect.y, rect.width, rect.height);

  DrawTestImage(rect);

  pb_print("%s", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, name);
  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight());
}

void SurfaceClipTests::TestXemuBug420() {
  host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                         host_.GetFramebufferHeight());
  host_.PrepareDraw(0xFC111155);

  const ClipRect rect = {0, 0, 512, 384};
  host_.SetSurfaceFormatImmediate(TestHost::SCF_R5G6B5, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight(), false, rect.x, rect.y, rect.width, rect.height);

  // This triggers an assertion failure:
  // `(pg->color_binding->width == pg->zeta_binding->width) && (pg->color_binding->height == pg->zeta_binding->height)'
  host_.ClearDepthStencilRegion(0xFFFFFF, 0x0, rect.x, rect.y, rect.width, rect.height);

  pb_print("%s", kXemuBug420Test);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, kXemuBug420Test);
  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight());
}

void SurfaceClipTests::TestRenderTarget(const ClipRect &rect) {
  std::string name = MakeTestName(true, rect);

  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;

  // Point the color buffer at the texture memory.
  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, reinterpret_cast<uint32_t>(host_.GetTextureMemory()) & 0x03FFFFFF);

    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, host_.GetFramebufferWidth() * 2) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));

    // TODO: Investigate if this is actually necessary. Morrowind does this after changing offsets.
    p = pb_push1(p, NV097_NO_OPERATION, 0);
    p = pb_push1(p, NV097_WAIT_FOR_IDLE, 0);
    pb_end(p);

    // Forcibly initialize the texture memory to a dark red, since the clip rect should prevent subsequent draws/clears
    // from touching it.
    auto pixel = reinterpret_cast<uint16_t *>(host_.GetTextureMemory());
    for (uint32_t y = 0; y < host_.GetFramebufferHeight(); ++y) {
      for (uint32_t x = 0; x < host_.GetFramebufferWidth(); ++x, ++pixel) {
        *pixel = 0x004;
      }
    }

    host_.SetSurfaceFormat(TestHost::SCF_R5G6B5, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                           host_.GetFramebufferHeight(), false, rect.x, rect.y, rect.width, rect.height);
  }

  host_.PrepareDraw();

  host_.CommitSurfaceFormat();
  host_.ClearDepthStencilRegion(0xFFFFFF, 0x0, rect.x, rect.y, rect.width, rect.height);

  DrawTestImage(rect);

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
    host_.PrepareDraw(0xFE262422);

    host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_R5G6B5));
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetImageDimensions(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());

    host_.SetTextureStageEnabled(0, true);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
    host_.SetupTextureStages();

    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

    {
      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetTexCoord0(0.0f, 0.0f);
      host_.SetVertex(0.0f, 0.0f, 0.1f, 1.0f);

      host_.SetTexCoord0(host_.GetFramebufferWidthF(), 0.0f);
      host_.SetVertex(host_.GetFramebufferWidthF(), 0.0f, 0.1f, 1.0f);

      host_.SetTexCoord0(host_.GetFramebufferWidthF(), host_.GetFramebufferHeightF());
      host_.SetVertex(host_.GetFramebufferWidthF(), host_.GetFramebufferHeightF(), 0.1f, 1.0f);

      host_.SetTexCoord0(0.0f, host_.GetFramebufferHeightF());
      host_.SetVertex(0.0f, host_.GetFramebufferHeightF(), 0.1f, 1.0f);
      host_.End();
    }

    host_.SetTextureStageEnabled(0, false);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE);

    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
  }

  pb_print("%s", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, name);
}

void SurfaceClipTests::DrawTestImage(const SurfaceClipTests::ClipRect &rect) {
  // Draw red quads outside the clip area.
  {
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(1.0f, 0.0f, 0.0f);
    const auto left = static_cast<float>(rect.x) - kTestRectThickness;
    const auto top = static_cast<float>(rect.y) - kTestRectThickness;
    const auto right = static_cast<float>(rect.x + rect.width) + kTestRectThickness;
    const auto bottom = static_cast<float>(rect.y + rect.height) + kTestRectThickness;

    // Top
    host_.SetVertex(left, top, 0.1f, 1.0f);
    host_.SetVertex(right, top, 0.1f, 1.0f);
    host_.SetVertex(right, top + kTestRectThickness, 0.1f, 1.0f);
    host_.SetVertex(left, top + kTestRectThickness, 0.1f, 1.0f);

    // Bottom
    host_.SetVertex(left, bottom - kTestRectThickness, 0.1f, 1.0f);
    host_.SetVertex(right, bottom - kTestRectThickness, 0.1f, 1.0f);
    host_.SetVertex(right, bottom, 0.1f, 1.0f);
    host_.SetVertex(left, bottom, 0.1f, 1.0f);

    // Left
    host_.SetVertex(left, top, 0.1f, 1.0f);
    host_.SetVertex(left + kTestRectThickness, top, 0.1f, 1.0f);
    host_.SetVertex(left + kTestRectThickness, bottom, 0.1f, 1.0f);
    host_.SetVertex(left, bottom, 0.1f, 1.0f);

    // Right
    host_.SetVertex(right - kTestRectThickness, top, 0.1f, 1.0f);
    host_.SetVertex(right, top, 0.1f, 1.0f);
    host_.SetVertex(right, bottom, 0.1f, 1.0f);
    host_.SetVertex(right - kTestRectThickness, bottom, 0.1f, 1.0f);
    host_.End();
  }

  // Draw a dark green rect across the entire surface
  {
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0.1f, 0.6f, 0.1f);
    const auto left = static_cast<float>(rect.x) - 1.0f;
    const auto top = static_cast<float>(rect.y) - 1.0f;
    const auto right = static_cast<float>(rect.x + rect.width) + 1.0f;
    const auto bottom = static_cast<float>(rect.y + rect.height) + 1.0f;

    host_.SetVertex(left, top, 0.1f, 1.0f);
    host_.SetVertex(right, top, 0.1f, 1.0f);
    host_.SetVertex(right, bottom, 0.1f, 1.0f);
    host_.SetVertex(left, bottom, 0.1f, 1.0f);
    host_.End();
  }

  // Draw green quads at the clip area.
  {
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0.0f, 1.0f, 0.0f);
    const auto left = static_cast<float>(rect.x);
    const auto top = static_cast<float>(rect.y);
    const auto right = static_cast<float>(rect.x + rect.width);
    const auto bottom = static_cast<float>(rect.y + rect.height);

    // Top
    host_.SetVertex(left, top, 0.1f, 1.0f);
    host_.SetVertex(right, top, 0.1f, 1.0f);
    host_.SetVertex(right, top + kTestRectThickness, 0.1f, 1.0f);
    host_.SetVertex(left, top + kTestRectThickness, 0.1f, 1.0f);

    // Bottom
    host_.SetVertex(left, bottom - kTestRectThickness, 0.1f, 1.0f);
    host_.SetVertex(right, bottom - kTestRectThickness, 0.1f, 1.0f);
    host_.SetVertex(right, bottom, 0.1f, 1.0f);
    host_.SetVertex(left, bottom, 0.1f, 1.0f);

    // Left
    host_.SetVertex(left, top, 0.1f, 1.0f);
    host_.SetVertex(left + kTestRectThickness, top, 0.1f, 1.0f);
    host_.SetVertex(left + kTestRectThickness, bottom, 0.1f, 1.0f);
    host_.SetVertex(left, bottom, 0.1f, 1.0f);

    // Right
    host_.SetVertex(right - kTestRectThickness, top, 0.1f, 1.0f);
    host_.SetVertex(right, top, 0.1f, 1.0f);
    host_.SetVertex(right, bottom, 0.1f, 1.0f);
    host_.SetVertex(right - kTestRectThickness, bottom, 0.1f, 1.0f);
    host_.End();
  }
}
