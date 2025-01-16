#include "dma_corruption_around_surface_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "test_host.h"
#include "texture_generator.h"

#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

#define NV2A_MMIO_BASE 0xFD000000
#define BLOCKPGRAPH_ADDR 0x400000
#define PGRAPH_ADDR(a) (NV2A_MMIO_BASE + BLOCKPGRAPH_ADDR + (a))

// From pbkit.c, DMA_A is set to channel 3 by default
// NV097_SET_CONTEXT_DMA_A == NV20_TCL_PRIMITIVE_3D_SET_OBJECT1
static constexpr uint32_t kDefaultDMAChannelA = 3;
// From pbkit.c, DMA_COLOR is set to channel 9 by default.
// NV097_SET_CONTEXT_DMA_COLOR == NV20_TCL_PRIMITIVE_3D_SET_OBJECT3
static constexpr uint32_t kDefaultDMAColorChannel = 9;

static constexpr char kTestName[] = "DMAOverlap";
static constexpr uint32_t kTextureSize = 128;

static constexpr uint32_t kCheckerSize = 8;
static constexpr uint32_t kCheckerboardB = 0xFF3333C0;

DMACorruptionAroundSurfaceTests::DMACorruptionAroundSurfaceTests(TestHost &host, std::string output_dir,
                                                                 const Config &config)
    : TestSuite(host, std::move(output_dir), "DMA corruption around surfaces", config) {
  tests_[kTestName] = [this]() { Test(); };
}

void DMACorruptionAroundSurfaceTests::Initialize() {
  TestSuite::Initialize();
  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);
}

void DMACorruptionAroundSurfaceTests::Test() {
  host_.PrepareDraw(0xFF050505);

  const uint32_t kRenderBufferPitch = kTextureSize * 4;
  const auto kHalfTextureSize = kRenderBufferPitch * (kTextureSize >> 1);

  // Configure a surface pointing at texture memory and do a no-op draw to force xemu to create a SurfaceBinding.
  {
    const auto kTextureMemory = reinterpret_cast<uint32_t>(host_.GetTextureMemoryForStage(0));
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kRenderBufferPitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kRenderBufferPitch));
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(kTextureMemory + kHalfTextureSize));
    pb_end(p);

    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, kTextureSize, kTextureSize);
    NoOpDraw();
  }

  // Do a CPU copy to texture memory
  WaitForGPU();
  GenerateRGBACheckerboard(host_.GetTextureMemoryForStage(0), 0, 0, kTextureSize, kTextureSize, kRenderBufferPitch,
                           0xFFCC3333, kCheckerboardB, kCheckerSize);

  // Point the output surface back at the framebuffer.
  {
    const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, 0);
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    pb_end(p);
  }

  // Restore the surface format to allow the texture to be rendered.
  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight());

  // Draw a quad with the texture.
  Draw();

  pb_print("%s\n", kTestName);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, kTestName);
}

void DMACorruptionAroundSurfaceTests::Draw() const {
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8));
  texture_stage.SetImageDimensions(kTextureSize, kTextureSize);
  host_.SetupTextureStages();

  static constexpr float inset = 64.0f;
  const float left = inset;
  const float right = host_.GetFramebufferWidthF() - inset;
  const float top = inset;
  const float bottom = host_.GetFramebufferHeightF() - inset;
  const float mid_x = left + (right - left) * 0.5f;
  const float mid_y = top + (bottom - top) * 0.5f;

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetTexCoord0(0.0f, 0.0f);
  host_.SetVertex(left, mid_y, 0.1f, 1.0f);

  host_.SetTexCoord0(kTextureSize, 0.0f);
  host_.SetVertex(mid_x, top, 0.1f, 1.0f);

  host_.SetTexCoord0(kTextureSize, kTextureSize);
  host_.SetVertex(right, mid_y, 0.1f, 1.0f);

  host_.SetTexCoord0(0.0f, kTextureSize);
  host_.SetVertex(mid_x, bottom, 0.1f, 1.0f);
  host_.End();

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);
}

void DMACorruptionAroundSurfaceTests::NoOpDraw() const {
  host_.Begin(TestHost::PRIMITIVE_TRIANGLES);
  host_.SetVertex(0.0f, 0.0f, 0.1f, 1.0f);
  host_.SetVertex(0.0f, 0.0f, 0.1f, 1.0f);
  host_.SetVertex(0.0f, 0.0f, 0.1f, 1.0f);
  host_.End();
}

void DMACorruptionAroundSurfaceTests::WaitForGPU() const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_NO_OPERATION, 0);
  p = pb_push1(p, NV097_WAIT_FOR_IDLE, 0);
  pb_end(p);
}
