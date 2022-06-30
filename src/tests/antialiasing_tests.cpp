#include "antialiasing_tests.h"

#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "shaders/precalculated_vertex_shader.h"
#include "test_host.h"
#include "texture_generator.h"

// From pbkit.c, DMA_A is set to channel 3 by default
// NV097_SET_CONTEXT_DMA_A == NV20_TCL_PRIMITIVE_3D_SET_OBJECT1
static constexpr uint32_t kDefaultDMAChannelA = 3;
// From pbkit.c, DMA_COLOR is set to channel 9 by default.
// NV097_SET_CONTEXT_DMA_COLOR == NV20_TCL_PRIMITIVE_3D_SET_OBJECT3
static constexpr uint32_t kDefaultDMAColorChannel = 9;

constexpr uint32_t kCheckerSize = 8;
static constexpr uint32_t kCheckerboardA = 0xFF808080;
static constexpr uint32_t kCheckerboardB = 0xFF3333C0;

static constexpr const char kAANone[] = "CreateSurfaceWithCenter1";
static constexpr const char kAA2[] = "CreateSurfaceWithCenterCorner2";
static constexpr const char kAA4[] = "CreateSurfaceWithSquareOffset4";
static constexpr const char kOnOffCPUWrite[] = "AAOnThenOffCPUWrite";
static constexpr const char kModifyNonFramebufferSurface[] = "SurfaceStatesAreIndependent";
static constexpr const char kFramebufferIsIndependent[] = "FramebufferNotModifiedBySurfaceState";
static constexpr const char kCPUWriteIgnoresSurfaceConfig[] = "CPUWriteIgnoresSurfaceConfig";
static constexpr const char kGPUAAWriteAfterCPUWrite[] = "GPUAAWriteAfterCPUWrite";

static constexpr uint32_t kTextureSize = 128;

AntialiasingTests::AntialiasingTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Antialiasing tests") {
  tests_[kAANone] = [this]() { Test(kAANone, TestHost::AA_CENTER_1); };
  tests_[kAA2] = [this]() { Test(kAA2, TestHost::AA_CENTER_CORNER_2); };
  tests_[kAA4] = [this]() { Test(kAA4, TestHost::AA_SQUARE_OFFSET_4); };
  tests_[kOnOffCPUWrite] = [this]() { TestAAOnThenOffThenCPUWrite(); };
  tests_[kModifyNonFramebufferSurface] = [this]() { TestModifyNonFramebufferSurface(); };
  tests_[kFramebufferIsIndependent] = [this]() { TestFramebufferIsIndependentOfSurface(); };
  tests_[kCPUWriteIgnoresSurfaceConfig] = [this]() { TestCPUWriteIgnoresSurfaceConfig(); };
  tests_[kGPUAAWriteAfterCPUWrite] = [this]() { TestGPUAAWriteAfterCPUWrite(); };
};

void AntialiasingTests::Initialize() {
  TestSuite::Initialize();

  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);
}

void AntialiasingTests::Test(const char *name, TestHost::AntiAliasingSetting aa) {
  // Point the color surface at texture memory, configure the surface with some anti-aliasing mode, clear, then point
  // the surface back to the framebuffer and render the texture memory.

  // Create a texture with an obvious border around it.
  memset(host_.GetTextureMemory(), 0xCC, kTextureSize * kTextureSize * 4);
  GenerateRGBACheckerboard(host_.GetTextureMemory(), 2, 2, kTextureSize - 4, kTextureSize - 4, kTextureSize * 4,
                           kCheckerboardA, kCheckerboardB, kCheckerSize);

  {
    // Hardware will assert with a limit error if the pitch is not sufficiently large to accommodate the anti aliasing
    // mode increase. Technically this should be based off of the AA mode, but in practice it's fine to use the max
    // value.
    static constexpr uint32_t anti_aliasing_multiplier = 4;

    const auto kTextureMemory = reinterpret_cast<uint32_t>(host_.GetTextureMemoryForStage(0));
    const uint32_t kRenderBufferPitch = kTextureSize * 4 * anti_aliasing_multiplier;
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kRenderBufferPitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kRenderBufferPitch));
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(kTextureMemory));
    pb_end(p);
  }

  // To allow the test to be run more than once, a dummy draw is done so that the next surface format call will recreate
  // the xemu SurfaceBinding.
  {
    host_.SetSurfaceFormatImmediate(TestHost::SCF_R5G6B5, TestHost::SZF_Z16, kTextureSize, kTextureSize);
    NoOpDraw();
  }

  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, kTextureSize, kTextureSize, false, 0, 0, 0,
                                  0, aa);

  // A nop draw is done to finish forcing the creation of the surface.
  NoOpDraw();

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

  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight());
  host_.PrepareDraw(0xFF050505);

  Draw();

  pb_print("%s\n", name);
  pb_draw_text_screen();
  host_.FinishDraw(allow_saving_, output_dir_, name);
}

void AntialiasingTests::TestAAOnThenOffThenCPUWrite() {
  host_.PrepareDraw(0xFF050505);

  // Configure the framebuffer surface with some anti-aliasing mode, clear, change the mode, modify it by writing
  // directly to VRAM, then render.

  {
    // NOTE: Hardware will assert with a limit error if the pitch is not sufficiently large to accommodate the anti
    // aliasing mode increase (for both color and zeta, even if zeta is not being written to).
    //
    // The actual backbuffer needs to be used in order to display the test results, but pbkit does not allocate
    // sufficient memory for fullscreen AA. Therefore a nop draw is performed with a reduced size in order to force
    // xemu to create the surface without asserting on an oversize error.
    static constexpr uint32_t kRenderSize = 256;
    static constexpr uint32_t kAAMultiplier = 2;
    const uint32_t kAAFramebufferPitch = kRenderSize * 4 * kAAMultiplier;
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kAAFramebufferPitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kAAFramebufferPitch));
    pb_end(p);

    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, kRenderSize, kRenderSize, false, 0, 0, 0,
                                    0, TestHost::AA_CENTER_CORNER_2);
    NoOpDraw();
  }

  // Reset the surface to the normal backbuffer and do a CPU rendering into it.
  {
    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, host_.GetFramebufferWidth(),
                                    host_.GetFramebufferHeight());
    const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    pb_end(p);
    WaitForGPU();
    GenerateRGBACheckerboard(pb_back_buffer(), 0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight(),
                             kFramebufferPitch, kCheckerboardA, 0xFF5555FF, kCheckerSize);
  }

  // pbkit's text drawing routines use the 3D pipeline which causes xemu to recreate the surface and masks the bug.
  //  pb_print("%s\n", kOnOffCPUWrite);
  //  pb_draw_text_screen();
  host_.FinishDraw(allow_saving_, output_dir_, kOnOffCPUWrite);
}

void AntialiasingTests::TestModifyNonFramebufferSurface() {
  host_.PrepareDraw(0xFF050505);

  // This test verifies that setting the color target to something other than the framebuffer does not modify the
  // behavior of the framebuffer.
  //
  // 1. Configure the framebuffer surface normally (do a nop draw to force xemu to create a surface)

  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight());
  NoOpDraw();

  // 2. Configure some other surface (do a nop draw to force xemu to create a surface)
  {
    // Hardware will assert with a limit error if the pitch is not sufficiently large to accommodate the anti aliasing
    // mode increase. Technically this should be based off of the AA mode, but in practice it's fine to use the max
    // value.
    static constexpr uint32_t anti_aliasing_multiplier = 4;

    const auto kTextureMemory = reinterpret_cast<uint32_t>(host_.GetTextureMemoryForStage(0));
    const uint32_t kRenderBufferPitch = kTextureSize * 4 * anti_aliasing_multiplier;
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kRenderBufferPitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kRenderBufferPitch));
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(kTextureMemory));
    pb_end(p);

    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, kTextureSize, kTextureSize, false, 0, 0,
                                    0, 0, TestHost::AA_CENTER_CORNER_2);
    NoOpDraw();
  }

  // 3. Do a CPU copy to the framebuffer
  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
  WaitForGPU();
  GenerateRGBACheckerboard(pb_back_buffer(), 0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight(),
                           kFramebufferPitch, 0xFF008080, kCheckerboardB, kCheckerSize);

  // pbkit's text drawing routines use the 3D pipeline which causes xemu to recreate the surface and masks the bug.
  //  pb_print("%s\n", kOnOffCPUWrite);
  //  pb_draw_text_screen();

  // Expected behavior is that the framebuffer looks normal, regardless of the fact that the unused texture surface is
  // set up as AA.
  host_.FinishDraw(allow_saving_, output_dir_, kModifyNonFramebufferSurface);

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, 0);
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    pb_end(p);
  }
}

void AntialiasingTests::TestFramebufferIsIndependentOfSurface() {
  host_.PrepareDraw(0xFF050505);

  // This test verifies that setting the color target for the framebuffer is irrelevant when direct CPU writes are done,
  // the framebuffer configuration is entirely handled by AvSetDisplayMode.

  // 1. Configure the framebuffer surface to use a non-standard pitch and size, then render to it.
  {
    // Hardware will assert with a limit error if the pitch is not sufficiently large to accommodate the anti aliasing
    // mode increase. Technically this should be based off of the AA mode, but in practice it's fine to use the max
    // value.
    static constexpr uint32_t anti_aliasing_multiplier = 4;

    const auto kTextureMemory = reinterpret_cast<uint32_t>(host_.GetTextureMemoryForStage(0));
    const uint32_t kRenderBufferPitch = kTextureSize * 4 * anti_aliasing_multiplier;
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kRenderBufferPitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kRenderBufferPitch));
    pb_end(p);

    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, kTextureSize, kTextureSize, false, 0, 0,
                                    0, 0, TestHost::AA_CENTER_CORNER_2);

    host_.Begin(TestHost::PRIMITIVE_TRIANGLES);
    host_.SetDiffuse(0xFFFF00FF);
    host_.SetVertex(0.0f, kTextureSize / 2.0f, 0.1f, 1.0f);
    host_.SetVertex(kTextureSize, 0.0f, 0.1f, 1.0f);
    host_.SetVertex(kTextureSize, kTextureSize, 0.1f, 1.0f);
    host_.End();
  }

  // 2. Do a CPU copy to the framebuffer
  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
  WaitForGPU();
  GenerateRGBACheckerboard(pb_back_buffer(), 0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight(),
                           kFramebufferPitch, 0xFF222222, 0xFF88AA00, kCheckerSize);

  // Expected behavior is that the framebuffer looks normal, regardless of the fact that it was a 3d draw target with
  // params that do not matc AvSetDisplayMode.
  host_.FinishDraw(allow_saving_, output_dir_, kFramebufferIsIndependent);

  // Restore the framebuffer surface format for future tests.
  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight());
}

void AntialiasingTests::TestCPUWriteIgnoresSurfaceConfig() {
  host_.PrepareDraw(0xFF050505);

  // This test verifies that direct writes to VRAM bypass any surface configuration and must not be interpreted using
  // the current xemu SurfaceBinding.

  // Configure a surface pointing at texture memory and do a no-op draw to force xemu to create a SurfaceBinding.
  {
    // Hardware will assert with a limit error if the pitch is not sufficiently large to accommodate the anti aliasing
    // mode increase. Technically this should be based off of the AA mode, but in practice it's fine to use the max
    // value.
    static constexpr uint32_t anti_aliasing_multiplier = 4;

    const auto kTextureMemory = reinterpret_cast<uint32_t>(host_.GetTextureMemoryForStage(0));
    const uint32_t kRenderBufferPitch = kTextureSize * 4 * anti_aliasing_multiplier;
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kRenderBufferPitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kRenderBufferPitch));
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(kTextureMemory));
    pb_end(p);

    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, kTextureSize, kTextureSize, false, 0, 0,
                                    0, 0, TestHost::AA_SQUARE_OFFSET_4);
    NoOpDraw();
  }

  // Do a CPU copy to texture memory, ignoring the AA setting.
  WaitForGPU();
  GenerateRGBACheckerboard(host_.GetTextureMemoryForStage(0), 0, 0, kTextureSize, kTextureSize, kTextureSize * 4,
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

  pb_print("%s\n", kCPUWriteIgnoresSurfaceConfig);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, kCPUWriteIgnoresSurfaceConfig);
}

void AntialiasingTests::TestGPUAAWriteAfterCPUWrite() {
  host_.PrepareDraw(0xFF050505);

  // This test checks the behavior of doing a GPU-based write to an anti-aliased surface that was initialized via a
  // CPU-based blit.

  // Configure a surface pointing at texture memory and do a no-op draw to force xemu to create a SurfaceBinding.
  {
    static constexpr uint32_t anti_aliasing_multiplier = 4;
    const auto kTextureMemory = reinterpret_cast<uint32_t>(host_.GetTextureMemoryForStage(0));
    const uint32_t kRenderBufferPitch = kTextureSize * 4 * anti_aliasing_multiplier;
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kRenderBufferPitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kRenderBufferPitch));
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(kTextureMemory));
    pb_end(p);

    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, kTextureSize, kTextureSize, false, 0, 0,
                                    0, 0, TestHost::AA_SQUARE_OFFSET_4);
    NoOpDraw();
  }

  // Do a CPU copy to texture memory, ignoring the AA setting.
  WaitForGPU();
  GenerateRGBACheckerboard(host_.GetTextureMemoryForStage(0), 0, 0, kTextureSize, kTextureSize, kTextureSize * 4,
                           0xFF333333, 0xFFEEAA33, kCheckerSize);

  // Do a GPU-based draw to the texture memory (respecting the AA setting)
  {
    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
    host_.SetTextureStageEnabled(0, false);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE);

    host_.Begin(TestHost::PRIMITIVE_TRIANGLES);
    host_.SetDiffuse(0xFFDDCC00);
    host_.SetVertex(5.0f, kTextureSize / 2.0f, 0.1f, 1.0f);
    host_.SetVertex(kTextureSize - 5.0f, 5.0f, 0.1f, 1.0f);
    host_.SetVertex(kTextureSize - 5.0f, kTextureSize - 5.0f, 0.1f, 1.0f);
    host_.End();
  }

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

  // Draw a quad with the texture. Note that the texture configuration used in the draw routine assumes a
  // non-antialiased pitch, so the GPU render should result in lines of pixels with gaps (as the AA pitch is > the
  // texture pitch).
  Draw();

  pb_print("%s\n", kGPUAAWriteAfterCPUWrite);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, kGPUAAWriteAfterCPUWrite);
}

void AntialiasingTests::Draw() const {
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

void AntialiasingTests::NoOpDraw() const {
  host_.Begin(TestHost::PRIMITIVE_TRIANGLES);
  host_.SetVertex(0.0f, 0.0f, 0.1f, 1.0f);
  host_.SetVertex(0.0f, 0.0f, 0.1f, 1.0f);
  host_.SetVertex(0.0f, 0.0f, 0.1f, 1.0f);
  host_.End();
}

void AntialiasingTests::WaitForGPU() const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_NO_OPERATION, 0);
  p = pb_push1(p, NV097_WAIT_FOR_IDLE, 0);
  pb_end(p);
}
