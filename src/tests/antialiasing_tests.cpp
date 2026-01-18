#include "antialiasing_tests.h"

#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "configure.h"
#include "shaders/passthrough_vertex_shader.h"
#include "test_host.h"
#include "texture_generator.h"

// From pbkit.c, DMA_A is set to channel 3 by default
// NV097_SET_CONTEXT_DMA_A == NV20_TCL_PRIMITIVE_3D_SET_OBJECT1
static constexpr uint32_t kDefaultDMAChannelA = 3;
// From pbkit.c, DMA_COLOR is set to channel 9 by default.
// NV097_SET_CONTEXT_DMA_COLOR == NV20_TCL_PRIMITIVE_3D_SET_OBJECT3
static constexpr uint32_t kDefaultDMAColorChannel = 9;
// NV097_SET_CONTEXT_DMA_ZETA is set to channel 10 by default.
static constexpr uint32_t kDefaultDMAZetaChannel = 10;

constexpr uint32_t kCheckerSize = 8;
static constexpr uint32_t kCheckerboardA = 0xFF808080;
static constexpr uint32_t kCheckerboardB = 0xFF3333C0;

static constexpr char kAANone[] = "CreateSurfaceWithCenter1";
static constexpr char kAA2[] = "CreateSurfaceWithCenterCorner2";
static constexpr char kAA4[] = "CreateSurfaceWithSquareOffset4";
static constexpr char kFBAANone[] = "FBSurfaceWithCenter1";
static constexpr char kFBAA2[] = "FBSurfaceWithCenterCorner2";
// static constexpr const char kFBAA4[] = "FBSurfaceWithSquareOffset4";
static constexpr char kOnOffCPUWrite[] = "AAOnThenOffCPUWrite";
static constexpr char kModifyNonFramebufferSurface[] = "SurfaceStatesAreIndependent";
static constexpr char kFramebufferIsIndependent[] = "FramebufferNotModifiedBySurfaceState";
static constexpr char kCPUWriteIgnoresSurfaceConfig[] = "CPUWriteIgnoresSurfaceConfig";
static constexpr char kGPUAAWriteAfterCPUWrite[] = "GPUAAWriteAfterCPUWrite";
static constexpr char kNonAACPURoundTrip[] = "NonAACPURoundTrip";
#ifdef ENABLE_MULTIFRAME_CPU_BLIT_TEST
static constexpr const char kMultiframeCPUBlit[] = "__MultiframeCPUBlit";
#endif

static constexpr uint32_t kTextureSize = 128;

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc AAOnThenOffCPUWrite
 *  Configure the framebuffer surface with some antialiasing mode, clear, change the mode, modify the framebuffer by
 *  writing directly to VRAM, then render. Used to reproduce xemu-project/xemu#652.
 *
 * @tc CPUWriteIgnoresSurfaceConfig
 *  This test verifies that direct writes to VRAM bypass any surface configuration. It sets up a texture surface with
 *  antialiasing enabled, then writes directly to texture memory and renders the texture to the screen. It is expected
 *  that antialiasing has no effect.
 *
 * @tc CreateSurfaceWithCenter1
 *  Tests rendering with NV097_SET_SURFACE_FORMAT_ANTI_ALIASING_CENTER_1
 *
 * @tc CreateSurfaceWithCenterCorner2
 *  Tests rendering with NV097_SET_SURFACE_FORMAT_ANTI_ALIASING_CENTER_CORNER_2
 *
 * @tc CreateSurfaceWithSquareOffset4
 *  Tests rendering with NV097_SET_SURFACE_FORMAT_ANTI_ALIASING_SQUARE_OFFSET_4
 *
 * @tc FBSurfaceWithCenter1
 *
 * @tc FBSurfaceWithCenterCorner2
 *
 * @tc FramebufferNotModifiedBySurfaceState
 *
 * @tc GPUAAWriteAfterCPUWrite
 *
 * @tc NonAACPURoundTrip
 *
 * @tc SurfaceStatesAreIndependent
 *
 */
AntialiasingTests::AntialiasingTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Antialiasing tests", config) {
  tests_[kAANone] = [this]() { Test(kAANone, TestHost::AA_CENTER_1); };
  tests_[kAA2] = [this]() { Test(kAA2, TestHost::AA_CENTER_CORNER_2); };
  tests_[kAA4] = [this]() { Test(kAA4, TestHost::AA_SQUARE_OFFSET_4); };

  tests_[kFBAANone] = [this]() { TestAARenderToFramebufferSurface(kFBAANone, TestHost::AA_CENTER_1); };
  tests_[kFBAA2] = [this]() { TestAARenderToFramebufferSurface(kFBAA2, TestHost::AA_CENTER_CORNER_2); };
  // pbkit doesn't reserve enough framebuffer RAM for this test.
  //  tests_[kFBAA4] = [this]() { TestAARenderToFramebufferSurface(kFBAA4, TestHost::AA_SQUARE_OFFSET_4); };

  tests_[kOnOffCPUWrite] = [this]() { TestAAOnThenOffThenCPUWrite(); };
  tests_[kModifyNonFramebufferSurface] = [this]() { TestModifyNonFramebufferSurface(); };
  tests_[kFramebufferIsIndependent] = [this]() { TestFramebufferIsIndependentOfSurface(); };
  tests_[kCPUWriteIgnoresSurfaceConfig] = [this]() { TestCPUWriteIgnoresSurfaceConfig(); };
  tests_[kGPUAAWriteAfterCPUWrite] = [this]() { TestGPUAAWriteAfterCPUWrite(); };
  tests_[kNonAACPURoundTrip] = [this]() { TestNonAACPURoundTrip(); };

#ifdef ENABLE_MULTIFRAME_CPU_BLIT_TEST
  tests_[kMultiframeCPUBlit] = [this]() { TestMultiframeCPUBlit(); };
#endif
};

void AntialiasingTests::Initialize() {
  TestSuite::Initialize();

  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, false);
  Pushbuffer::Push(NV097_SET_DEPTH_MASK, false);
  Pushbuffer::Push(NV097_SET_STENCIL_TEST_ENABLE, false);
  Pushbuffer::Push(NV097_SET_STENCIL_MASK, false);
  Pushbuffer::End();
}

/**
 * Point the color surface at texture memory, configure the surface with some antialiasing mode, clear, then point the
 * surface back to the framebuffer and render the texture memory.
 */
void AntialiasingTests::Test(const char *name, TestHost::AntiAliasingSetting aa) {
  // Create a texture with an obvious border around it.
  memset(host_.GetTextureMemory(), 0xCC, kTextureSize * kTextureSize * 4);
  GenerateRGBACheckerboard(host_.GetTextureMemory(), 2, 2, kTextureSize - 4, kTextureSize - 4, kTextureSize * 4,
                           kCheckerboardA, kCheckerboardB, kCheckerSize);
  {
    // Hardware will assert with a limit error if the pitch is not sufficiently large to accommodate the antialiasing
    // mode increase. Technically this should be based off of the AA mode, but in practice it's fine to use the max
    // value.
    static constexpr uint32_t anti_aliasing_multiplier = 4;

    const auto kTextureMemory = reinterpret_cast<uint32_t>(host_.GetTextureMemoryForStage(0));
    const uint32_t kRenderBufferPitch = kTextureSize * 4 * anti_aliasing_multiplier;
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kRenderBufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kRenderBufferPitch));
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(kTextureMemory));
    Pushbuffer::End();
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
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, 0);
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    Pushbuffer::End();
  }

  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight());
  host_.PrepareDraw(0xFF050505);

  Draw();

  pb_print("%s\n", name);
  pb_draw_text_screen();
  FinishDraw(name);
}

void AntialiasingTests::TestAARenderToFramebufferSurface(const char *name, TestHost::AntiAliasingSetting aa) {
  // Configure an antialiased surface coincident with the framebuffer and render to it, then display the contents.

  uint32_t aa_multiplier;
  switch (aa) {
    case TestHost::AA_CENTER_1:
      aa_multiplier = 1;
      break;

    case TestHost::AA_CENTER_CORNER_2:
      aa_multiplier = 2;
      break;

    case TestHost::AA_SQUARE_OFFSET_4:
      aa_multiplier = 4;
      break;
  }

  {
    const uint32_t pitch = host_.GetFramebufferWidth() * 4 * aa_multiplier;
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH,
                     SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, pitch) | SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, pitch));
    // Point zeta at an unbounded DMA channel so it won't fail limit checks, it won't be written to anyway.
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_ZETA, kDefaultDMAChannelA);
    Pushbuffer::Push(NV097_SET_SURFACE_ZETA_OFFSET, VRAM_ADDR(host_.GetTextureMemory()));
    Pushbuffer::End();
  }

  host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, host_.GetFramebufferWidth(),
                         host_.GetFramebufferHeight(), false, 0, 0, 0, 0, aa);
  host_.PrepareDraw(0xFF050505);

  {
    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
    host_.SetTextureStageEnabled(0, false);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE);

    static constexpr float inset = 64.0f;
    const float left = inset;
    const float right = host_.GetFramebufferWidthF() - inset;
    const float top = inset;
    const float bottom = host_.GetFramebufferHeightF() - inset;
    const float mid_x = left + (right - left) * 0.5f;
    const float mid_y = top + (bottom - top) * 0.5f;

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0.25f, 0.95f, 0.75f);
    host_.SetVertex(left, mid_y, 0.1f, 1.0f);
    host_.SetVertex(mid_x, top, 0.1f, 1.0f);
    host_.SetVertex(right, mid_y, 0.1f, 1.0f);
    host_.SetVertex(mid_x, bottom, 0.1f, 1.0f);
    host_.End();
  }

  pb_print("%s\n", name);
  pb_draw_text_screen();
  FinishDraw(name);

  {
    const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_ZETA, kDefaultDMAZetaChannel);
    Pushbuffer::Push(NV097_SET_SURFACE_ZETA_OFFSET, 0);
    Pushbuffer::End();
  }
  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight());
}

/**
 * Configure the framebuffer surface with some antialiasing mode, clear, change the mode, modify the framebuffer by
 * writing directly to VRAM, then render.
 */
void AntialiasingTests::TestAAOnThenOffThenCPUWrite() {
  host_.PrepareDraw(0xFF050505);

  {
    // NOTE: Hardware will assert with a limit error if the pitch is not sufficiently large to accommodate the anti
    // aliasing mode increase (for both color and zeta, even if zeta is not being written to).
    //
    // The actual backbuffer needs to be used in order to display the test results, but pbkit does not allocate
    // sufficient memory for fullscreen AA. Therefore, a nop draw is performed with a reduced size in order to force
    // xemu to create the surface without asserting on an oversize error.
    static constexpr uint32_t kRenderSize = 256;
    static constexpr uint32_t kAAMultiplier = 2;
    const uint32_t kAAFramebufferPitch = kRenderSize * 4 * kAAMultiplier;
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kAAFramebufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kAAFramebufferPitch));
    Pushbuffer::End();

    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, kRenderSize, kRenderSize, false, 0, 0, 0,
                                    0, TestHost::AA_CENTER_CORNER_2);
    NoOpDraw();
  }

  // Reset the surface to the normal backbuffer and do a CPU rendering into it.
  {
    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, host_.GetFramebufferWidth(),
                                    host_.GetFramebufferHeight());
    const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    Pushbuffer::End();
    WaitForGPU();
    GenerateRGBACheckerboard(pb_back_buffer(), 0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight(),
                             kFramebufferPitch, kCheckerboardA, 0xFF5555FF, kCheckerSize);
  }

  // pbkit's text drawing routines use the 3D pipeline which causes xemu to recreate the surface and masks the bug.
  //  pb_print("%s\n", kOnOffCPUWrite);
  //  pb_draw_text_screen();
  FinishDraw(kOnOffCPUWrite);
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
    // Hardware will assert with a limit error if the pitch is not sufficiently large to accommodate the antialiasing
    // mode increase. Technically this should be based off of the AA mode, but in practice it's fine to use the max
    // value.
    static constexpr uint32_t anti_aliasing_multiplier = 4;

    const auto kTextureMemory = reinterpret_cast<uint32_t>(host_.GetTextureMemoryForStage(0));
    const uint32_t kRenderBufferPitch = kTextureSize * 4 * anti_aliasing_multiplier;
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kRenderBufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kRenderBufferPitch));
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(kTextureMemory));
    Pushbuffer::End();

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
  FinishDraw(kModifyNonFramebufferSurface);

  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, 0);
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    Pushbuffer::End();
  }
}

void AntialiasingTests::TestFramebufferIsIndependentOfSurface() {
  host_.PrepareDraw(0xFF050505);

  // This test verifies that setting the color target for the framebuffer is irrelevant when direct CPU writes are done,
  // the framebuffer configuration is entirely handled by AvSetDisplayMode.

  // 1. Configure the framebuffer surface to use a non-standard pitch and size, then render to it.
  {
    // Hardware will assert with a limit error if the pitch is not sufficiently large to accommodate the antialiasing
    // mode increase. Technically this should be based off of the AA mode, but in practice it's fine to use the max
    // value.
    static constexpr uint32_t anti_aliasing_multiplier = 4;

    const uint32_t kRenderBufferPitch = kTextureSize * 4 * anti_aliasing_multiplier;
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kRenderBufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kRenderBufferPitch));
    Pushbuffer::End();

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
  FinishDraw(kFramebufferIsIndependent);

  // Restore the framebuffer surface format for future tests.
  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight());
}

/**
 * This test verifies that direct writes to VRAM bypass any surface configuration and must not be interpreted using the
 * current xemu SurfaceBinding.
 */
void AntialiasingTests::TestCPUWriteIgnoresSurfaceConfig() {
  host_.PrepareDraw(0xFF050505);

  // Configure a surface pointing at texture memory and do a no-op draw to force xemu to create a SurfaceBinding.
  {
    // Hardware will assert with a limit error if the pitch is not sufficiently large to accommodate the antialiasing
    // mode increase. Technically this should be based off of the AA mode, but in practice it's fine to use the max
    // value.
    static constexpr uint32_t anti_aliasing_multiplier = 4;

    const auto kTextureMemory = reinterpret_cast<uint32_t>(host_.GetTextureMemoryForStage(0));
    const uint32_t kRenderBufferPitch = kTextureSize * 4 * anti_aliasing_multiplier;
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kRenderBufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kRenderBufferPitch));
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(kTextureMemory));
    Pushbuffer::End();

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
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, 0);
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    Pushbuffer::End();
  }

  // Restore the surface format to allow the texture to be rendered.
  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight());

  // Draw a quad with the texture.
  Draw();

  pb_print("%s\n", kCPUWriteIgnoresSurfaceConfig);
  pb_draw_text_screen();

  FinishDraw(kCPUWriteIgnoresSurfaceConfig);
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
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kRenderBufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kRenderBufferPitch));
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(kTextureMemory));
    Pushbuffer::End();

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
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, 0);
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    Pushbuffer::End();
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

  FinishDraw(kGPUAAWriteAfterCPUWrite);
}

/**
 * This test verifies that nop draws to a CPU blitted surface are truly nops.
 */
void AntialiasingTests::TestNonAACPURoundTrip() {
  host_.PrepareDraw(0xFF050505);

  // Configure a surface pointing at texture memory and do a no-op draw to force xemu to create a SurfaceBinding.
  // In this case, antialiasing is left off to verify that xemu's behavior is correct when dealing with a pitch that
  // does not match the content.
  {
    static constexpr uint32_t anti_aliasing_multiplier = 4;
    const auto kTextureMemory = reinterpret_cast<uint32_t>(host_.GetTextureMemoryForStage(0));
    const uint32_t kRenderBufferPitch = kTextureSize * 4 * anti_aliasing_multiplier;
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kRenderBufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kRenderBufferPitch));
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(kTextureMemory));
    Pushbuffer::End();

    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, kTextureSize, kTextureSize);
    NoOpDraw();
  }

  // Do a CPU copy to texture memory.
  WaitForGPU();
  GenerateRGBACheckerboard(host_.GetTextureMemoryForStage(0), 0, 0, kTextureSize, kTextureSize, kTextureSize * 4,
                           0xFF336666, 0xFF33FF33, kCheckerSize);

  // Point the output surface back at the framebuffer.
  {
    const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, 0);
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    Pushbuffer::End();
  }

  // Restore the surface format to allow the texture to be rendered.
  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight());

  // Draw a quad with the texture. Note that the texture configuration used in the draw routine assumes a
  // non-antialiased pitch, so the GPU render should result in lines of pixels with gaps (as the AA pitch is > the
  // texture pitch).
  Draw();

  pb_print("%s\n", kNonAACPURoundTrip);
  pb_draw_text_screen();

  FinishDraw(kNonAACPURoundTrip);
}

#ifdef ENABLE_MULTIFRAME_CPU_BLIT_TEST
void AntialiasingTests::TestMultiframeCPUBlit() {
  static constexpr uint32_t kNumFrames = 120;
  static constexpr uint32_t kColors[][2] = {
      {0xFF000000, 0xFFCCCCCC}, {0xFFFF3333, 0xFF000000}, {0xFF000000, 0xFF3333FF},
      {0xFF33FF33, 0xFF000000}, {0xFF000000, 0xFFCC44CC}, {0xFFCCCC44, 0xFF000000},
  };

  const auto width = host_.GetFramebufferWidth();
  const auto height = host_.GetFramebufferHeight();
  const auto kFramebufferPitch = width * 4;

  for (auto i = 0; i < kNumFrames; ++i) {
    host_.PrepareDraw(0xFF050505);

    // Configure a surface pointing at the current framebuffer and do a no-op draw to force xemu to create a
    // SurfaceBinding with a sparse pitch.
    {
      static constexpr uint32_t anti_aliasing_multiplier = 2;
      const uint32_t kRenderBufferPitch = kTextureSize * 4 * anti_aliasing_multiplier;
      Pushbuffer::Begin();
      Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kRenderBufferPitch) |
                                                    SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kRenderBufferPitch));
      Pushbuffer::End();

      host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, kTextureSize, kTextureSize, false, 0,
                                      0, 0, 0, TestHost::AA_CENTER_CORNER_2);
      NoOpDraw();
    }

    // Do a CPU copy to texture memory, ignoring the AA setting.
    WaitForGPU();
    auto color = kColors[i % (sizeof(kColors) / sizeof(kColors[0]))];
    GenerateRGBACheckerboard(pb_back_buffer(), 0, 0, width, height, kFramebufferPitch, color[0], color[1], 24);

    FinishDrawNoSave(kMultiframeCPUBlit);
  }

  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    Pushbuffer::End();
  }
  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight());
}
#endif  // ENABLE_MULTIFRAME_CPU_BLIT_TEST

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
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_NO_OPERATION, 0);
  Pushbuffer::Push(NV097_WAIT_FOR_IDLE, 0);
  Pushbuffer::End();
}
