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
 */
BlendSurfaceTests::BlendSurfaceTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Blend surface", config) {
  for (auto &blend_config : kBlendConfigs) {
    for (const auto &test : kSurfaceFormatBlendTests) {
      std::string name = test.name;
      name += "_";
      name += blend_config.name;

      tests_[name] = [this, name, &test, &blend_config] {
        Test(name, test.format, blend_config.func, blend_config.source_factor, blend_config.destination_factor);
      };
    }
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
  memset(host_.GetTextureMemoryForStage(2), 0x88, kMultipassSwatchSize * kMultipassSwatchSize * 4);

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
