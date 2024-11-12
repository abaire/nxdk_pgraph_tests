#include "bump_map_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <array>
#include <memory>
#include <utility>

#include "debug_output.h"
#include "shaders/passthrough_vertex_shader.h"
#include "test_host.h"
#include "texture_format.h"
#include "texture_generator.h"
#include "vertex_buffer.h"

static int GenerateBumpMapSurface(SDL_Surface **bump_surface, int width, int height,
                                  const std::array<uint32_t, 4> &colors);

static bool SkipGenericTest(const TextureFormatInfo &format) {
  switch (format.xbox_format) {
    case NV097_SET_TEXTURE_FORMAT_COLOR_SZ_I8_A8R8G8B8:
    case NV097_SET_TEXTURE_FORMAT_COLOR_SZ_G8B8:
    case NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R16B16:
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

BumpMapTests::BumpMapTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Bump map", config) {
  for (auto i = 0; i < kNumFormats; ++i) {
    auto &format = kTextureFormats[i];
    if (!SkipGenericTest(format)) {
      std::string name = MakeTestName(format, false, false);
      tests_[name] = [this, format]() { Test(format, false, false); };
    }
  }

  auto &format = GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_G8B8);

  for (auto cross_on_blue : {false, true}) {
    for (auto rotate90 : {false, true}) {
      std::string name = MakeTestName(format, cross_on_blue, rotate90);
      tests_[name] = [this, format, cross_on_blue, rotate90]() { Test(format, cross_on_blue, rotate90); };
    }
  }

  auto &format16 = GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R16B16);

  for (auto cross_on_blue : {false, true}) {
    std::string name = MakeTestName(format16, cross_on_blue, false);
    tests_[name] = [this, format16, cross_on_blue]() { Test16bit(format16, cross_on_blue); };
  }
}

void BumpMapTests::Initialize() { TestSuite::Initialize(); }

void BumpMapTests::Test(const TextureFormatInfo &texture_format, bool cross_on_blue, bool rotate90) {
  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  host_.SetTextureStageEnabled(0, true);
  host_.SetTextureStageEnabled(1, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE, TestHost::STAGE_BUMPENVMAP);
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX1);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX1, true);

  std::array<uint32_t, 4> bump_colors = {0x007f4500, 0x00804500, 0x007f4500, 0x00804500};
  if (cross_on_blue) {
    bump_colors = {0x00457f00, 0x00457f00, 0x00458000, 0x00458000};
  }

  SDL_Surface *texture_surface;
  int update_texture_result = GenerateBumpMapSurface(&texture_surface, (int)host_.GetMaxTextureWidth(),
                                                     (int)host_.GetMaxTextureHeight(), bump_colors);
  ASSERT(!update_texture_result && "Failed to generate bump SDL surface");

  host_.SetTextureFormat(texture_format, 0);
  update_texture_result = host_.SetTexture(texture_surface, 0);
  SDL_FreeSurface(texture_surface);
  ASSERT(!update_texture_result && "Failed to bump map texture");

  update_texture_result = GenerateCheckerboardSurface(&texture_surface, (int)host_.GetMaxTextureWidth(),
                                                      (int)host_.GetMaxTextureHeight(), 0xFF0000FE, 0x7F202122);
  ASSERT(!update_texture_result && "Failed to generate SDL surface");

  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8), 1);
  update_texture_result = host_.SetTexture(texture_surface, 1);
  SDL_FreeSurface(texture_surface);
  ASSERT(!update_texture_result && "Failed to set texture");
  /*
  {
    auto &stage = host_.GetTextureStage(1);
    stage.SetFilter(0, TextureStage::K_QUINCUNX, TextureStage::MIN_TENT_LOD0, TextureStage::MAG_TENT_LOD0);
  }
  */

  {
    auto &stage = host_.GetTextureStage(1);
    if (rotate90) {
      stage.SetBumpEnv(0.0, -0.1, 0.3, 0.0, 0.0, 0.0);
    } else {
      stage.SetBumpEnv(0.3, 0.0, 0.0, 0.5, 0.0, 0.0);
      // stage.SetBumpEnv(0.3, 0.0, 0.0, 5.0, 0.0, 0.0);
    }
  }

  auto draw = [this, texture_format](float left, float top, float right, float bottom) {
    float w = (float)host_.GetMaxTextureWidth();
    float h = (float)host_.GetMaxTextureHeight();

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    if (texture_format.xbox_linear) {
      host_.SetTexCoord0(1.0f, 1.0f);
    } else {
      host_.SetTexCoord0(1.0f / w, 1.0f / h);
    }
    host_.SetTexCoord1(0.0f, 0.0f);
    host_.SetVertex(left, top, 1.0f);

    if (texture_format.xbox_linear) {
      host_.SetTexCoord0(3.0f, 1.0f);
    } else {
      host_.SetTexCoord0(3.0f / w, 1.0f / h);
    }
    host_.SetTexCoord1(1.0f, 0.0f);
    host_.SetVertex(right, top, 1.0f);

    if (texture_format.xbox_linear) {
      host_.SetTexCoord0(3.0f, 3.0f);
    } else {
      host_.SetTexCoord0(3.0f / w, 3.0f / h);
    }
    host_.SetTexCoord1(1.0f, 1.0f);
    host_.SetVertex(right, bottom, 1.0f);

    if (texture_format.xbox_linear) {
      host_.SetTexCoord0(1.0f, 3.0f);
    } else {
      host_.SetTexCoord0(1.0f / w, 3.0f / h);
    }
    host_.SetTexCoord1(0.0f, 1.0f);
    host_.SetVertex(left, bottom, 1.0f);
    host_.End();

    while (pb_busy()) {
      /* Wait for completion... */
    }
  };

  DrawRectangles(texture_format, 'B', 'G', cross_on_blue, rotate90, draw);
  host_.SetTextureStageEnabled(0, false);
  host_.SetTextureStageEnabled(1, false);
}

void BumpMapTests::DrawRectangles(const TextureFormatInfo &texture_format, char x_channel, char y_channel,
                                  bool cross_on_blue, bool rotate90,
                                  const std::function<void(float, float, float, float)> &draw) {
  host_.PrepareDraw(0xFE202020);

  int w = host_.GetFramebufferWidth();
  int h = host_.GetFramebufferHeight();
  int bh = h * 3 / 8;
  int th = bh * 120 / 128;
  int sh = (bh - th) / 2;
  int sx = w / 2 - bh + sh;
  int sy = h / 2 - bh + sh;

  for (auto ysigned : {0, 1}) {
    for (auto xsigned : {0, 1}) {
      {
        bool rsigned = false;
        bool gsigned = (x_channel == 'B' && y_channel == 'G') ? !!ysigned : !!xsigned;
        bool bsigned = (x_channel == 'B' && y_channel == 'G') ? !!xsigned : false;
        bool asigned = (x_channel == 'B' && y_channel == 'G') ? false : !!ysigned;
        auto &stage = host_.GetTextureStage(0);
        stage.SetFilter(0, TextureStage::K_QUINCUNX, TextureStage::MIN_TENT_LOD0, TextureStage::MAG_TENT_LOD0, asigned,
                        rsigned, gsigned, bsigned);
      }
      host_.SetupTextureStages();

      int left = sx + xsigned * bh;
      int top = sy + ysigned * bh;
      int right = left + th;
      int bottom = top + th;
      draw(left, top, right, bottom);
      pb_printat(5 + 7 * ysigned, 17 + 20 * xsigned, "%c%c %c%c", x_channel, xsigned ? 's' : 'u', y_channel,
                 ysigned ? 's' : 'u');
    }
  }

  pb_printat(0, 0, "N: %s\n", texture_format.name);
  pb_print("F: 0x%x\n", texture_format.xbox_format);
  pb_print("SZ: %d\n", texture_format.xbox_swizzled);
  pb_print("CrossB: %d\n", cross_on_blue);
  pb_print("Rot90: %d\n", rotate90);
  pb_draw_text_screen();

  std::string test_name = MakeTestName(texture_format, cross_on_blue, rotate90);
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, test_name);
}

void BumpMapTests::Test16bit(const TextureFormatInfo &texture_format, bool cross_on_blue) {
  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  host_.SetTextureStageEnabled(0, true);
  host_.SetTextureStageEnabled(1, true);
  host_.SetTextureStageEnabled(2, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE, TestHost::STAGE_DOT_PRODUCT, TestHost::STAGE_DOT_ST);
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX2);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX2, true);
  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_DOT_RGBMAPPING, 0x044);  // dotmap_hilo_1 for stages 1 and 2.
    pb_end(p);
  }

  std::array<uint32_t, 4> bump_colors = {0x02000100, 0x02000300, 0xff000100, 0xff000300};
  if (cross_on_blue) {
    bump_colors = {0x02000300, 0x0200ff00, 0x00000300, 0x0000ff00};
  }

  SDL_Surface *texture_surface;
  int update_texture_result = GenerateBumpMapSurface(&texture_surface, (int)host_.GetMaxTextureWidth(),
                                                     (int)host_.GetMaxTextureHeight(), bump_colors);
  ASSERT(!update_texture_result && "Failed to generate bump SDL surface");

  host_.SetTextureFormat(texture_format, 0);
  update_texture_result = host_.SetTexture(texture_surface, 0);
  SDL_FreeSurface(texture_surface);
  ASSERT(!update_texture_result && "Failed to bump map texture");

  update_texture_result = GenerateCheckerboardSurface(&texture_surface, (int)host_.GetMaxTextureWidth(),
                                                      (int)host_.GetMaxTextureHeight(), 0xFF0000FE, 0x7F202122);
  ASSERT(!update_texture_result && "Failed to generate SDL surface");

  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8), 2);
  update_texture_result = host_.SetTexture(texture_surface, 2);
  SDL_FreeSurface(texture_surface);
  ASSERT(!update_texture_result && "Failed to set texture");

  auto draw = [this, texture_format](float left, float top, float right, float bottom) {
    float w = (float)host_.GetMaxTextureWidth();
    float h = (float)host_.GetMaxTextureHeight();

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    if (texture_format.xbox_linear) {
      host_.SetTexCoord0(1.0f, 1.0f);
    } else {
      host_.SetTexCoord0(1.0f / w, 1.0f / h);
    }
    host_.SetTexCoord1(10.0f, 0.0f, 0.0f, 1.0f);
    host_.SetTexCoord2(0.0f, 15.0f, 0.0f, 1.0f);
    host_.SetVertex(left, top, 1.0f);

    if (texture_format.xbox_linear) {
      host_.SetTexCoord0(3.0f, 1.0f);
    } else {
      host_.SetTexCoord0(3.0f / w, 1.0f / h);
    }
    host_.SetTexCoord1(10.0f, 0.0f, 1.0f, 1.0f);
    host_.SetTexCoord2(0.0f, 15.0f, 0.0f, 1.0f);
    host_.SetVertex(right, top, 1.0f);

    if (texture_format.xbox_linear) {
      host_.SetTexCoord0(3.0f, 3.0f);
    } else {
      host_.SetTexCoord0(3.0f / w, 3.0f / h);
    }
    host_.SetTexCoord1(10.0f, 0.0f, 1.0f, 1.0f);
    host_.SetTexCoord2(0.0f, 15.0f, 1.0f, 1.0f);
    host_.SetVertex(right, bottom, 1.0f);

    if (texture_format.xbox_linear) {
      host_.SetTexCoord0(1.0f, 3.0f);
    } else {
      host_.SetTexCoord0(1.0f / w, 3.0f / h);
    }
    host_.SetTexCoord1(10.0f, 0.0f, 0.0f, 1.0f);
    host_.SetTexCoord2(0.0f, 15.0f, 1.0f, 1.0f);
    host_.SetVertex(left, bottom, 1.0f);
    host_.End();

    while (pb_busy()) {
      /* Wait for completion... */
    }
  };

  DrawRectangles(texture_format, 'G', 'A', cross_on_blue, false, draw);
  host_.SetTextureStageEnabled(0, false);
  host_.SetTextureStageEnabled(1, false);
  host_.SetTextureStageEnabled(2, false);
}

std::string BumpMapTests::MakeTestName(const TextureFormatInfo &texture_format, bool cross_on_blue, bool rotate90) {
  std::string test_name = "BumpMap_";

  test_name += texture_format.name;
  if (texture_format.xbox_linear) {
    test_name += "_L";
  }
  if (cross_on_blue) {
    test_name += "_B";
  }
  if (rotate90) {
    test_name += "_R90";
  }

  return test_name;
}

static int GenerateBumpMapSurface(SDL_Surface **bump_surface, int width, int height,
                                  const std::array<uint32_t, 4> &colors) {
  *bump_surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
  if (!(*bump_surface)) {
    return 1;
  }

  if (SDL_LockSurface(*bump_surface)) {
    SDL_FreeSurface(*bump_surface);
    *bump_surface = nullptr;
    return 2;
  }

  auto pixels = static_cast<uint32_t *>((*bump_surface)->pixels);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      *pixels++ = colors[(y >= 2) * 2 + (x >= 2)];
    }
  }

  SDL_UnlockSurface(*bump_surface);

  return 0;
}
