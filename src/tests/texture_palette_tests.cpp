#include "texture_palette_tests.h"

#include <array>

#include "test_host.h"
#include "xbox-swizzle/swizzle.h"

static constexpr char kPaletteSwappingTest[] = "PaletteSwapping";
static constexpr char kXemu2646Test[] = "XemuHighPaletteBug";

static constexpr auto kTextureWidth = 128;
static constexpr auto kTextureHeight = 128;
static constexpr auto kDefaultPaletteSize = 256;

namespace {
void GenerateSwizzledPalettizedGradientSurface(uint8_t *gradient_surface, int width, int height, uint32_t palette_size,
                                               uint32_t scale = 1);
void GeneratePalette(uint32_t *palette, uint32_t palette_size, uint32_t mask);

template <size_t Size>
constexpr uint32_t GetPaletteLengthMask() {
  static_assert(Size == 256 || Size == 128 || Size == 64 || Size == 32,
                "Error: Texture palette size must be 32, 64, 128, or 256.");

  if constexpr (Size == 256) {
    return NV097_SET_TEXTURE_PALETTE_LENGTH_256;
  } else if constexpr (Size == 128) {
    return NV097_SET_TEXTURE_PALETTE_LENGTH_128;
  } else if constexpr (Size == 64) {
    return NV097_SET_TEXTURE_PALETTE_LENGTH_64;
  } else {
    return NV097_SET_TEXTURE_PALETTE_LENGTH_32;
  }
}

}  // namespace

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc PaletteSwapping
 *   Renders a gradient texture multiple times, changing the palettes between each render.
 *
 * @tc XemuHighPaletteBug
 *  Renders a gradient texture multiple times, reusing the same palette but changing just the high entries within it
 *  between each draw. Exercises xemu#2646 which miscalculates the size of the palette and excludes high entries from
 *  the cache key.
 */
TexturePaletteTests::TexturePaletteTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Texture palette", config) {
  tests_[kPaletteSwappingTest] = [this]() { TestPaletteSwapping(); };
  tests_[kXemu2646Test] = [this]() { TestXemu2646(); };
}

void TexturePaletteTests::Initialize() {
  TestSuite::Initialize();

  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  host_.SetPaletteSize(static_cast<TestHost::PaletteSize>(kDefaultPaletteSize));
}

namespace {

void DrawPalettes(TestHost &host_) {
  static constexpr auto kPaletteWidth = 256.f;
  static constexpr auto kPaletteHeight = 32.f;
  static constexpr auto kSpacing = 4.f;
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX1);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX1, true);
  host_.SetTextureStageEnabled(0, false);

  auto &texture_stage = host_.GetTextureStage(1);
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8));
  texture_stage.SetImageDimensions(kDefaultPaletteSize * 4, 4);
  host_.SetTextureStageEnabled(1, true);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_2D_PROJECTIVE);
  host_.SetupTextureStages();

  auto top = host_.GetFramebufferHeightF() - ((kPaletteHeight + kSpacing) * 4 - kSpacing);
  const float left = host_.CenterX(kPaletteWidth);

  for (auto x = 0; x < 4; ++x, top += kPaletteHeight + kSpacing) {
    const float u_left = static_cast<float>(x) * kDefaultPaletteSize;
    const float u_right = u_left + kDefaultPaletteSize;

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord1(u_left, 0.f);
    host_.SetScreenVertex(left, top);

    host_.SetTexCoord1(u_right, 0.f);
    host_.SetScreenVertex(left + kPaletteWidth, top);

    host_.SetTexCoord1(u_right, 1.f);
    host_.SetScreenVertex(left + kPaletteWidth, top + kPaletteHeight);

    host_.SetTexCoord1(u_left, 1.f);
    host_.SetScreenVertex(left, top + kPaletteHeight);
    host_.End();
  }

  host_.SetTextureStageEnabled(1, false);
  host_.SetupTextureStages();
}

}  // namespace

void TexturePaletteTests::TestPaletteSwapping() {
  host_.PrepareDraw(0xFF333333);

  GenerateSwizzledPalettizedGradientSurface(host_.GetTextureMemoryForStage(0), kTextureWidth, kTextureHeight,
                                            kDefaultPaletteSize);

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

  {
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_I8_A8R8G8B8));
    texture_stage.SetTextureDimensions(kTextureWidth, kTextureHeight);

    host_.SetTextureStageEnabled(0, true);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
    host_.SetupTextureStages();
  }

  static constexpr auto kQuadSize = 96;
  auto draw_quad = [this](float left, float top) {
    const auto right = left + kQuadSize;
    const auto bottom = top + kQuadSize;

    {
      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetTexCoord0(0.f, 0.f);
      host_.SetScreenVertex(left, top);

      host_.SetTexCoord0(1.f, 0.f);
      host_.SetScreenVertex(right, top);

      host_.SetTexCoord0(1.f, 1.f);
      host_.SetScreenVertex(right, bottom);

      host_.SetTexCoord0(0.f, 1.f);
      host_.SetScreenVertex(left, bottom);
      host_.End();
    }
  };

  auto palettes = reinterpret_cast<uint32_t *>(host_.GetTextureMemoryForStage(1));

  auto palette0 = palettes;
  auto palette1 = palette0 + kDefaultPaletteSize;
  auto palette2 = palette1 + kDefaultPaletteSize;
  auto palette3 = palette2 + kDefaultPaletteSize;

  GeneratePalette(palette0, kDefaultPaletteSize, 0xFFFFFFFF);
  GeneratePalette(palette1, kDefaultPaletteSize, 0xFF00FFFF);
  GeneratePalette(palette2, kDefaultPaletteSize, 0xFFFF00FF);
  GeneratePalette(palette3, kDefaultPaletteSize, 0xFFFFFF00);

  static constexpr uint32_t DMA_B = 2;

  auto set_palette = [](const uint32_t *palette) {
    auto palette_addr = reinterpret_cast<uintptr_t>(palette);
    ASSERT((palette_addr & 0x03ffffc0) == (palette_addr & 0x03ffffff) && "Misaligned palette");

    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_TEXTURE_PALETTE,
                     MASK(NV097_SET_TEXTURE_PALETTE_CONTEXT_DMA, DMA_B) |
                         MASK(NV097_SET_TEXTURE_PALETTE_LENGTH, GetPaletteLengthMask<kDefaultPaletteSize>()) |
                         (palette_addr & 0x03ffffc0));
    Pushbuffer::End();
  };

  // Draw the quads
  {
    auto top = 80.f;
    const auto kSpacing = floorf((host_.GetFramebufferWidthF() - kQuadSize * 4) / 5);

    float left = kSpacing;
    set_palette(palette0);
    draw_quad(left, top);
    left += kQuadSize + kSpacing;

    set_palette(palette1);
    draw_quad(left, top);
    left += kQuadSize + kSpacing;

    set_palette(palette2);
    draw_quad(left, top);
    left += kQuadSize + kSpacing;

    set_palette(palette3);
    draw_quad(left, top);

    host_.PBKitBusyWait();

    // Draw a row reusing the same palette, modifying it between each render.
    top += kQuadSize + 8.f;
    left = kSpacing;

    GeneratePalette(palette0, kDefaultPaletteSize, 0xFFFFFF00);
    set_palette(palette0);
    draw_quad(left, top);
    left += kQuadSize + kSpacing;
    host_.PBKitBusyWait();

    GeneratePalette(palette0, kDefaultPaletteSize, 0xFFFF00FF);
    set_palette(palette0);
    draw_quad(left, top);
    left += kQuadSize + kSpacing;
    host_.PBKitBusyWait();

    GeneratePalette(palette0, kDefaultPaletteSize, 0xFF00FFFF);
    set_palette(palette0);
    draw_quad(left, top);
    left += kQuadSize + kSpacing;
    host_.PBKitBusyWait();

    GeneratePalette(palette0, kDefaultPaletteSize, 0xFFFFFFFF);
    set_palette(palette0);
    draw_quad(left, top);
  }

  // Draw the final palettes
  DrawPalettes(host_);

  pb_print("%s\n", kPaletteSwappingTest);
  pb_print("The same image is rendered with different palettes\n");
  pb_draw_text_screen();

  FinishDraw(kPaletteSwappingTest);
}

void TexturePaletteTests::TestXemu2646() {
  host_.PrepareDraw(0xFF333333);

  GenerateSwizzledPalettizedGradientSurface(host_.GetTextureMemoryForStage(0), kTextureWidth, kTextureHeight,
                                            kDefaultPaletteSize, 5);

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

  {
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_I8_A8R8G8B8));
    texture_stage.SetTextureDimensions(kTextureWidth, kTextureHeight);

    host_.SetTextureStageEnabled(0, true);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
    host_.SetupTextureStages();
  }

  static constexpr auto kQuadSize = 96;
  auto draw_quad = [this](float left, float top) {
    const auto right = left + kQuadSize;
    const auto bottom = top + kQuadSize;

    {
      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetTexCoord0(0.f, 0.f);
      host_.SetScreenVertex(left, top);

      host_.SetTexCoord0(1.f, 0.f);
      host_.SetScreenVertex(right, top);

      host_.SetTexCoord0(1.f, 1.f);
      host_.SetScreenVertex(right, bottom);

      host_.SetTexCoord0(0.f, 1.f);
      host_.SetScreenVertex(left, bottom);
      host_.End();
    }
  };

  auto palette = reinterpret_cast<uint32_t *>(host_.GetTextureMemoryForStage(1));
  GeneratePalette(palette, kDefaultPaletteSize, 0xFFFFFFFF);

  static constexpr uint32_t DMA_B = 2;

  auto set_palette = [](const uint32_t *palette) {
    auto palette_addr = reinterpret_cast<uintptr_t>(palette);
    ASSERT((palette_addr & 0x03ffffc0) == (palette_addr & 0x03ffffff) && "Misaligned palette");

    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_TEXTURE_PALETTE,
                     MASK(NV097_SET_TEXTURE_PALETTE_CONTEXT_DMA, DMA_B) |
                         MASK(NV097_SET_TEXTURE_PALETTE_LENGTH, GetPaletteLengthMask<kDefaultPaletteSize>()) |
                         (palette_addr & 0x03ffffc0));
    Pushbuffer::End();
  };

  auto shift_palette = [palette]() {
    for (uint32_t i = kDefaultPaletteSize / 4; i < kDefaultPaletteSize; ++i) {
      uint32_t color = palette[i];
      uint32_t rgb = color & 0x00FFFFFF;
      uint32_t rotated_rgb = ((rgb << 8) | (rgb >> 16)) & 0x00FFFFFF;
      palette[i] = 0xFF000000 | rotated_rgb;
    }
  };

  auto roll_palette = [palette]() {
    auto start = palette + (kDefaultPaletteSize / 4);
    auto end = palette + kDefaultPaletteSize;
    std::rotate(start, start + 31, end);
  };

  // Draw a row reusing the same palette, modifying only the entries larger than kDefaultPaletteSize / 4 to exercise
  // xemu bug where palette size during texture hashing is 1/4 what it should be.
  {
    const auto top = host_.CenterY(kQuadSize);
    const auto kSpacing = floorf((host_.GetFramebufferWidthF() - kQuadSize * 4) / 5);
    auto left = kSpacing;

    set_palette(palette);
    draw_quad(left, top);
    left += kQuadSize + kSpacing;
    host_.PBKitBusyWait();

    roll_palette();
    set_palette(palette);
    draw_quad(left, top);
    left += kQuadSize + kSpacing;
    host_.PBKitBusyWait();

    shift_palette();
    set_palette(palette);
    draw_quad(left, top);
    left += kQuadSize + kSpacing;
    host_.PBKitBusyWait();

    shift_palette();
    set_palette(palette);
    draw_quad(left, top);
  }

  pb_print("%s\n", kXemu2646Test);
  pb_print("Image is rendered with only high palette entries changed\n");
  pb_print("Entries should be clearly different\n");
  pb_draw_text_screen();

  FinishDraw(kXemu2646Test);
}

namespace {
void GenerateSwizzledPalettizedGradientSurface(uint8_t *gradient_surface, int width, int height, uint32_t palette_size,
                                               uint32_t scale) {
  std::vector<uint8_t> temp_buffer(width * height);
  auto cx = static_cast<float>(width) * 0.5f;
  auto cy = static_cast<float>(height) * 0.5f;
  auto max_distance = cx * cx + cy * cy;
  auto max_value = static_cast<float>(palette_size - 1);
  auto pixels = temp_buffer.data();
  for (uint32_t y = 0; y < height; ++y) {
    auto dy = static_cast<float>(y) - cx;
    auto dy_squared = dy * dy;
    for (uint32_t x = 0; x < width; ++x, ++pixels) {
      auto dx = static_cast<float>(x) - cx;
      auto distance = dx * dx + dy_squared;
      *pixels = (static_cast<uint8_t>(max_value * (1.f - distance / max_distance)) * scale) & 0xFF;
    }
  }
  swizzle_rect(temp_buffer.data(), width, height, gradient_surface, width, 1);
}

void GeneratePalette(uint32_t *palette, uint32_t palette_size, uint32_t mask) {
  ASSERT(palette && palette_size >= 32);

  const uint32_t divisor = palette_size - 1;

  for (uint32_t i = 0; i < palette_size; ++i) {
    auto r = ((i * 255) / divisor) & 0xFF;
    auto g = 255 - r;
    auto b = (i * 4) & 0xFF;

    uint32_t color = 0xFF000000 | (r << 16) | (g << 8) | b;
    palette[i] = color & mask;
  }
}

}  // namespace
