#include "surface_clip_tests.h"

#include "shaders/passthrough_vertex_shader.h"

#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

// From pbkit.c, DMA_A is set to channel 3 by default
// NV097_SET_CONTEXT_DMA_A == NV20_TCL_PRIMITIVE_3D_SET_OBJECT1
static constexpr uint32_t kDefaultDMAChannelA = 3;

// From pbkit.c, DMA_COLOR is set to channel 9 by default
static constexpr uint32_t kDefaultDMAColorChannel = 9;

static constexpr float kTestRectThickness = 4.0f;

struct NamedSurfaceFormat {
  const char *suffix;
  TestHost::SurfaceColorFormat format;
};

// clang-format off
static constexpr SurfaceClipTests::ClipRect kTestRects[] = {
    {0, 0, 0, 0},
    {0, 0, 512, 0},
    {0, 0, 0, 384},
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

  // Halo2 multiplayer 2-player
  {0,240,640,240},
  // Halo2 multiplayer 4-player
  {320, 240,320,240},
 };

static constexpr NamedSurfaceFormat kSurfaceFormats[] {
  {"", TestHost::SCF_R5G6B5},
  {"_A8R8G8B8", TestHost::SCF_A8R8G8B8},
  {"_B8", TestHost::SCF_B8},
  {"_G8B8", TestHost::SCF_G8B8},
};
// clang-format on

static constexpr char kXemuBug420Test[] = "XemuBug420";
static constexpr char kTestDebugTextIsClipped[] = "DebugTextShouldClip";

static std::string MakeTestName(bool render_target, const SurfaceClipTests::ClipRect &rect) {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%sx%dy%d_w%dh%d", render_target ? "rt_" : "", rect.x, rect.y, rect.width,
           rect.height);
  return buffer;
}

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc DebugTextShouldClip
 *  Sets a tiny clip area and renders debug text. No text should be visible.
 *
 * @tc XemuBug420
 *  Reproduction case for xemu#420. A 32-bit surface is created, then immediately changed to a 16-bit one, followed by a
 *  region clear. This caused an assertion in versions before xemu#919
 *
 * @tc rt_x0y0_w0h0
 *  Configures a texture target as R5G6B5 and sets the clip region to 0,0 0x0, then clears the clipped region and
 *  draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary. The texture is then rendered into an 8888 backbuffer.
 *
 * @tc rt_x0y0_w512h0
 *  Configures a texture target as R5G6B5 and sets the clip region to 0,0 512x0, then clears the clipped region and
 *  draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary. The texture is then rendered into an 8888 backbuffer.
 *
 * @tc rt_x0y0_w0h384
 *  Configures a texture target as R5G6B5 and sets the clip region to 0,0 0x384, then clears the clipped region and
 *  draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary. The texture is then rendered into an 8888 backbuffer.
 *
 * @tc rt_x0y0_w512h384
 *  Configures a texture target as R5G6B5 and sets the clip region to 0,0 512x384, then clears the clipped region and
 *  draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary. The texture is then rendered into an 8888 backbuffer. No red should be seen,
 *  and the light green quads should be fully visible.
 *
 * @tc rt_x0y0_w640h480
 *  Configures a texture target as R5G6B5 and sets the clip region to 0,0 640x480, then clears the clipped region and
 *  draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary.The texture is then rendered into an 8888 backbuffer. No red should be seen,
 *  and the light green quads should be fully visible.
 *
 * @tc rt_x16y8_w512h384
 *  Configures a texture target as R5G6B5 and sets the clip region to 16,8 512x384, then clears the clipped region and
 *  draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary. The texture is then rendered into an 8888 backbuffer. No red should be seen,
 *  and the light green quads should be fully visible.
 *
 * @tc rt_x8y16_w632h464
 *  Configures a texture target as R5G6B5 and sets the clip region to 8,16 632x464, then clears the clipped region and
 *  draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary. The texture is then rendered into an 8888 backbuffer. No red should be seen,
 *  and the light green quads should be fully visible.
 *
 * @tc rt_x0y240_w640h240
 *  Configures a texture target as R5G6B5 and sets the clip region to 0,240 640x240, then clears the clipped region and
 *  draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary. The texture is then rendered into an 8888 backbuffer. No red should be seen,
 *  and the light green quads should be fully visible.
 *
 * @tc rt_x320y240_w320h240
 *  Configures a texture target as R5G6B5 and sets the clip region to 320,240 320x240, then clears the clipped region
 *  and draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary. The texture is then rendered into an 8888 backbuffer. No red should be seen,
 *  and the light green quads should be fully visible.
 *
 * @tc x0y0_w0h0
 *  Configures the backbuffer as R5G6B5 and sets the clip region to 0,0 0x0, then clears the clipped region and
 *  draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary.
 *  Because the format is 565, the colors are shifted from red -> green, light green -> light pink, and dark green ->
 *  pink.
 *
 * @tc x0y0_w512h0
 *  Configures the backbuffer as R5G6B5 and sets the clip region to 0,0 512x0, then clears the clipped region and
 *  draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary.
 *  Because the format is 565, the colors are shifted from red -> green, light green -> light pink, and dark green ->
 *  pink.
 *
 * @tc x0y0_w0h384
 *  Configures the backbuffer as R5G6B5 and sets the clip region to 0,0 0x384, then clears the clipped region and
 *  draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary.
 *  Because the format is 565, the colors are shifted from red -> green, light green -> light pink, and dark green ->
 *  pink.
 *
 * @tc x0y0_w512h384
 *  Configures the backbuffer as R5G6B5 and sets the clip region to 0,0 512x384, then clears the clipped region and
 *  draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary. No red should be seen, and the light green quads should be fully visible.
 *  Because the format is 565, the colors are shifted from red -> green, light green -> light pink, and dark green ->
 *  pink.
 *
 * @tc x0y0_w640h480
 *  Configures the backbuffer as R5G6B5 and sets the clip region to 0,0 640x480, then clears the clipped region and
 *  draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary. No red should be seen, and the light green quads should be fully visible.
 *  Because the format is 565, the colors are shifted from red -> green, light green -> light pink, and dark green ->
 *  pink.
 *
 * @tc x16y8_w512h384
 *  Configures the backbuffer as R5G6B5 and sets the clip region to 16,8 512x384, then clears the clipped region and
 *  draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary. No red should be seen, and the light green quads should be fully visible.
 *  Because the format is 565, the colors are shifted from red -> green, light green -> light pink, and dark green ->
 *  pink.
 *
 * @tc x8y16_w632h464
 *  Configures the backbuffer as R5G6B5 and sets the clip region to 8,16 632x464, then clears the clipped region and
 *  draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary. No red should be seen, and the light green quads should be fully visible.
 *  Because the format is 565, the colors are shifted from red -> green, light green -> light pink, and dark green ->
 *  pink.
 *
 * @tc x0y240_w640h240
 *  Configures the backbuffer as R5G6B5 and sets the clip region to 0,240 640x240, then clears the clipped region and
 *  draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary. No red should be seen, and the light green quads should be fully visible.
 *  Because the format is 565, the colors are shifted from red -> green, light green -> light pink, and dark green ->
 *  pink.
 *
 * @tc x320y240_w320h240
 *  Configures the backbuffer as R5G6B5 and sets the clip region to 320,240 320x240, then clears the clipped region and
 *  draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary. No red should be seen, and the light green quads should be fully visible.
 *  Because the format is 565, the colors are shifted from red -> green, light green -> light pink, and dark green ->
 *  pink.
 *
 * @tc x0y0_w0h0_A8R8G8B8
 *  Configures the backbuffer as A8R8G8B8 and sets the clip region to 0,0 0x0, then clears the clipped region and
 *  draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary. No geometry should be seen, since width and height of the clip region are 0.
 *
 * @tc x0y0_w512h0_A8R8G8B8
 *  Configures the backbuffer as A8R8G8B8 and sets the clip region to 0,0 512x0, then clears the clipped region and
 *  draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary. No geometry should be seen, since width and height of the clip region are 0.
 *
 * @tc x0y0_w0h384_A8R8G8B8
 *  Configures the backbuffer as A8R8G8B8 and sets the clip region to 0,0 0x384, then clears the clipped region and
 *  draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary. No geometry should be seen, since width and height of the clip region are 0.
 *
 * @tc x0y0_w512h384_A8R8G8B8
 *  Configures the backbuffer as A8R8G8B8 and sets the clip region to 0,0 512x384, then clears the clipped region and
 *  draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary. No red should be seen, and the light green quads should be fully visible.
 *
 * @tc x0y0_w640h480_A8R8G8B8
 *  Configures the backbuffer as A8R8G8B8 and sets the clip region to 0,0 640x480, then clears the clipped region and
 *  draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary. No red should be seen, and the light green quads should be fully visible.
 *
 * @tc x16y8_w512h384_A8R8G8B8
 *  Configures the backbuffer as A8R8G8B8 and sets the clip region to 16,8 512x384, then clears the clipped region and
 *  draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary. No red should be seen, and the light green quads should be fully visible.
 *
 * @tc x8y16_w632h464_A8R8G8B8
 *  Configures the backbuffer as A8R8G8B8 and sets the clip region to 8,16 632x464, then clears the clipped region and
 *  draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary. No red should be seen, and the light green quads should be fully visible.
 *
 * @tc x0y240_w640h240_A8R8G8B8
 *  Configures the backbuffer as A8R8G8B8 and sets the clip region to 0,240 640x240, then clears the clipped region and
 *  draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary. No red should be seen, and the light green quads should be fully visible.
 *
 * @tc x320y240_w320h240_A8R8G8B8
 *  Configures the backbuffer as A8R8G8B8 and sets the clip region to 320,240 320x240, then clears the clipped region
 *  and draws 4 red quads just outside the clip region, a dark green quad 1 pixel within the clip region, and 4 lighter
 *  green quads along the clip boundary. No red should be seen, and the light green quads should be fully visible.
 */
SurfaceClipTests::SurfaceClipTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Surface clip", config) {
  for (auto &rect : kTestRects) {
    for (auto &format : kSurfaceFormats) {
      auto name = MakeTestName(false, rect) + format.suffix;
      tests_[name] = [this, &rect, name, &format]() { Test(name, rect, format.format); };
    }

    auto name = MakeTestName(true, rect);
    tests_[name] = [this, &rect, name]() { TestRenderTarget(name, rect); };
  }

  tests_[kXemuBug420Test] = [this]() { TestXemuBug420(); };

  tests_[kTestDebugTextIsClipped] = [this]() { TestDebugTextIsClipped(); };
}

void SurfaceClipTests::Initialize() {
  TestSuite::Initialize();
  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  // Failing to disable alpha blending on B8 and G8B8 will trigger a hardware exception.
  host_.SetBlend(false);
}

void SurfaceClipTests::Test(const std::string &name, const ClipRect &rect, TestHost::SurfaceColorFormat color_format) {
  host_.PrepareDraw(0xFC111155);

  host_.SetSurfaceFormatImmediate(color_format, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight(), false, rect.x, rect.y, rect.width, rect.height);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_SURFACE_CLIP_HORIZONTAL, (rect.width << 16) + rect.x);
  Pushbuffer::Push(NV097_SET_SURFACE_CLIP_VERTICAL, (rect.height << 16) + rect.y);
  Pushbuffer::End();

  // Note: Clearing the depth stencil in B8 and G8B8 formats will result in a zeta limit exception.
  // host_.ClearDepthStencilRegion(0xFFFFFF, 0x0, rect.x, rect.y, rect.width, rect.height);

  DrawTestImage(rect);

  host_.PBKitBusyWait();
  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight());
  pb_print("%s", name.c_str());
  pb_draw_text_screen();

  host_.SetSurfaceFormatImmediate(color_format, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight(), false, rect.x, rect.y, rect.width, rect.height);
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_SURFACE_CLIP_HORIZONTAL, (rect.width << 16) + rect.x);
  Pushbuffer::Push(NV097_SET_SURFACE_CLIP_VERTICAL, (rect.height << 16) + rect.y);
  Pushbuffer::End();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
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
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_SURFACE_CLIP_HORIZONTAL, (rect.width << 16) + rect.x);
  Pushbuffer::Push(NV097_SET_SURFACE_CLIP_VERTICAL, (rect.height << 16) + rect.y);
  Pushbuffer::End();

  // This triggers an assertion failure:
  // `(pg->color_binding->width == pg->zeta_binding->width) && (pg->color_binding->height == pg->zeta_binding->height)'
  host_.ClearDepthStencilRegion(0xFFFFFF, 0x0, rect.x, rect.y, rect.width, rect.height);

  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight());
  pb_print("%s", kXemuBug420Test);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kXemuBug420Test);

  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight());
}

void SurfaceClipTests::TestDebugTextIsClipped() {
  host_.PrepareDraw(0xFC111155);

  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight(), false, 0.f, 240.f, 640.f, 240.f);

  host_.ClearDepthStencilRegion(0xFFFFFF, 0x0);

  pb_printat(0, 0, "%s", kTestDebugTextIsClipped);
  pb_printat(1, 0, "This text should be clipped and invisible.");
  pb_printat(12, 0, "%s", kTestDebugTextIsClipped);
  pb_printat(13, 0, "The text above midscreen should be clipped and invisible.");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kTestDebugTextIsClipped);
  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight());
}

void SurfaceClipTests::TestRenderTarget(const std::string &name, const ClipRect &rect) {
  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;

  // Point the color buffer at the texture memory.
  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, reinterpret_cast<uint32_t>(host_.GetTextureMemory()) & 0x03FFFFFF);

    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, host_.GetFramebufferWidth() * 2) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));

    // TODO: Investigate if this is actually necessary. Morrowind does this after changing offsets.
    Pushbuffer::Push(NV097_NO_OPERATION, 0);
    Pushbuffer::Push(NV097_WAIT_FOR_IDLE, 0);
    Pushbuffer::End();

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
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_CLIP_HORIZONTAL, (rect.width << 16) + rect.x);
    Pushbuffer::Push(NV097_SET_SURFACE_CLIP_VERTICAL, (rect.height << 16) + rect.y);
    Pushbuffer::End();
  }

  host_.PrepareDraw();

  host_.CommitSurfaceFormat();
  host_.ClearDepthStencilRegion(0xFFFFFF, 0x0, rect.x, rect.y, rect.width, rect.height);

  DrawTestImage(rect);

  // Restore the color buffer and render the texture to a quad.
  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, 0);
    Pushbuffer::End();
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

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
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
