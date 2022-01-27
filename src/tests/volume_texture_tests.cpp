#include "volume_texture_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "shaders/perspective_vertex_shader.h"
#include "test_host.h"
#include "texture_format.h"
#include "vertex_buffer.h"

static int generate_gradient_surface(SDL_Surface **gradient_surface, int width, int height);
static uint32_t *GeneratePalette(TestHost::PaletteSize size);

static const VolumeTextureTests::TestConfig kTestConfigs[] = {
    {"Indexed", GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8B8G8R8), false},
};

static constexpr const char *kTestName = "Palettized";

VolumeTextureTests::VolumeTextureTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Volume texture") {
  for (auto &config : kTestConfigs) {
    tests_[kTestName] = [this, &config]() { Test(config); };
  }
}

void VolumeTextureTests::Initialize() {
  TestSuite::Initialize();
  CreateGeometry();

  host_.SetShaderProgram(nullptr);
  CreateGeometry();
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
}

void VolumeTextureTests::CreateGeometry() {
  const float left = -2.75f;
  const float right = 2.75f;
  const float top = 1.75f;
  const float bottom = -1.75f;
  const float mid_width = 0;
  const float mid_height = 0;

  const uint32_t num_quads = 4;
  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(6 * num_quads);
  buffer->SetTexCoord0Count(3);

  const float spacing = 0.05f;
  int index = 0;

  buffer->DefineBiTri(index++, left, top, mid_width - spacing, mid_height + spacing);

  buffer->DefineBiTri(index++, mid_width + spacing, top, right, mid_height + spacing);

  buffer->DefineBiTri(index++, left, mid_height - spacing, mid_width - spacing, bottom);

  buffer->DefineBiTri(index++, mid_width + spacing, mid_height - spacing, right, bottom);

  // Set texcoords.
  auto vertex = buffer->Lock();

  auto set_bitri_texcoords = [&vertex](float p) {
    vertex++->SetTexCoord0(0.0f, 0.0f, p, 0.0);
    vertex++->SetTexCoord0(1.0f, 0.0f, p, 0.0);
    vertex++->SetTexCoord0(1.0f, 1.0f, p, 0.0);

    vertex++->SetTexCoord0(0.0f, 0.0f, p, 0.0);
    vertex++->SetTexCoord0(1.0f, 1.0f, p, 0.0);
    vertex++->SetTexCoord0(0.0f, 1.0f, p, 0.0);
  };

  set_bitri_texcoords(0.0f);

  set_bitri_texcoords(0.33f);

  set_bitri_texcoords(0.66f);

  set_bitri_texcoords(1.0f);

  buffer->Unlock();

  buffer->Linearize(static_cast<float>(host_.GetMaxTextureWidth()), static_cast<float>(host_.GetMaxTextureHeight()));
}

void VolumeTextureTests::Test(const TestConfig &config) {
  static constexpr uint32_t kBackgroundColor = 0xFF303030;
  host_.PrepareDraw(kBackgroundColor);
  //  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_I8_A8R8G8B8));

  //  SDL_Surface *gradient_surface;
  //  int update_texture_result =
  //      GenerateGradientSurface(&gradient_surface, host_.GetMaxTextureWidth(), host_.GetMaxTextureHeight());
  //  if (!update_texture_result) {
  //    update_texture_result = host_.SetTexture(gradient_surface);
  //    SDL_FreeSurface(gradient_surface);
  //  } else {
  //    pb_print("FAILED TO GENERATE SDL SURFACE - TEST IS INVALID: %d\n", update_texture_result);
  //    pb_draw_text_screen();
  //    host_.FinishDraw(false, "", "");
  //    return;
  //  }

  host_.DrawArrays();

  //  /* PrepareDraw some text on the screen */
  pb_print("N: %s\n", config.format.name);
  pb_print("F: 0x%x\n", config.format.xbox_format);
  pb_print("SZ: %d\n", config.format.xbox_swizzled);
  pb_print("C: %d\n", config.format.require_conversion);
  //  pb_print("W: %d\n", host_.GetMaxTextureWidth());
  //  pb_print("H: %d\n", host_.GetMaxTextureHeight());
  //  pb_print("P: %d\n", texture_format.xbox_bpp * host_.GetMaxTextureWidth());
  //  pb_print("ERR: %d\n", update_texture_result);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, config.name);
}

static int generate_gradient_surface(SDL_Surface **gradient_surface, int width, int height) {
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
  for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; ++x, ++pixels) {
      int x_normal = static_cast<int>(static_cast<float>(x) * 255.0f / static_cast<float>(width));
      int y_normal = static_cast<int>(static_cast<float>(y) * 255.0f / static_cast<float>(height));
      *pixels = SDL_MapRGBA((*gradient_surface)->format, y_normal, x_normal, 255 - y_normal, x_normal + y_normal);
    }

  SDL_UnlockSurface(*gradient_surface);

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