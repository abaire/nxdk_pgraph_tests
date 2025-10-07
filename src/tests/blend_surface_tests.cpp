#include "blend_surface_tests.h"

#include "texture_generator.h"

struct SurfaceFormatBlendTest {
  const char *name;
  TestHost::SurfaceColorFormat format;
};

static constexpr struct SurfaceFormatBlendTest kSurfaceFormatBlendTests[] = {
    {"X_Z1RGB5", TestHost::SCF_X1R5G5B5_Z1R5G5B5},
    {"X_O1RGB5", TestHost::SCF_X1R5G5B5_O1R5G5B5},
    {"R5G6B5", TestHost::SCF_R5G6B5},
    {"X_ZRGB8", TestHost::SCF_X8R8G8B8_Z8R8G8B8},
    {"X_ORGB8", TestHost::SCF_X8R8G8B8_O8R8G8B8},
    {"XA_Z1A7RGB8", TestHost::SCF_X1A7R8G8B8_Z1A7R8G8B8},
    {"XA_O1A7RGB8", TestHost::SCF_X1A7R8G8B8_O1A7R8G8B8},
    {"ARGB8", TestHost::SCF_A8R8G8B8},
};

struct BlendConfig {
  const char *name;
  uint32_t func;
  uint32_t source_factor;
  uint32_t destination_factor;
};

static constexpr struct BlendConfig kBlendConfigs[] = {
    {"Add_SrcA_1-SrcA", NV097_SET_BLEND_EQUATION_V_FUNC_ADD, NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA,
     NV097_SET_BLEND_FUNC_DFACTOR_V_ONE_MINUS_SRC_ALPHA},
    {"Add_SrcA_DstA", NV097_SET_BLEND_EQUATION_V_FUNC_ADD, NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA,
     NV097_SET_BLEND_FUNC_SFACTOR_V_DST_ALPHA},
};

static constexpr uint32_t kTextureSize = 128;

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc DstAlpha_ARGB8
 *   Demonstrates behavior of blend func ADD (DstAlpha) (Zero) with surface mode
 *   NV097_SET_SURFACE_FORMAT_COLOR_LE_A8R8G8B8. The background is initialized to various colors (see labels) via a
 *   DIFFUSE quad render. A white quad is then blended on top and the final composition rendered to the backbuffer with
 *   alpha forced to 1.0 to display the effect on the color channels.
 *
 * @tc DstAlpha_R5G6B5
 *   Demonstrates behavior of blend func ADD (DstAlpha) (Zero) with surface mode
 *   NV097_SET_SURFACE_FORMAT_COLOR_LE_R5G6B5. The background is initialized to various colors (see labels) via a
 *   DIFFUSE quad render. A white quad is then blended on top and the final composition rendered to the backbuffer with
 *   alpha forced to 1.0 to display the effect on the color channels.
 *
 * @tc DstAlpha_X_O1RGB5
 *   Demonstrates behavior of blend func ADD (DstAlpha) (Zero) with surface mode
 *   NV097_SET_SURFACE_FORMAT_COLOR_LE_X1R5G5B5_O1R5G5B5. The background is initialized to various colors (see labels)
 *   via a DIFFUSE quad render. A white quad is then blended on top and the final composition rendered to the backbuffer
 *   with alpha forced to 1.0 to display the effect on the color channels.
 *
 * @tc DstAlpha_X_ORGB8
 *   Demonstrates behavior of blend func ADD (DstAlpha) (Zero) with surface mode
 *   NV097_SET_SURFACE_FORMAT_COLOR_LE_X8R8G8B8_O8R8G8B8. The background is initialized to various colors (see labels)
 *   via a DIFFUSE quad render. A white quad is then blended on top and the final composition rendered to the backbuffer
 *   with alpha forced to 1.0 to display the effect on the color channels.
 *
 * @tc DstAlpha_X_Z1RGB5
 *   Demonstrates behavior of blend func ADD (DstAlpha) (Zero) with surface mode
 *   NV097_SET_SURFACE_FORMAT_COLOR_LE_X1R5G5B5_Z1R5G5B5. The background is initialized to various colors (see labels)
 *   via a DIFFUSE quad render. A white quad is then blended on top and the final composition rendered to the backbuffer
 *   with alpha forced to 1.0 to display the effect on the color channels.
 *
 * @tc DstAlpha_X_ZRGB8
 *   Demonstrates behavior of blend func ADD (DstAlpha) (Zero) with surface mode
 *   NV097_SET_SURFACE_FORMAT_COLOR_LE_X8R8G8B8_Z8R8G8B8. The background is initialized to various colors (see labels)
 *   via a DIFFUSE quad render. A white quad is then blended on top and the final composition rendered to the backbuffer
 *   with alpha forced to 1.0 to display the effect on the color channels.
 *
 * @tc DstAlpha_XA_O1A7RGB8
 *   Demonstrates behavior of blend func ADD (DstAlpha) (Zero) with surface mode
 *   NV097_SET_SURFACE_FORMAT_COLOR_LE_X1A7R8G8B8_O1A7R8G8B8. The background is initialized to various colors (see
 *   labels) via a DIFFUSE quad render. A white quad is then blended on top and the final composition rendered to the
 *   backbuffer with alpha forced to 1.0 to display the effect on the color channels.
 *
 * @tc DstAlpha_XA_Z1A7RGB8
 *   Demonstrates behavior of blend func ADD (DstAlpha) (Zero) with surface mode
 *   NV097_SET_SURFACE_FORMAT_COLOR_LE_X1A7R8G8B8_Z1A7R8G8B8. The background is initialized to various colors (see
 *   labels) via a DIFFUSE quad render. A white quad is then blended on top and the final composition rendered to the
 *   backbuffer with alpha forced to 1.0 to display the effect on the color channels.
 *
 * @tc ARGB8_Add_SrcA_1-SrcA
 *  Demonstrates the behavior of blending various texture formats with the surface mode set to
 *  NV097_SET_SURFACE_FORMAT_COLOR_LE_A8R8G8B8 and blend factors {SrcAlpha, 1 - SrcAlpha}.
 *  A test pattern is rendered multiple times, partially overlapping itself. A low opacity grey quad is rendered in the
 *  upper left, then composited against itself multiple times in the upper right. A zero alpha (or zero high nibble for
 *  non-alpha formats) quad is rendered in the lower left and composited against itself in the lower right.
 *
 * @tc ARGB8_Add_SrcA_DstA
 *  Demonstrates the behavior of blending various texture formats with the surface mode set to
 *  NV097_SET_SURFACE_FORMAT_COLOR_LE_A8R8G8B8 and blend factors {SrcAlpha, DstAlpha}.
 *  A test pattern is rendered multiple times, partially overlapping itself. A low opacity grey quad is rendered in the
 *  upper left, then composited against itself multiple times in the upper right. A zero alpha (or zero high nibble for
 *  non-alpha formats) quad is rendered in the lower left and composited against itself in the lower right.
 *
 * @tc R5G6B5_Add_SrcA_1-SrcA
 *  Demonstrates the behavior of blending various texture formats with the surface mode set to
 *  NV097_SET_SURFACE_FORMAT_COLOR_LE_R5G6B5 and blend factors {SrcAlpha, 1 - SrcAlpha}.
 *  A test pattern is rendered multiple times, partially overlapping itself. A low opacity grey quad is rendered in the
 *  upper left, then composited against itself multiple times in the upper right. A zero alpha (or zero high nibble for
 *  non-alpha formats) quad is rendered in the lower left and composited against itself in the lower right.
 *
 * @tc R5G6B5_Add_SrcA_DstA
 *  Demonstrates the behavior of blending various texture formats with the surface mode set to
 *  NV097_SET_SURFACE_FORMAT_COLOR_LE_R5G6B5 and blend factors {SrcAlpha, DstAlpha}.
 *  A test pattern is rendered multiple times, partially overlapping itself. A low opacity grey quad is rendered in the
 *  upper left, then composited against itself multiple times in the upper right. A zero alpha (or zero high nibble for
 *  non-alpha formats) quad is rendered in the lower left and composited against itself in the lower right.
 *
 * @tc X_O1RGB5_Add_SrcA_1-SrcA
 *  Demonstrates the behavior of blending various texture formats with the surface mode set to
 *  NV097_SET_SURFACE_FORMAT_COLOR_LE_X1R5G5B5_O1R5G5B5 and blend factors {SrcAlpha, 1 - SrcAlpha}.
 *  A test pattern is rendered multiple times, partially overlapping itself. A low opacity grey quad is rendered in the
 *  upper left, then composited against itself multiple times in the upper right. A zero alpha (or zero high nibble for
 *  non-alpha formats) quad is rendered in the lower left and composited against itself in the lower right.
 *
 * @tc X_O1RGB5_Add_SrcA_DstA
 *  Demonstrates the behavior of blending various texture formats with the surface mode set to
 *  NV097_SET_SURFACE_FORMAT_COLOR_LE_X1R5G5B5_O1R5G5B5 and blend factors {SrcAlpha, DstAlpha}.
 *  A test pattern is rendered multiple times, partially overlapping itself. A low opacity grey quad is rendered in the
 *  upper left, then composited against itself multiple times in the upper right. A zero alpha (or zero high nibble for
 *  non-alpha formats) quad is rendered in the lower left and composited against itself in the lower right.
 *
 * @tc X_ORGB8_Add_SrcA_1-SrcA
 *  Demonstrates the behavior of blending various texture formats with the surface mode set to
 *  NV097_SET_SURFACE_FORMAT_COLOR_LE_X8R8G8B8_O8R8G8B8 and blend factors {SrcAlpha, 1 - SrcAlpha}.
 *  A test pattern is rendered multiple times, partially overlapping itself. A low opacity grey quad is rendered in the
 *  upper left, then composited against itself multiple times in the upper right. A zero alpha (or zero high nibble for
 *  non-alpha formats) quad is rendered in the lower left and composited against itself in the lower right.
 *
 * @tc X_ORGB8_Add_SrcA_DstA
 *  Demonstrates the behavior of blending various texture formats with the surface mode set to
 *  NV097_SET_SURFACE_FORMAT_COLOR_LE_X8R8G8B8_O8R8G8B8 and blend factors {SrcAlpha, DstAlpha}.
 *  A test pattern is rendered multiple times, partially overlapping itself. A low opacity grey quad is rendered in the
 *  upper left, then composited against itself multiple times in the upper right. A zero alpha (or zero high nibble for
 *  non-alpha formats) quad is rendered in the lower left and composited against itself in the lower right.
 *
 * @tc X_Z1RGB5_Add_SrcA_1-SrcA
 *  Demonstrates the behavior of blending various texture formats with the surface mode set to
 *  NV097_SET_SURFACE_FORMAT_COLOR_LE_X1R5G5B5_Z1R5G5B5 and blend factors {SrcAlpha, 1 - SrcAlpha}.
 *  A test pattern is rendered multiple times, partially overlapping itself. A low opacity grey quad is rendered in the
 *  upper left, then composited against itself multiple times in the upper right. A zero alpha (or zero high nibble for
 *  non-alpha formats) quad is rendered in the lower left and composited against itself in the lower right.
 *
 * @tc X_Z1RGB5_Add_SrcA_DstA
 *  Demonstrates the behavior of blending various texture formats with the surface mode set to
 *  NV097_SET_SURFACE_FORMAT_COLOR_LE_X1R5G5B5_Z1R5G5B5 and blend factors {SrcAlpha, DstAlpha}.
 *  A test pattern is rendered multiple times, partially overlapping itself. A low opacity grey quad is rendered in the
 *  upper left, then composited against itself multiple times in the upper right. A zero alpha (or zero high nibble for
 *  non-alpha formats) quad is rendered in the lower left and composited against itself in the lower right.
 *
 * @tc X_ZRGB8_Add_SrcA_1-SrcA
 *  Demonstrates the behavior of blending various texture formats with the surface mode set to
 *  NV097_SET_SURFACE_FORMAT_COLOR_LE_X8R8G8B8_Z8R8G8B8 and blend factors {SrcAlpha, 1 - SrcAlpha}.
 *  A test pattern is rendered multiple times, partially overlapping itself. A low opacity grey quad is rendered in the
 *  upper left, then composited against itself multiple times in the upper right. A zero alpha (or zero high nibble for
 *  non-alpha formats) quad is rendered in the lower left and composited against itself in the lower right.
 *
 * @tc X_ZRGB8_Add_SrcA_DstA
 *  Demonstrates the behavior of blending various texture formats with the surface mode set to
 *  NV097_SET_SURFACE_FORMAT_COLOR_LE_X8R8G8B8_Z8R8G8B8 and blend factors {SrcAlpha, DstAlpha}.
 *  A test pattern is rendered multiple times, partially overlapping itself. A low opacity grey quad is rendered in the
 *  upper left, then composited against itself multiple times in the upper right. A zero alpha (or zero high nibble for
 *  non-alpha formats) quad is rendered in the lower left and composited against itself in the lower right.
 *
 * @tc XA_O1A7RGB8_Add_SrcA_1-SrcA
 *  Demonstrates the behavior of blending various texture formats with the surface mode set to
 *  NV097_SET_SURFACE_FORMAT_COLOR_LE_X1A7R8G8B8_O1A7R8G8B8 and blend factors {SrcAlpha, 1 - SrcAlpha}.
 *  A test pattern is rendered multiple times, partially overlapping itself. A low opacity grey quad is rendered in the
 *  upper left, then composited against itself multiple times in the upper right. A zero alpha (or zero high nibble for
 *  non-alpha formats) quad is rendered in the lower left and composited against itself in the lower right.
 *
 * @tc XA_O1A7RGB8_Add_SrcA_DstA
 *  Demonstrates the behavior of blending various texture formats with the surface mode set to
 *  NV097_SET_SURFACE_FORMAT_COLOR_LE_X1A7R8G8B8_O1A7R8G8B8 and blend factors {SrcAlpha, DstAlpha}.
 *  A test pattern is rendered multiple times, partially overlapping itself. A low opacity grey quad is rendered in the
 *  upper left, then composited against itself multiple times in the upper right. A zero alpha (or zero high nibble for
 *  non-alpha formats) quad is rendered in the lower left and composited against itself in the lower right.
 *
 * @tc XA_Z1A7RGB8_Add_SrcA_1-SrcA
 *  Demonstrates the behavior of blending various texture formats with the surface mode set to
 *  NV097_SET_SURFACE_FORMAT_COLOR_LE_X1A7R8G8B8_Z1A7R8G8B8 and blend factors {SrcAlpha, 1 - SrcAlpha}.
 *  A test pattern is rendered multiple times, partially overlapping itself. A low opacity grey quad is rendered in the
 *  upper left, then composited against itself multiple times in the upper right. A zero alpha (or zero high nibble for
 *  non-alpha formats) quad is rendered in the lower left and composited against itself in the lower right.
 *
 * @tc XA_Z1A7RGB8_Add_SrcA_DstA
 *  Demonstrates the behavior of blending various texture formats with the surface mode set to
 *  NV097_SET_SURFACE_FORMAT_COLOR_LE_X1A7R8G8B8_Z1A7R8G8B8 and blend factors {SrcAlpha, DstAlpha}.
 *  A test pattern is rendered multiple times, partially overlapping itself. A low opacity grey quad is rendered in the
 *  upper left, then composited against itself multiple times in the upper right. A zero alpha (or zero high nibble for
 *  non-alpha formats) quad is rendered in the lower left and composited against itself in the lower right.
 *
 */
BlendSurfaceTests::BlendSurfaceTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Blend surface", config) {
  for (const auto &test : kSurfaceFormatBlendTests) {
    for (auto &blend_config : kBlendConfigs) {
      std::string name = test.name;
      name += "_";
      name += blend_config.name;

      tests_[name] = [this, name, &test, &blend_config] {
        Test(name, test.format, blend_config.func, blend_config.source_factor, blend_config.destination_factor);
      };
    }

    std::string name = "DstAlpha_";
    name += test.name;
    tests_[name] = [this, name, &test] { TestDstAlpha(name, test.format); };
  }
}

void BlendSurfaceTests::Initialize() {
  TestSuite::Initialize();

  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_ALPHA_TEST_ENABLE, false);
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, false);
  Pushbuffer::End();
}

void BlendSurfaceTests::Test(const std::string &name, TestHost::SurfaceColorFormat surface_format, uint32_t blend_func,
                             uint32_t sfactor, uint32_t dfactor) {
  host_.PrepareDraw(0xFF555555);

  // The number of partially overlapped quads to blend against each other.
  static constexpr uint32_t kNumBlends = 4;

  static constexpr uint32_t kSwatchSize = kTextureSize / 2;
  static constexpr uint32_t kMultipassSwatchSize = 8;

  // Set texture 2 to a low opacity grey color.
  memset(host_.GetTextureMemoryForStage(2), 0x44, kMultipassSwatchSize * kMultipassSwatchSize * 4);

  auto prepare_texture = [this, surface_format, blend_func, sfactor, dfactor](uint32_t texture_format) {
    host_.RenderToSurfaceStart(host_.GetTextureMemoryForStage(0), surface_format, kTextureSize, kTextureSize);

    // Set the initial background alpha.
    {
      static constexpr float kBackgroundAlpha = 0.7f;
      host_.SetBlend(false);
      host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
      host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
      host_.SetDiffuse(0.1f, 0.1f, 0.1f, kBackgroundAlpha);
      host_.DrawScreenQuad(0.0, 0.0, kTextureSize, kTextureSize, 1.f);
    }

    // Blend in the test pattern.
    {
      host_.SetBlend(true, blend_func, sfactor, dfactor);

      switch (texture_format) {
        case NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8:
        case NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8:
        case NV097_SET_TEXTURE_FORMAT_COLOR_SZ_X8R8G8B8:
          GenerateSwizzledRGBRadialATestPattern(host_.GetTextureMemoryForStage(1), kSwatchSize, kSwatchSize);
          {
            auto pixel = reinterpret_cast<uint32_t *>(host_.GetTextureMemoryForStage(3));
            for (auto i = 0; i < kMultipassSwatchSize * kMultipassSwatchSize; ++i) {
              *pixel++ = 0x00FFFF55;
            }
          }
          break;

        case NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R5G6B5:
        case NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R6G5B5:
        case NV097_SET_TEXTURE_FORMAT_COLOR_SZ_X1R5G5B5:
        case NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A1R5G5B5:
        case NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A4R4G4B4:
          GenerateSwizzledRGBA444RadialAlphaPattern(host_.GetTextureMemoryForStage(1), kSwatchSize, kSwatchSize);
          {
            auto pixel = reinterpret_cast<uint16_t *>(host_.GetTextureMemoryForStage(3));
            for (auto i = 0; i < kMultipassSwatchSize * kMultipassSwatchSize; ++i) {
              *pixel++ = 0x0FF5;
            }
          }
          break;

        default:
          ASSERT(!"Unsupported texture format");
      }

      // Draw partially overlapping colored test patterns.
      {
        host_.SetFinalCombiner0Just(TestHost::SRC_TEX1);
        host_.SetFinalCombiner1Just(TestHost::SRC_TEX1, true);
        host_.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_2D_PROJECTIVE);
        auto &texture_stage = host_.GetTextureStage(1);
        texture_stage.SetEnabled();
        texture_stage.SetFormat(GetTextureFormatInfo(texture_format));
        texture_stage.SetTextureDimensions(kSwatchSize, kSwatchSize);
        host_.SetupTextureStages();

        float increment = floorf(static_cast<float>(kSwatchSize) / (kNumBlends + 1));
        float xy = increment;
        for (auto i = 0; i < kNumBlends; ++i) {
          host_.DrawSwizzledTexturedScreenQuad(xy, xy, xy + kSwatchSize, xy + kSwatchSize, 0.f);
          xy += increment;
        }

        texture_stage.SetEnabled(false);
      }

      const float inset = kTextureSize - kMultipassSwatchSize;
      {
        host_.SetFinalCombiner0Just(TestHost::SRC_TEX2);
        host_.SetFinalCombiner1Just(TestHost::SRC_TEX2, true);
        host_.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_NONE, TestHost::STAGE_2D_PROJECTIVE);
        auto &texture_stage = host_.GetTextureStage(2);
        texture_stage.SetEnabled();
        texture_stage.SetFormat(GetTextureFormatInfo(texture_format));
        texture_stage.SetTextureDimensions(kMultipassSwatchSize, kMultipassSwatchSize);
        host_.SetupTextureStages();

        // Draw a translucent grey square in the upper left.
        float x = 0.f;
        float y = 0.f;
        host_.DrawSwizzledTexturedScreenQuad(x, y, x + kMultipassSwatchSize, y + kMultipassSwatchSize, 0.f);

        // Draw a perfectly overlapping grey square multiple times in the upper right
        x = inset;
        for (auto i = 0; i < kNumBlends; ++i) {
          host_.DrawSwizzledTexturedScreenQuad(x, y, x + kMultipassSwatchSize, y + kMultipassSwatchSize, 0.f);
        }
        texture_stage.SetEnabled(false);
      }

      {
        host_.SetFinalCombiner0Just(TestHost::SRC_TEX3);
        host_.SetFinalCombiner1Just(TestHost::SRC_TEX3, true);
        host_.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_NONE, TestHost::STAGE_NONE,
                                    TestHost::STAGE_2D_PROJECTIVE);
        auto &texture_stage = host_.GetTextureStage(3);
        texture_stage.SetEnabled();
        texture_stage.SetFormat(GetTextureFormatInfo(texture_format));
        texture_stage.SetTextureDimensions(kMultipassSwatchSize, kMultipassSwatchSize);
        host_.SetupTextureStages();

        // Draw a zero alpha square in the lower left.
        float x = 0.f;
        float y = inset;
        host_.DrawSwizzledTexturedScreenQuad(x, y, x + kMultipassSwatchSize, y + kMultipassSwatchSize, 0.f);

        // Draw an overlapping zero alpha square multiple times in the lower right
        x = inset;
        for (auto i = 0; i < kNumBlends; ++i) {
          host_.DrawSwizzledTexturedScreenQuad(x, y, x + kMultipassSwatchSize, y + kMultipassSwatchSize, 0.f);
        }
        texture_stage.SetEnabled(false);
      }

      host_.SetupTextureStages();
      host_.SetShaderStageProgram(TestHost::STAGE_NONE);

      host_.SetBlend(false);
    }

    host_.RenderToSurfaceEnd();

    host_.PBKitBusyWait();
  };

  auto draw_quad = [this, surface_format](float left, float top) {
    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetEnabled();
    texture_stage.SetFormat(GetTextureFormatInfo(TextureFormatForSurfaceFormat(surface_format, true)));
    texture_stage.SetImageDimensions(kTextureSize, kTextureSize);
    host_.SetupTextureStages();

    host_.DrawTexturedScreenQuad(left, top, left + kTextureSize, top + kTextureSize, 0.f, kTextureSize, kTextureSize);

    texture_stage.SetEnabled(false);
    host_.SetupTextureStages();
    host_.SetShaderStageProgram(TestHost::STAGE_NONE);
  };

  struct NamedTextureFormat {
    uint32_t format;
    const char *name;
  };
  static constexpr NamedTextureFormat kCompositedTextureFormats[] = {
      {NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8, "RGBA8"},
      {NV097_SET_TEXTURE_FORMAT_COLOR_SZ_X8R8G8B8, "RGBX8"},
      {NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R5G6B5, "RGB565"},
      {NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R6G5B5, "RGB555"},
      {NV097_SET_TEXTURE_FORMAT_COLOR_SZ_X1R5G5B5, "X1R5G5B5"},
      {NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A1R5G5B5, "A1R5G5B5"},
      {NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A4R4G4B4, "A4R4G4B4"},
  };

  static constexpr float kMargin = 32.f;
  float left = kMargin;
  float top = 92.f;
  static constexpr int kTextColumnStart = 3;
  int text_col = kTextColumnStart;
  int text_row = 2;
  static constexpr auto kTextureSpacing = kTextureSize + 16.f;

  for (auto &texture_format : kCompositedTextureFormats) {
    prepare_texture(texture_format.format);

    draw_quad(left, top);
    pb_printat(text_row, text_col, "%s", texture_format.name);

    left += kTextureSpacing;
    text_col += 14;

    if (left + kTextureSpacing >= host_.GetFramebufferWidthF()) {
      left = kMargin;
      top += kTextureSpacing;
      text_col = kTextColumnStart;
      text_row += 6;
    }
  }

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

  pb_printat(0, 0, (char *)"%s\n", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

void BlendSurfaceTests::TestDstAlpha(const std::string &name, TestHost::SurfaceColorFormat surface_format) {
  host_.PrepareDraw(0xFF555555);

  static constexpr uint32_t kSwatchColor = 0x22FFFFFF;
  static constexpr uint32_t kSwatchSize = 128;

  auto prepare_texture = [this, surface_format](uint32_t background_color) {
    host_.RenderToSurfaceStart(host_.GetTextureMemoryForStage(0), surface_format, kSwatchSize, kSwatchSize);

    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

    // Set the initial background.
    {
      host_.SetBlend(false);
      host_.SetDiffuse(background_color);
      host_.DrawScreenQuad(0.0, 0.0, kSwatchSize, kSwatchSize, 1.f);
    }

    // Render a white rect on top of the background, forcing alpha to opaque and blending the color using DstAlpha
    // as the source factor.
    {
      host_.SetBlend(true, NV097_SET_BLEND_EQUATION_V_FUNC_ADD, NV097_SET_BLEND_FUNC_SFACTOR_V_DST_ALPHA,
                     NV097_SET_BLEND_FUNC_DFACTOR_V_ZERO);
      host_.SetDiffuse(kSwatchColor);
      host_.DrawScreenQuad(0.0, 0.0, kSwatchSize, kSwatchSize, 1.f);
      host_.SetBlend(false);
    }
    host_.RenderToSurfaceEnd();
  };

  auto draw_quad = [this, surface_format](float left, float top) {
    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetEnabled();
    texture_stage.SetFormat(GetTextureFormatInfo(TextureFormatForSurfaceFormat(surface_format, true)));
    texture_stage.SetImageDimensions(kSwatchSize, kSwatchSize);
    host_.SetupTextureStages();

    host_.DrawTexturedScreenQuad(left, top, left + kSwatchSize, top + kSwatchSize, 0.f, kSwatchSize, kSwatchSize);

    texture_stage.SetEnabled(false);
    host_.SetupTextureStages();
    host_.SetShaderStageProgram(TestHost::STAGE_NONE);
  };

  static constexpr float kMargin = 32.f;
  float left = kMargin;
  float top = 92.f;
  static constexpr int kTextColumnStart = 4;
  int text_col = kTextColumnStart;
  int text_row = 2;
  static constexpr auto kTextureSpacing = kSwatchSize + 16.f;

  static constexpr uint32_t kBackgroundAlphas[] = {
      0x00000000, 0x40000000, 0x80000000, 0xFF000000, 0x00FF0000, 0x40FF0000, 0x80FF0000, 0xFFFF0000,
  };

  for (auto &background_color : kBackgroundAlphas) {
    prepare_texture(background_color);

    draw_quad(left, top);
    pb_printat(text_row, text_col, "0x%08X", background_color);

    auto foo = reinterpret_cast<uint32_t *>(host_.GetTextureMemoryForStage(0));
    DbgPrint("SRC: 0x%08X DST: 0x%08X = 0x%08X\n", kSwatchColor, background_color, *foo++);

    left += kTextureSpacing;
    text_col += 14;

    if (left + kTextureSpacing >= host_.GetFramebufferWidthF()) {
      left = kMargin;
      top += kTextureSpacing + 16.f;
      text_col = kTextColumnStart;
      text_row += 6;
    }
  }

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

  pb_printat(0, 0, (char *)"%s\n", name.c_str());
  pb_printat(1, 0, "Blend 0x%08X ADD DstAlpha + 0", kSwatchColor);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}
