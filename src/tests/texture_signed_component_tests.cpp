#include "texture_signed_component_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "shaders/perspective_vertex_shader.h"
#include "shaders/pixel_shader_program.h"
#include "shaders/precalculated_vertex_shader.h"
#include "test_host.h"
#include "texture_format.h"
#include "texture_generator.h"
#include "vertex_buffer.h"

static int GenerateBlockTestSurface(SDL_Surface **surface, int width, int height);

TextureSignedComponentTests::TextureSignedComponentTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Texture signed component tests") {
  auto add_test = [this](uint32_t texture_format, uint32_t signed_flags) {
    const TextureFormatInfo &texture_format_info = GetTextureFormatInfo(texture_format);
    std::string name = MakeTestName(texture_format_info, signed_flags);
    tests_[name] = [this, texture_format_info, signed_flags, name]() { Test(texture_format_info, signed_flags, name); };
  };

  for (auto i = 0; i <= 0x0F; ++i) {
    add_test(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8, i);
  }

  {
    const TextureFormatInfo &texture_format_info = GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8);
    tests_[texture_format_info.name] = [this, texture_format_info]() { TestGradients(texture_format_info); };
  }
}

void TextureSignedComponentTests::Initialize() {
  TestSuite::Initialize();
  CreateGeometry();

  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  PixelShaderProgram::LoadTexturedPixelShader();
}

void TextureSignedComponentTests::CreateGeometry() {
  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(6);
  buffer->DefineBiTri(0, -0.75, 0.75, 0.75, -0.75, 0.1f);
  buffer->Linearize(static_cast<float>(host_.GetMaxTextureWidth()), static_cast<float>(host_.GetMaxTextureHeight()));
}

void TextureSignedComponentTests::Test(const TextureFormatInfo &texture_format, uint32_t signed_flags,
                                       const std::string &test_name) {
  auto shader = std::make_shared<PerspectiveVertexShader>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  shader->SetLightingEnabled(false);
  host_.SetVertexShaderProgram(shader);

  host_.SetTextureFormat(texture_format);

  SDL_Surface *test_surface;
  int update_texture_result =
      GenerateBlockTestSurface(&test_surface, (int)host_.GetMaxTextureWidth(), (int)host_.GetMaxTextureHeight());
  ASSERT(!update_texture_result && "Failed to generate SDL surface");

  const bool signed_alpha = signed_flags & 0x01;
  const bool signed_red = signed_flags & 0x02;
  const bool signed_green = signed_flags & 0x04;
  const bool signed_blue = signed_flags & 0x08;

  auto &stage = host_.GetTextureStage(0);
  stage.SetFilter(0, TextureStage::K_QUINCUNX, TextureStage::MIN_BOX_LOD0, TextureStage::MAG_BOX_LOD0, signed_alpha,
                  signed_red, signed_green, signed_blue);

  update_texture_result = host_.SetTexture(test_surface);
  SDL_FreeSurface(test_surface);
  ASSERT(!update_texture_result && "Failed to set texture");

  host_.PrepareDraw(0xFE202020);
  host_.DrawArrays();

  pb_print("%s    Signed: %s%s%s%s\n", test_name.c_str(), signed_alpha ? "A" : "", signed_red ? "R" : "",
           signed_green ? "G" : "", signed_blue ? "B" : "");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, test_name);
}

void TextureSignedComponentTests::TestGradients(const TextureFormatInfo &texture_format) {
  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);

  host_.SetTextureFormat(texture_format);

  static constexpr uint32_t kTextureSize = 256;
  auto setup_texture = [this](uint32_t mask) {
    SDL_Surface *test_surface =
        SDL_CreateRGBSurfaceWithFormat(0, kTextureSize, kTextureSize, 32, SDL_PIXELFORMAT_ABGR8888);
    ASSERT(test_surface);
    ASSERT(!SDL_LockSurface(test_surface));

    auto texture_memory = reinterpret_cast<uint32_t *>(test_surface->pixels);
    for (uint32_t y = 0; y < kTextureSize; ++y) {
      uint32_t val = (y & 0xFF);
      uint32_t texture_value;
      if (mask == 0xFF000000) {
        texture_value = (val << 24) | 0xFFFFFF;
      } else {
        texture_value = 0xFF000000 | (((val << 16) | (val << 8) | val) & mask);
      }

      for (uint32_t x = 0; x < kTextureSize; ++x) {
        *texture_memory++ = texture_value;
      }
    }
    SDL_UnlockSurface(test_surface);

    int update_texture_result = host_.SetTexture(test_surface);
    SDL_FreeSurface(test_surface);
    ASSERT(!update_texture_result && "Failed to set texture");
  };

  auto &stage = host_.GetTextureStage(0);

  host_.PrepareDraw(0xFFFF00FF);

  constexpr uint32_t kMasks[] = {
      0xFF0000FF,
      0xFF00FF00,
      0xFFFF0000,
      0xFF000000,
  };

  auto draw = [this](float left, float top, float right, float bottom) {
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(0.0f, 0.0f);
    host_.SetVertex(left, top, 1.0f);

    host_.SetTexCoord0(1.0f, 0.0f);
    host_.SetVertex(right, top, 1.0f);

    host_.SetTexCoord0(1.0f, 1.0f);
    host_.SetVertex(right, bottom, 1.0f);

    host_.SetTexCoord0(0.0f, 1.0f);
    host_.SetVertex(left, bottom, 1.0f);
    host_.End();
  };

  float left = 0.0f;
  float x_inc = host_.GetFramebufferWidthF() / 4.0f;
  float top = 0.0f;
  float y_inc = host_.GetFramebufferHeightF() / 2.0f;
  for (auto mask : kMasks) {
    setup_texture(mask);

    stage.SetFilter(0, TextureStage::K_QUINCUNX, TextureStage::MIN_BOX_LOD0, TextureStage::MAG_BOX_LOD0);
    host_.SetupTextureStages();

    float right = left + x_inc;
    float bottom = top + y_inc;

    draw(left, top, right, bottom);
    left = right;
    right += x_inc;

    stage.SetFilter(0, TextureStage::K_QUINCUNX, TextureStage::MIN_BOX_LOD0, TextureStage::MAG_BOX_LOD0,
                    mask == 0xFF000000, (mask & 0xFF) != 0, (mask & 0xFF00) != 0, (mask & 0xFF0000) != 0);
    host_.SetupTextureStages();
    draw(left, top, right, bottom);
    left = right;

    if (right >= host_.GetFramebufferWidthF()) {
      top = bottom;
      left = 0.0f;
    }
  }

  pb_print("%s\n", texture_format.name);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, texture_format.name);
}

std::string TextureSignedComponentTests::MakeTestName(const TextureFormatInfo &texture_format, uint32_t signed_flags) {
  std::string test_name = texture_format.name;
  if (texture_format.xbox_linear) {
    test_name += "_L";
  }

  char buf[16] = {0};
  sprintf(buf, "_0x%04X", signed_flags);
  test_name += buf;

  return std::move(test_name);
}

static int GenerateBlockTestSurface(SDL_Surface **surface, int width, int height) {
  int ret = GenerateCheckerboardSurface(surface, width, height, 0xFFFEFDFC, 0x7F202122);
  if (ret) {
    return ret;
  }

  if (SDL_LockSurface(*surface)) {
    SDL_FreeSurface(*surface);
    *surface = nullptr;
    return 2;
  }

  auto pixels = static_cast<uint32_t *>((*surface)->pixels);

  static constexpr int kLeft = 5;
  static constexpr int kTop = 10;
  int x = kLeft;
  int y = kTop;
  const int sub_width = width / 7;
  const int sub_height = height / 5;

  GenerateRGBACheckerboard(pixels, x, y, sub_width, sub_height, (*surface)->pitch, 0xFF0000FF, 0xFF00007F, 4);
  x += sub_width + 2;
  GenerateRGBACheckerboard(pixels, x, y, sub_width, sub_height, (*surface)->pitch, 0xFF000080, 0xFF000000, 4);
  x += sub_width + 2;
  GenerateRGBACheckerboard(pixels, x, y, sub_width, sub_height, (*surface)->pitch, 0x7F0000FF, 0x7F00007F, 4);
  x += sub_width + 2;
  GenerateRGBACheckerboard(pixels, x, y, sub_width, sub_height, (*surface)->pitch, 0x7F000080, 0x7F000000, 4);
  x += sub_width + 2;
  GenerateRGBACheckerboard(pixels, x, y, sub_width, sub_height, (*surface)->pitch, 0x000000FF, 0x0000007F, 4);
  x += sub_width + 2;
  GenerateRGBACheckerboard(pixels, x, y, sub_width, sub_height, (*surface)->pitch, 0x00000080, 0x00000000, 4);
  x = kLeft;

  y += sub_height + 2;
  GenerateRGBACheckerboard(pixels, x, y, sub_width, sub_height, (*surface)->pitch, 0xFF00FF00, 0xFF007F00, 4);
  x += sub_width + 2;
  GenerateRGBACheckerboard(pixels, x, y, sub_width, sub_height, (*surface)->pitch, 0xFF008000, 0xFF000000, 4);
  x += sub_width + 2;
  GenerateRGBACheckerboard(pixels, x, y, sub_width, sub_height, (*surface)->pitch, 0x7F00FF00, 0x7F007F00, 4);
  x += sub_width + 2;
  GenerateRGBACheckerboard(pixels, x, y, sub_width, sub_height, (*surface)->pitch, 0x7F008000, 0x7F000000, 4);
  x += sub_width + 2;
  GenerateRGBACheckerboard(pixels, x, y, sub_width, sub_height, (*surface)->pitch, 0x0000FF00, 0x00007F00, 4);
  x += sub_width + 2;
  GenerateRGBACheckerboard(pixels, x, y, sub_width, sub_height, (*surface)->pitch, 0x00008000, 0x00000000, 4);
  x = kLeft;

  y += sub_height + 2;
  GenerateRGBACheckerboard(pixels, x, y, sub_width, sub_height, (*surface)->pitch, 0xFFFF0000, 0xFF7F0000, 4);
  x += sub_width + 2;
  GenerateRGBACheckerboard(pixels, x, y, sub_width, sub_height, (*surface)->pitch, 0xFF800000, 0xFF000000, 4);
  x += sub_width + 2;
  GenerateRGBACheckerboard(pixels, x, y, sub_width, sub_height, (*surface)->pitch, 0x7FFF0000, 0x7F7F0000, 4);
  x += sub_width + 2;
  GenerateRGBACheckerboard(pixels, x, y, sub_width, sub_height, (*surface)->pitch, 0x7F800000, 0x7F000000, 4);
  x += sub_width + 2;
  GenerateRGBACheckerboard(pixels, x, y, sub_width, sub_height, (*surface)->pitch, 0x00FF0000, 0x007F0000, 4);
  x += sub_width + 2;
  GenerateRGBACheckerboard(pixels, x, y, sub_width, sub_height, (*surface)->pitch, 0x00800000, 0x00000000, 4);

  SDL_UnlockSurface(*surface);

  return 0;
}