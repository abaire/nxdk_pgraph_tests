#include "texture_format_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <algorithm>
#include <memory>
#include <utility>

#include "debug_output.h"
#include "shaders/passthrough_vertex_shader.h"
#include "shaders/perspective_vertex_shader_no_lighting.h"
#include "shaders/pixel_shader_program.h"
#include "test_host.h"
#include "texture_format.h"
#include "texture_generator.h"
#include "vertex_buffer.h"
#include "xbox-swizzle/swizzle.h"

static int GenerateGradientSurface(SDL_Surface **gradient_surface, int width, int height);
static int GeneratePalettizedGradientSurface(uint8_t **gradient_surface, int width, int height,
                                             TestHost::PaletteSize size);
static uint32_t *GeneratePalette(TestHost::PaletteSize size);

static const TestHost::PaletteSize kPaletteSizes[] = {
    TestHost::PALETTE_256,
    TestHost::PALETTE_128,
    TestHost::PALETTE_64,
    TestHost::PALETTE_32,
};

static constexpr uint32_t kXAlphaFormats[] = {
    NV097_SET_TEXTURE_FORMAT_COLOR_SZ_X8R8G8B8, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_X1R5G5B5,
    NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_X8R8G8B8, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_X1R5G5B5,
    NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8};

static bool RequiresSpecialTest(const TextureFormatInfo &format) {
  switch (format.xbox_format) {
    case NV097_SET_TEXTURE_FORMAT_COLOR_SZ_I8_A8R8G8B8:
    case NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_Y16_FIXED:
    case NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_Y16_FLOAT:
    case NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_X8_Y24_FIXED:
    case NV097_SET_TEXTURE_FORMAT_COLOR_L_DXT23_A8R8G8B8:
    case NV097_SET_TEXTURE_FORMAT_COLOR_L_DXT45_A8R8G8B8:
      return true;

    default:
      return false;
  }
}

TextureFormatTests::TextureFormatTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Texture format", config) {
  for (auto i = 0; i < kNumFormats; ++i) {
    auto &format = kTextureFormats[i];
    if (!RequiresSpecialTest(format)) {
      std::string name = MakeTestName(format);
      tests_[name] = [this, format]() { Test(format); };

      //      if (format.xbox_swizzled) {
      //        std::string mip_name = MakeTestName(format, true);
      //        tests_[mip_name] = [this, format]() { TestMipMap(format); };
      //      }
    }
  }

  for (auto size : kPaletteSizes) {
    std::string name = MakePalettizedTestName(size);
    tests_[name] = [this, size]() { TestPalettized(size); };
  }

  for (auto format_id : kXAlphaFormats) {
    auto &format = GetTextureFormatInfo(format_id);
    std::string name = MakeXAlphaTestName(format);
    tests_[name] = [this, format]() { TestXAlpha(format); };
  }
}

void TextureFormatTests::Initialize() {
  TestSuite::Initialize();
  CreateGeometry();

  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
  PixelShaderProgram::LoadTexturedPixelShader();
}

void TextureFormatTests::CreateGeometry() {
  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(6);
  buffer->DefineBiTri(0, -0.75, 0.75, 0.75, -0.75, 0.1f);
  buffer->Linearize(static_cast<float>(host_.GetMaxTextureWidth()), static_cast<float>(host_.GetMaxTextureHeight()));
}

void TextureFormatTests::Test(const TextureFormatInfo &texture_format) {
  auto shader =
      std::make_shared<PerspectiveVertexShaderNoLighting>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  host_.SetVertexShaderProgram(shader);

  host_.SetTextureFormat(texture_format);
  std::string test_name = MakeTestName(texture_format);

  SDL_Surface *gradient_surface;
  int update_texture_result =
      GenerateGradientSurface(&gradient_surface, (int)host_.GetMaxTextureWidth(), (int)host_.GetMaxTextureHeight());
  ASSERT(!update_texture_result && "Failed to generate SDL surface");

  update_texture_result = host_.SetTexture(gradient_surface);
  SDL_FreeSurface(gradient_surface);
  ASSERT(!update_texture_result && "Failed to set texture");

  host_.PrepareDraw(0xFE202020);
  host_.DrawArrays();

  pb_print("N: %s\n", texture_format.name);
  pb_print("F: 0x%x\n", texture_format.xbox_format);
  pb_print("SZ: %d\n", texture_format.xbox_swizzled);
  pb_print("C: %d\n", texture_format.require_conversion);
  pb_print("W: %d\n", host_.GetMaxTextureWidth());
  pb_print("H: %d\n", host_.GetMaxTextureHeight());
  pb_print("P: %d\n", texture_format.xbox_bpp * host_.GetMaxTextureWidth() / 8);
  pb_draw_text_screen();

  FinishDraw(test_name);
}

void TextureFormatTests::TestPalettized(TestHost::PaletteSize size) {
  auto shader =
      std::make_shared<PerspectiveVertexShaderNoLighting>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  host_.SetVertexShaderProgram(shader);

  auto &texture_format = GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_I8_A8R8G8B8);
  host_.SetTextureFormat(texture_format);
  std::string test_name = MakePalettizedTestName(size);

  uint8_t *gradient_surface = nullptr;
  int err = GeneratePalettizedGradientSurface(&gradient_surface, (int)host_.GetMaxTextureWidth(),
                                              (int)host_.GetMaxTextureHeight(), size);
  ASSERT(!err && "Failed to generate palettized surface");

  err = host_.SetRawTexture(gradient_surface, host_.GetMaxTextureWidth(), host_.GetMaxTextureHeight(), 1,
                            host_.GetMaxTextureWidth(), 1, texture_format.xbox_swizzled);
  delete[] gradient_surface;
  ASSERT(!err && "Failed to set texture");

  auto palette = GeneratePalette(size);
  err = host_.SetPalette(palette, size);
  delete[] palette;
  ASSERT(!err && "Failed to set palette");

  host_.PrepareDraw(0xFE202020);
  host_.DrawArrays();

  pb_print("N: %s\n", texture_format.name);
  pb_print("Ps: %d\n", size);
  pb_print("F: 0x%x\n", texture_format.xbox_format);
  pb_print("SZ: %d\n", texture_format.xbox_swizzled);
  pb_print("C: %d\n", texture_format.require_conversion);
  pb_print("W: %d\n", host_.GetMaxTextureWidth());
  pb_print("H: %d\n", host_.GetMaxTextureHeight());
  pb_print("P: %d\n", texture_format.xbox_bpp * host_.GetMaxTextureWidth() / 8);
  pb_draw_text_screen();

  FinishDraw(test_name);
}

// TODO: Implement mipmap generation and fully populate the texture.
// void TextureFormatTests::TestMipMap(const TextureFormatInfo &texture_format) {
//  auto shader = std::make_shared<PassthroughVertexShader>();
//  host_.SetVertexShaderProgram(shader);
//
//  host_.SetTextureFormat(texture_format);
//  std::string test_name = MakeTestName(texture_format, true);
//
//  SDL_Surface *gradient_surface;
//  int update_texture_result =
//      GenerateGradientSurface(&gradient_surface, (int)host_.GetMaxTextureWidth(), (int)host_.GetMaxTextureHeight());
//  ASSERT(!update_texture_result && "Failed to generate SDL surface");
//
//  update_texture_result = host_.SetTexture(gradient_surface);
//  SDL_FreeSurface(gradient_surface);
//  ASSERT(!update_texture_result && "Failed to set texture");
//
//  auto &texture_stage = host_.GetTextureStage(0);
//  texture_stage.SetFilter(0, TextureStage::K_QUINCUNX, TextureStage::MIN_TENT_TENT_LOD);
//  host_.SetupTextureStages();
//
//  host_.PrepareDraw(0xFE202020);
//
//  auto draw = [this](float left, float top, float size) {
//    float right = left + size;
//    float bottom = top + size;
//
//    host_.Begin(TestHost::PRIMITIVE_QUADS);
//    host_.SetTexCoord0(0.f, 0.f);
//    host_.SetVertex(left, top, 0.1f, 1.f);
//
//    host_.SetTexCoord0(1.f, 0.f);
//    host_.SetVertex(right, top, 0.1f, 1.f);
//
//    host_.SetTexCoord0(1.f, 1.f);
//    host_.SetVertex(right, bottom, 0.1f, 1.f);
//
//    host_.SetTexCoord0(0.f, 1.f);
//    host_.SetVertex(left, bottom, 0.1f, 1.f);
//    host_.End();
//  };
//
//  draw(5.f, 80.f, 256.f);
//  draw(270.f, 80.f, 128.f);
//  draw(410.f, 80.f, 64.f);
//  draw(480.f, 80.f, 32.f);
//  draw(520.f, 80.f, 16.f);
//  draw(270.f, 220.f, 8.f);
//  draw(280.f, 220.f, 4.f);
//  draw(290.f, 220.f, 2.f);
//  draw(300.f, 220.f, 1.f);
//
//  texture_stage.SetMipMapLevels(1);
//
//  pb_print("N: %s\n", test_name.c_str());
//  pb_print("F: 0x%x\n", texture_format.xbox_format);
//  pb_print("SZ: %d\n", texture_format.xbox_swizzled);
//  pb_print("C: %d\n", texture_format.require_conversion);
//  pb_print("W: %d\n", host_.GetMaxTextureWidth());
//  pb_print("H: %d\n", host_.GetMaxTextureHeight());
//  pb_print("P: %d\n", texture_format.xbox_bpp * host_.GetMaxTextureWidth() / 8);
//  pb_draw_text_screen();
//
//  FinishDraw(test_name);
//}

std::string TextureFormatTests::MakeTestName(const TextureFormatInfo &texture_format, bool mipmap) {
  std::string test_name = mipmap ? "Mip_" : "TexFmt_";

  test_name += texture_format.name;
  if (texture_format.xbox_linear) {
    test_name += "_L";
  }
  return std::move(test_name);
}

std::string TextureFormatTests::MakePalettizedTestName(TestHost::PaletteSize size) {
  std::string test_name = "TexFmt_";
  auto &fmt = GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_I8_A8R8G8B8);
  test_name += fmt.name;

  char buf[32] = {0};
  snprintf(buf, 31, "_p%d", size);
  test_name += buf;

  return std::move(test_name);
}

static constexpr uint32_t kCheckerboardA = 0xFF202020;
static constexpr uint32_t kCheckerboardB = 0xFF707070;

static void DrawCheckerboardBackground(TestHost &host) {
  static constexpr auto kTextureSize = 256;
  auto texture_memory = host.GetTextureMemoryForStage(1);
  GenerateRGBACheckerboard(texture_memory, 0, 0, kTextureSize, kTextureSize, kTextureSize * 4, kCheckerboardA,
                           kCheckerboardB);
  host.SetBlend(false);
  host.SetFinalCombiner0Just(TestHost::SRC_TEX1);
  host.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
  host.SetTextureStageEnabled(1, true);
  host.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_2D_PROJECTIVE);

  auto &texture_stage = host.GetTextureStage(1);
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8B8G8R8));
  texture_stage.SetImageDimensions(kTextureSize, kTextureSize);
  host.SetupTextureStages();

  const float right = host.GetFramebufferWidth();
  const float bottom = host.GetFramebufferHeight();
  static constexpr float kZ = 0.f;

  host.Begin(TestHost::PRIMITIVE_QUADS);
  host.SetTexCoord1(0.f, 0.f);
  host.SetVertex(0.f, 0.f, kZ);

  host.SetTexCoord1(kTextureSize, 0.f);
  host.SetVertex(right, 0.f, kZ);

  host.SetTexCoord1(kTextureSize, kTextureSize);
  host.SetVertex(right, bottom, kZ);

  host.SetTexCoord1(0.f, kTextureSize);
  host.SetVertex(0.f, bottom, kZ);
  host.End();

  host.PBKitBusyWait();

  host.SetTextureStageEnabled(1, false);
  host.SetShaderStageProgram(TestHost::STAGE_NONE);
  host.SetBlend(true);
}

std::string TextureFormatTests::MakeXAlphaTestName(const TextureFormatInfo &texture_format) {
  std::string test_name = "TexFmt_XAlpha_";
  test_name += texture_format.name;
  if (texture_format.xbox_linear) {
    test_name += "_L";
  }
  return test_name;
}

void TextureFormatTests::TestXAlpha(const TextureFormatInfo &texture_format) {
  static constexpr uint32_t kTextureWidth = 64;
  static constexpr uint32_t kTextureHeight = 64;
  static constexpr uint32_t kTextureOffsetStep = 16384;
  static constexpr float kQuadWidth = 80.f;
  static constexpr float kQuadHeight = 75.f;
  static constexpr float kLeftStart = 30.f;
  static constexpr float kQuadSpacing = 20.f;
  static constexpr float kRow1Top = 200.f;
  static constexpr float kRow2Top = 325.f;
  static constexpr float kZ = 0.f;

  static constexpr uint32_t kColors32[] = {
      0x00DACABA, 0x011B2B3B, 0x7F3C2C1C, 0x804D5D6D, 0xFE0ECE3E, 0xFF8F9FAF,
  };
  static constexpr uint16_t kColors16[] = {
      0x03E0, 0x83E0, 0x03FF, 0x83FF, 0x7FFF, 0xFFFF,
  };
  static_assert(std::size(kColors32) == std::size(kColors16), "kColors32 and kColors16 must have the same length");
  static constexpr uint32_t kNumTestValues = std::size(kColors32);

  const bool is_16bpp = (texture_format.xbox_bpp == 16);

  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetFormat(texture_format);
  texture_stage.SetImageDimensions(kTextureWidth, kTextureHeight);
  texture_stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
  texture_stage.SetFilter(0, TextureStage::K_QUINCUNX, TextureStage::MIN_BOX_LOD0, TextureStage::MAG_BOX_LOD0);

  uint8_t *texture_base = host_.GetTextureMemoryForStage(0);
  const uint32_t texture_dma_addr = reinterpret_cast<uint32_t>(texture_base) & 0x03ffffff;

  if (is_16bpp) {
    uint16_t pixel_buffer[kTextureWidth * kTextureHeight];
    for (uint32_t i = 0; i < kNumTestValues; ++i) {
      std::fill_n(pixel_buffer, kTextureWidth * kTextureHeight, kColors16[i]);
      uint8_t *target = texture_base + i * kTextureOffsetStep;
      if (texture_format.xbox_swizzled) {
        swizzle_rect(reinterpret_cast<const uint8_t *>(pixel_buffer), kTextureWidth, kTextureHeight, target,
                     kTextureWidth * sizeof(uint16_t), sizeof(uint16_t));
      } else {
        memcpy(target, pixel_buffer, kTextureWidth * kTextureHeight * sizeof(uint16_t));
      }
    }
  } else {
    uint32_t pixel_buffer[kTextureWidth * kTextureHeight];
    for (uint32_t i = 0; i < kNumTestValues; ++i) {
      std::fill_n(pixel_buffer, kTextureWidth * kTextureHeight, kColors32[i]);
      uint8_t *target = texture_base + i * kTextureOffsetStep;
      if (texture_format.xbox_swizzled) {
        swizzle_rect(reinterpret_cast<const uint8_t *>(pixel_buffer), kTextureWidth, kTextureHeight, target,
                     kTextureWidth * sizeof(uint32_t), sizeof(uint32_t));
      } else {
        memcpy(target, pixel_buffer, kTextureWidth * kTextureHeight * sizeof(uint32_t));
      }
    }
  }

  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, false);
  Pushbuffer::End();

  host_.PrepareDraw(0xFF220022);
  DrawCheckerboardBackground(host_);

  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
  host_.SetupTextureStages();

  const float u_max = texture_format.xbox_linear ? static_cast<float>(kTextureWidth) : 1.f;
  const float v_max = texture_format.xbox_linear ? static_cast<float>(kTextureHeight) : 1.f;
  const float u_half = u_max * 0.5f;

  for (uint32_t i = 0; i < kNumTestValues; ++i) {
    const float left = kLeftStart + static_cast<float>(i) * (kQuadWidth + kQuadSpacing);
    const float right = left + kQuadWidth;
    const float center_x = left + kQuadWidth * 0.5f;
    const float bottom1 = kRow1Top + kQuadHeight;
    const float bottom2 = kRow2Top + kQuadHeight;

    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_TEXTURE_OFFSET, texture_dma_addr + i * kTextureOffsetStep);
    Pushbuffer::End();

    // Row 1: Left half unblended Tex0.rgb
    host_.SetBlend(false);
    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(0.f, 0.f);
    host_.SetVertex(left, kRow1Top, kZ);
    host_.SetTexCoord0(u_half, 0.f);
    host_.SetVertex(center_x, kRow1Top, kZ);
    host_.SetTexCoord0(u_half, v_max);
    host_.SetVertex(center_x, bottom1, kZ);
    host_.SetTexCoord0(0.f, v_max);
    host_.SetVertex(left, bottom1, kZ);
    host_.End();

    // Row 1: Right half blended Tex0.rgb
    host_.SetBlend(true);

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(u_half, 0.f);
    host_.SetVertex(center_x, kRow1Top, kZ);
    host_.SetTexCoord0(u_max, 0.f);
    host_.SetVertex(right, kRow1Top, kZ);
    host_.SetTexCoord0(u_max, v_max);
    host_.SetVertex(right, bottom1, kZ);
    host_.SetTexCoord0(u_half, v_max);
    host_.SetVertex(center_x, bottom1, kZ);
    host_.End();

    // Row 2: Full quad unblended Tex0.aaa (replicate alpha to RGB)
    host_.SetBlend(false);
    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0, true);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(0.f, 0.f);
    host_.SetVertex(left, kRow2Top, kZ);
    host_.SetTexCoord0(u_max, 0.f);
    host_.SetVertex(right, kRow2Top, kZ);
    host_.SetTexCoord0(u_max, v_max);
    host_.SetVertex(right, bottom2, kZ);
    host_.SetTexCoord0(0.f, v_max);
    host_.SetVertex(left, bottom2, kZ);
    host_.End();
  }

  std::string test_name = MakeXAlphaTestName(texture_format);
  pb_printat(0, 0, (char *)"%s (%s)", test_name.c_str(), texture_format.name);
  pb_printat(1, 0, (char *)"Fmt: 0x%X Bpp: %d %s", texture_format.xbox_format, texture_format.xbox_bpp,
             texture_format.xbox_linear ? "Linear" : "Swizzled");

  if (is_16bpp) {
    pb_printat(3, 0, (char *)"Alpha:     0         1         0         1         0         1");
  } else {
    pb_printat(3, 0, (char *)"Alpha:    0x00      0x01      0x7F      0x80      0xFE      0xFF");
  }

  for (uint32_t i = 0; i < kNumTestValues; ++i) {
    if (is_16bpp) {
      pb_printat(4, 2 + i * 10, (char *)"0x%04X", kColors16[i]);
    } else {
      pb_printat(4, 1 + i * 10, (char *)"%08X", kColors32[i]);
    }
  }

  pb_printat(6, 0, (char *)"Tex0.rgb (Left: Opaque, Right: Blend)");
  pb_printat(11, 0, (char *)"Grayscale Tex0.a (out.rgb = Tex0.aaa)");

  pb_draw_text_screen();
  FinishDraw(test_name);

  // Restore state
  host_.SetBlend(false);
  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
  PixelShaderProgram::LoadTexturedPixelShader();

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_TEXTURE_OFFSET, texture_dma_addr);
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, true);
  Pushbuffer::End();
}

static int GenerateGradientSurface(SDL_Surface **gradient_surface, int width, int height) {
  *gradient_surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
  if (!(*gradient_surface)) {
    return 1;
  }

  if (SDL_LockSurface(*gradient_surface)) {
    SDL_FreeSurface(*gradient_surface);
    *gradient_surface = nullptr;
    return 2;
  }

  auto pixels = static_cast<uint32_t *>((*gradient_surface)->pixels);
  for (int y = 0; y < height; ++y) {
    auto y_normal = static_cast<int>(static_cast<float>(y) * 255.f / static_cast<float>(height));

    for (int x = 0; x < width; ++x, ++pixels) {
      auto x_normal = static_cast<int>(static_cast<float>(x) * 255.f / static_cast<float>(width));
      *pixels = SDL_MapRGBA((*gradient_surface)->format, y_normal, x_normal, 255 - y_normal, x_normal + y_normal);
    }
  }

  SDL_UnlockSurface(*gradient_surface);

  return 0;
}

static int GeneratePalettizedGradientSurface(uint8_t **gradient_surface, int width, int height,
                                             TestHost::PaletteSize palette_size) {
  *gradient_surface = new uint8_t[width * height];
  if (!(*gradient_surface)) {
    return 1;
  }

  auto pixel = *gradient_surface;

  uint32_t total_size = width * height;
  uint32_t half_size = total_size >> 1;

  for (uint32_t i = 0; i < half_size; ++i, ++pixel) {
    *pixel = i & (palette_size - 1);
  }

  for (uint32_t i = half_size; i < total_size; i += 4) {
    uint8_t value = i & (palette_size - 1);
    *pixel++ = value;
    *pixel++ = value;
    *pixel++ = value;
    *pixel++ = value;
  }

  return 0;
}

static uint32_t *GeneratePalette(TestHost::PaletteSize size) {
  auto ret = new uint32_t[size];

  uint32_t block_size = size / 4;
  auto component_inc = (uint32_t)ceilf(255.f / (float)block_size);
  uint32_t i = 0;
  uint32_t component = 0;
  for (; i < block_size; ++i, component += component_inc) {
    uint32_t color_value = 0xFF - component;
    ret[i + block_size * 0] = 0xFF000000 + color_value;
    ret[i + block_size * 1] = 0xFF000000 + (color_value << 8);
    ret[i + block_size * 2] = 0xFF000000 + (color_value << 16);
    ret[i + block_size * 3] = 0xFF000000 + color_value + (color_value << 8) + (color_value << 16);
  }

  return ret;
}
