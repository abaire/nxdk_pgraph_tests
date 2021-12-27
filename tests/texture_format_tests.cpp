#include "texture_format_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "shaders/perspective_vertex_shader.h"
#include "test_host.h"
#include "texture_format.h"
#include "vertex_buffer.h"

static int generate_gradient_surface(SDL_Surface **gradient_surface, int width, int height);

TextureFormatTests::TextureFormatTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Texture format") {
  for (auto i = 0; i < kNumFormats; ++i) {
    auto &format = kTextureFormats[i];
    std::string name = MakeTestName(format);

    tests_[name] = [this, format]() { this->Test(format); };
  }
}

void TextureFormatTests::Initialize() {
  CreateGeometry();

  host_.SetDepthBufferFormat(NV097_SET_SURFACE_FORMAT_ZETA_Z24S8);
  host_.SetDepthBufferFloatMode(false);

  host_.SetShaderProgram(
      std::make_shared<PerspectiveVertexShader>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight()));
  host_.SetTextureStageEnabled(0, true);
}

void TextureFormatTests::CreateGeometry() {
  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(6);
  buffer->DefineQuad(0, -0.75, 0.75, 0.75, -0.75, 0.1f);
  buffer->Linearize(static_cast<float>(host_.GetTextureWidth()), static_cast<float>(host_.GetTextureHeight()));
}

void TextureFormatTests::Test(const TextureFormatInfo &texture_format) {
  host_.SetTextureFormat(texture_format);

  SDL_Surface *gradient_surface;
  int update_texture_result =
      generate_gradient_surface(&gradient_surface, host_.GetTextureWidth(), host_.GetTextureHeight());
  if (!update_texture_result) {
    update_texture_result = host_.SetTexture(gradient_surface);
    SDL_FreeSurface(gradient_surface);
  } else {
    pb_print("FAILED TO GENERATE SDL SURFACE - TEST IS INVALID: %d\n", update_texture_result);
    pb_draw_text_screen();
    host_.FinishDraw();
    return;
  }

  host_.PrepareDraw();
  host_.DrawVertices();

  /* PrepareDraw some text on the screen */
  pb_print("N: %s\n", texture_format.name);
  pb_print("F: 0x%x\n", texture_format.xbox_format);
  pb_print("SZ: %d\n", texture_format.xbox_swizzled);
  pb_print("C: %d\n", texture_format.require_conversion);
  pb_print("W: %d\n", host_.GetTextureWidth());
  pb_print("H: %d\n", host_.GetTextureHeight());
  pb_print("P: %d\n", texture_format.xbox_bpp * host_.GetTextureWidth());
  pb_print("ERR: %d\n", update_texture_result);
  pb_draw_text_screen();

  std::string test_name = MakeTestName(texture_format);
  host_.FinishDrawAndSave(output_dir_, test_name);
}

std::string TextureFormatTests::MakeTestName(const TextureFormatInfo &texture_format) {
  std::string test_name = "TexFmt_";
  test_name += texture_format.name;
  return std::move(test_name);
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
