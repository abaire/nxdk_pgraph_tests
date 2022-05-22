#include "texture_format_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "shaders/perspective_vertex_shader.h"
#include "shaders/pixel_shader_program.h"
#include "test_host.h"
#include "texture_format.h"
#include "vertex_buffer.h"

static int GenerateGradientSurface(SDL_Surface **gradient_surface, int width, int height);
static int GeneratePalettizedGradientSurface(uint8_t **gradient_surface, int width, int height,
                                             TestHost::PaletteSize size);
static uint32_t *GeneratePalette(TestHost::PaletteSize size);

static constexpr TestHost::PaletteSize kPaletteSizes[] = {
    TestHost::PALETTE_256,
    TestHost::PALETTE_128,
    TestHost::PALETTE_64,
    TestHost::PALETTE_32,
};

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

TextureFormatTests::TextureFormatTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Texture format") {
  for (auto i = 0; i < kNumFormats; ++i) {
    auto &format = kTextureFormats[i];
    std::string name = MakeTestName(format);

    if (!RequiresSpecialTest(format)) {
      tests_[name] = [this, format]() { Test(format); };
    }
  }

  for (auto size : kPaletteSizes) {
    std::string name = MakePalettizedTestName(size);
    tests_[name] = [this, size]() { TestPalettized(size); };
  }
}

void TextureFormatTests::Initialize() {
  TestSuite::Initialize();
  CreateGeometry();

  auto shader = std::make_shared<PerspectiveVertexShader>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  shader->SetLightingEnabled(false);
  host_.SetVertexShaderProgram(shader);
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

  host_.FinishDraw(allow_saving_, output_dir_, test_name);
}

void TextureFormatTests::TestPalettized(TestHost::PaletteSize size) {
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

  host_.FinishDraw(allow_saving_, output_dir_, test_name);
}

std::string TextureFormatTests::MakeTestName(const TextureFormatInfo &texture_format) {
  std::string test_name = "TexFmt_";
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
    for (int x = 0; x < width; ++x, ++pixels) {
      int x_normal = static_cast<int>(static_cast<float>(x) * 255.0f / static_cast<float>(width));
      int y_normal = static_cast<int>(static_cast<float>(y) * 255.0f / static_cast<float>(height));
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
  auto component_inc = (uint32_t)ceilf(255.0f / (float)block_size);
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
