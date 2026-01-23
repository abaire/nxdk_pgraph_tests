#include "texgen_matrix_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "test_host.h"
#include "texture_format.h"
#include "texture_generator.h"
#include "vertex_buffer.h"
#include "xbox_math_matrix.h"
#include "xbox_math_types.h"

using namespace XboxMath;

static constexpr int kTextureWidth = 256;
static constexpr int kTextureHeight = 128;

static std::string TestNameForTexGenMode(TextureStage::TexGen mode);

static TextureStage::TexGen kTestModes[] = {
    TextureStage::TG_DISABLE,    TextureStage::TG_EYE_LINEAR, TextureStage::TG_OBJECT_LINEAR,
    TextureStage::TG_SPHERE_MAP, TextureStage::TG_NORMAL_MAP, TextureStage::TG_REFLECTION_MAP,
};

TexgenMatrixTests::TexgenMatrixTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Texgen with texture matrix", config) {
  for (auto mode : kTestModes) {
    std::string name = TestNameForTexGenMode(mode);
    {
      std::string test_name = name + "_Identity";
      tests_[test_name] = [this, test_name, mode]() {
        matrix4_t matrix;
        MatrixSetIdentity(matrix);
        Test(test_name, matrix, mode);
      };
    }
    {
      std::string test_name = name + "_Double";
      tests_[test_name] = [this, test_name, mode]() {
        matrix4_t matrix;
        MatrixSetIdentity(matrix);
        vector_t scale = {2.0, 2.0, 2.0, 1.0};
        MatrixScale(matrix, scale);
        Test(test_name, matrix, mode);
      };
    }
    {
      std::string test_name = name + "_Half";
      tests_[test_name] = [this, test_name, mode]() {
        matrix4_t matrix;
        MatrixSetIdentity(matrix);
        vector_t scale = {0.5, 0.5, 0.5, 1.0};
        MatrixScale(matrix, scale);
        Test(test_name, matrix, mode);
      };
    }
    {
      std::string test_name = name + "_ShiftHPlus";
      tests_[test_name] = [this, test_name, mode]() {
        matrix4_t matrix;
        MatrixSetIdentity(matrix);
        vector_t translate = {0.5, 0.0, 0.0, 0.0};
        MatrixTranslate(matrix, translate);
        Test(test_name, matrix, mode);
      };
    }
    {
      std::string test_name = name + "_ShiftHMinus";
      tests_[test_name] = [this, test_name, mode]() {
        matrix4_t matrix;
        MatrixSetIdentity(matrix);
        vector_t translate = {-0.5, 0.0, 0.0, 0.0};
        MatrixTranslate(matrix, translate);
        Test(test_name, matrix, mode);
      };
    }
    {
      std::string test_name = name + "_ShiftVPlus";
      tests_[test_name] = [this, test_name, mode]() {
        matrix4_t matrix;
        MatrixSetIdentity(matrix);
        vector_t translate = {0.0, 0.5, 0.0, 0.0};
        MatrixTranslate(matrix, translate);
        Test(test_name, matrix, mode);
      };
    }
    {
      std::string test_name = name + "_ShiftVMinus";
      tests_[test_name] = [this, test_name, mode]() {
        matrix4_t matrix;
        MatrixSetIdentity(matrix);
        vector_t translate = {0.0, -0.5, 0.0, 0.0};
        MatrixTranslate(matrix, translate);
        Test(test_name, matrix, mode);
      };
    }
    {
      std::string test_name = name + "_RotateX";
      tests_[test_name] = [this, test_name, mode]() {
        matrix4_t matrix;
        MatrixSetIdentity(matrix);
        vector_t rot = {M_PI * 0.5, 0.0, 0.0, 0.0};
        MatrixRotate(matrix, rot);
        Test(test_name, matrix, mode);
      };
    }
    {
      std::string test_name = name + "_RotateY";
      tests_[test_name] = [this, test_name, mode]() {
        matrix4_t matrix;
        MatrixSetIdentity(matrix);
        vector_t rot = {0.0, M_PI * 0.5, 0.0, 0.0};
        MatrixRotate(matrix, rot);
        Test(test_name, matrix, mode);
      };
    }
    {
      std::string test_name = name + "_RotateZ";
      tests_[test_name] = [this, test_name, mode]() {
        matrix4_t matrix;
        MatrixSetIdentity(matrix);
        vector_t rot = {0.0, 0.0, M_PI * 0.5, 0.0};
        MatrixRotate(matrix, rot);
        Test(test_name, matrix, mode);
      };
    }
    {
      std::string test_name = name + "_Arbitrary";
      tests_[test_name] = [this, test_name, mode]() {
        matrix4_t matrix = {
            {0.7089392, 0.0, 0.515, 0.0},
            {0.0, 1.2603364, 0.49, 0.0},
            {0.0, 0.0, 0.0, 0.0},
            {0.0, 0.0, 1.0, 0.0},
        };
        Test(test_name, matrix, mode);
      };
    }
  }
}

void TexgenMatrixTests::Initialize() {
  TestSuite::Initialize();
  CreateGeometry();

  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R8G8B8A8));
  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetBorderColor(0xFF7F007F);
  texture_stage.SetTextureDimensions(kTextureWidth, kTextureHeight);

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

void TexgenMatrixTests::CreateGeometry() {
  static constexpr float left = -2.75f;
  static constexpr float right = 2.75f;
  static constexpr float top = 1.0f;
  static constexpr float bottom = -2.5f;

  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(6);

  buffer->DefineBiTri(0, left, top, right, bottom);
}

void TexgenMatrixTests::Test(const std::string &test_name, const matrix4_t &matrix, TextureStage::TexGen gen_mode) {
  host_.PrepareDraw(0xFE202020);

  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetTexgenS(gen_mode);
  texture_stage.SetTexgenT(gen_mode);
  texture_stage.SetTexgenR(gen_mode);
  texture_stage.SetTextureMatrixEnable(true);
  MatrixCopyMatrix(texture_stage.GetTextureMatrix(), matrix);

  host_.SetupTextureStages();
  host_.DrawArrays();

  pb_print("%s\n", test_name.c_str());
  const float *val = texture_stage.GetTextureMatrix()[0];
  for (auto i = 0; i < 4; ++i, val += 4) {
    pb_print_with_floats("%.3f %.3f %.3f %.3f\n", val[0], val[1], val[2], val[3]);
  }
  pb_draw_text_screen();

  FinishDraw(test_name);
}

static std::string TestNameForTexGenMode(TextureStage::TexGen mode) {
  switch (mode) {
    case TextureStage::TG_DISABLE:
      return "Disabled";

    case TextureStage::TG_EYE_LINEAR:
      return "EyeLinear";

    case TextureStage::TG_OBJECT_LINEAR:
      return "ObjectLinear";

    case TextureStage::TG_SPHERE_MAP:
      return "SphereMap";

    case TextureStage::TG_NORMAL_MAP:
      return "NormalMap";

    case TextureStage::TG_REFLECTION_MAP:
      return "ReflectionMap";
  }
}
