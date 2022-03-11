#include "texture_matrix_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "test_host.h"
#include "texture_format.h"
#include "vertex_buffer.h"

static int GenerateSurface(SDL_Surface **surface, int width, int height);

static constexpr int kTextureWidth = 256;
static constexpr int kTextureHeight = 128;

TextureMatrixTests::TextureMatrixTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Texture Matrix") {
  {
    constexpr char kTestName[] = "Identity";
    tests_[kTestName] = [this, kTestName]() {
      MATRIX matrix;
      matrix_unit(matrix);
      Test(kTestName, matrix);
    };
  }
  {
    constexpr char kTestName[] = "Double";
    tests_[kTestName] = [this, kTestName]() {
      MATRIX matrix;
      matrix_unit(matrix);
      VECTOR scale = {2.0, 2.0, 2.0, 1.0};
      matrix_scale(matrix, matrix, scale);
      Test(kTestName, matrix);
    };
  }
  {
    constexpr char kTestName[] = "Half";
    tests_[kTestName] = [this, kTestName]() {
      MATRIX matrix;
      matrix_unit(matrix);
      VECTOR scale = {0.5, 0.5, 0.5, 1.0};
      matrix_scale(matrix, matrix, scale);
      Test(kTestName, matrix);
    };
  }
  {
    constexpr char kTestName[] = "ShiftHPlus";
    tests_[kTestName] = [this, kTestName]() {
      MATRIX matrix;
      matrix_unit(matrix);
      VECTOR translate = {0.5, 0.0, 0.0, 0.0};
      matrix_translate(matrix, matrix, translate);
      Test(kTestName, matrix);
    };
  }
  {
    constexpr char kTestName[] = "ShiftHMinus";
    tests_[kTestName] = [this, kTestName]() {
      MATRIX matrix;
      matrix_unit(matrix);
      VECTOR translate = {-0.5, 0.0, 0.0, 0.0};
      matrix_translate(matrix, matrix, translate);
      Test(kTestName, matrix);
    };
  }
  {
    constexpr char kTestName[] = "ShiftVPlus";
    tests_[kTestName] = [this, kTestName]() {
      MATRIX matrix;
      matrix_unit(matrix);
      VECTOR translate = {0.0, 0.5, 0.0, 0.0};
      matrix_translate(matrix, matrix, translate);
      Test(kTestName, matrix);
    };
  }
  {
    constexpr char kTestName[] = "ShiftVMinus";
    tests_[kTestName] = [this, kTestName]() {
      MATRIX matrix;
      matrix_unit(matrix);
      VECTOR translate = {0.0, -0.5, 0.0, 0.0};
      matrix_translate(matrix, matrix, translate);
      Test(kTestName, matrix);
    };
  }
  {
    constexpr char kTestName[] = "RotateX";
    tests_[kTestName] = [this, kTestName]() {
      MATRIX matrix;
      matrix_unit(matrix);
      VECTOR rot = {M_PI * 0.5, 0.0, 0.0, 0.0};
      matrix_rotate(matrix, matrix, rot);
      Test(kTestName, matrix);
    };
  }
  {
    constexpr char kTestName[] = "RotateY";
    tests_[kTestName] = [this, kTestName]() {
      MATRIX matrix;
      matrix_unit(matrix);
      VECTOR rot = {0.0, M_PI * 0.5, 0.0, 0.0};
      matrix_rotate(matrix, matrix, rot);
      Test(kTestName, matrix);
    };
  }
  {
    constexpr char kTestName[] = "RotateZ";
    tests_[kTestName] = [this, kTestName]() {
      MATRIX matrix;
      matrix_unit(matrix);
      VECTOR rot = {0.0, 0.0, M_PI * 0.5, 0.0};
      matrix_rotate(matrix, matrix, rot);
      Test(kTestName, matrix);
    };
  }
}

void TextureMatrixTests::Initialize() {
  TestSuite::Initialize();
  CreateGeometry();

  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R8G8B8A8));
  {
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetBorderColor(0xFF7F007F);
    texture_stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
    texture_stage.SetTextureMatrixEnable(true);
  }

  SDL_Surface *gradient_surface;
  int update_texture_result = GenerateSurface(&gradient_surface, kTextureWidth, kTextureHeight);
  ASSERT(!update_texture_result && "Failed to generate SDL surface");
  update_texture_result = host_.SetTexture(gradient_surface, 0);
  SDL_FreeSurface(gradient_surface);
  ASSERT(!update_texture_result && "Failed to set texture");

  host_.SetCombinerControl(1, true, true);
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
}

void TextureMatrixTests::CreateGeometry() {
  static constexpr float left = -2.75f;
  static constexpr float right = 2.75f;
  static constexpr float top = 1.0f;
  static constexpr float bottom = -2.5f;

  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(6);

  buffer->DefineBiTri(0, left, top, right, bottom);
}

void TextureMatrixTests::Test(const char *test_name, MATRIX matrix) {
  host_.PrepareDraw(0xFE202020);

  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetTextureMatrix(matrix);

  host_.SetupTextureStages();
  host_.DrawArrays();

  pb_print("%s\n", test_name);

  const float *val = matrix;
  for (auto i = 0; i < 4; ++i, val += 4) {
    pb_print_with_floats("%.3f %.3f %.3f %.3f\n", val[0], val[1], val[2], val[3]);
  }
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, test_name);
}

static int GenerateSurface(SDL_Surface **surface, int width, int height) {
  *surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
  if (!(*surface)) {
    return 1;
  }

  if (SDL_LockSurface(*surface)) {
    SDL_FreeSurface(*surface);
    *surface = nullptr;
    return 2;
  }

  auto pixels = static_cast<uint32_t *>((*surface)->pixels);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x, ++pixels) {
      int x_normal = static_cast<int>(static_cast<float>(x) * 255.0f / static_cast<float>(width));
      int y_normal = static_cast<int>(static_cast<float>(y) * 255.0f / static_cast<float>(height));
      *pixels = SDL_MapRGBA((*surface)->format, y_normal, x_normal, 255 - y_normal, x_normal + y_normal);
    }
  }

  SDL_UnlockSurface(*surface);

  return 0;
}
