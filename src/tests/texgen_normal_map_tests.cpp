#include "texgen_normal_map_tests.h"

#include <pbkit/pbkit.h>

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "models/light_control_test_mesh_suzanne_model.h"
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

struct MatrixConfig {
  const char *name;
  bool matrix_enable{true};
  std::function<void(matrix4_t &)> setup;
};

static void SetBaseNormalMatrix(matrix4_t &m) {
  MatrixSetIdentity(m);
  m[0][0] = 0.5f;
  m[1][1] = 0.5f;
  m[2][2] = 1.0f;
  m[3][0] = 0.5f;
  m[3][1] = 0.5f;
  m[3][3] = 1.0f;
}

static const MatrixConfig kMatrixConfigs[] = {
    {"MatrixOff", false,
     [](matrix4_t &m) {
       SetBaseNormalMatrix(m);
       m[0][0] = 1.0f;
       m[1][1] = 1.0f;
     }},
    {"Identity", true, [](matrix4_t &m) { SetBaseNormalMatrix(m); }},
    {"Double", true,
     [](matrix4_t &m) {
       SetBaseNormalMatrix(m);
       m[0][0] = 1.0f;
       m[1][1] = 1.0f;
     }},
    {"Half", true,
     [](matrix4_t &m) {
       SetBaseNormalMatrix(m);
       m[0][0] = 0.25f;
       m[1][1] = 0.25f;
     }},
    {"ShiftHPlus", true,
     [](matrix4_t &m) {
       SetBaseNormalMatrix(m);
       vector_t translate = {0.5f, 0.0f, 0.0f, 0.0f};
       MatrixTranslate(m, translate);
     }},
    {"ShiftHMinus", true,
     [](matrix4_t &m) {
       SetBaseNormalMatrix(m);
       vector_t translate = {-0.5f, 0.0f, 0.0f, 0.0f};
       MatrixTranslate(m, translate);
     }},
    {"ShiftVPlus", true,
     [](matrix4_t &m) {
       SetBaseNormalMatrix(m);
       vector_t translate = {0.0f, 0.5f, 0.0f, 0.0f};
       MatrixTranslate(m, translate);
     }},
    {"ShiftVMinus", true,
     [](matrix4_t &m) {
       SetBaseNormalMatrix(m);
       vector_t translate = {0.0f, -0.5f, 0.0f, 0.0f};
       MatrixTranslate(m, translate);
     }},
    {"RotateX", true,
     [](matrix4_t &m) {
       SetBaseNormalMatrix(m);
       vector_t rot = {static_cast<float>(M_PI * 0.5), 0.0f, 0.0f, 0.0f};
       MatrixRotate(m, rot);
     }},
    {"RotateY", true,
     [](matrix4_t &m) {
       SetBaseNormalMatrix(m);
       vector_t rot = {0.0f, static_cast<float>(M_PI * 0.5), 0.0f, 0.0f};
       MatrixRotate(m, rot);
     }},
    {"RotateZ", true,
     [](matrix4_t &m) {
       SetBaseNormalMatrix(m);
       vector_t rot = {0.0f, 0.0f, static_cast<float>(M_PI * 0.5), 0.0f};
       MatrixRotate(m, rot);
     }},
    {"Negate", true,
     [](matrix4_t &m) {
       SetBaseNormalMatrix(m);
       m[0][0] = -0.5f;
       m[1][1] = -0.5f;
     }},
    {"Arbitrary", true,
     [](matrix4_t &m) {
       matrix4_t arbitrary = {
           {0.7089392f, 0.0f, 0.515f, 0.0f},
           {0.0f, 1.2603364f, 0.49f, 0.0f},
           {0.0f, 0.0f, 0.0f, 0.0f},
           {0.0f, 0.0f, 1.0f, 0.0f},
       };
       MatrixCopyMatrix(m, arbitrary);
     }},
};

/**
 * Tests NormalMap TexGen with non-flat geometry.
 * Coordinates are generated from eye-space vertex normals, producing smooth variation across the surface.
 *
 * @tc Basic_InfiniteViewer
 *  Tests NormalMap TexGen with infinite viewer model and identity texture matrix under STAGE_2D_PROJECTIVE. Normal
 *  vectors are viewer-independent, producing the same output as Identity.
 *
 * @tc MatrixOff
 *  Tests NormalMap TexGen with texture matrix disabled under STAGE_2D_PROJECTIVE on a non-flat mesh. Untransformed
 *  [-1, 1] normals lack [0, 1] remap bias, sampling the texture boundary color.
 * @tc Identity
 *  Tests NormalMap TexGen with identity texture matrix under STAGE_2D_PROJECTIVE on a non-flat mesh.
 * @tc Double
 *  Tests NormalMap TexGen with 2x scaling texture matrix under STAGE_2D_PROJECTIVE on a non-flat mesh.
 * @tc Half
 *  Tests NormalMap TexGen with 0.5x scaling texture matrix under STAGE_2D_PROJECTIVE on a non-flat mesh.
 * @tc ShiftHPlus
 *  Tests NormalMap TexGen with +0.5 horizontal translation texture matrix under STAGE_2D_PROJECTIVE on a non-flat
 *  mesh.
 * @tc ShiftHMinus
 *  Tests NormalMap TexGen with -0.5 horizontal translation texture matrix under STAGE_2D_PROJECTIVE on a non-flat
 *  mesh.
 * @tc ShiftVPlus
 *  Tests NormalMap TexGen with +0.5 vertical translation texture matrix under STAGE_2D_PROJECTIVE on a non-flat
 *  mesh.
 * @tc ShiftVMinus
 *  Tests NormalMap TexGen with -0.5 vertical translation texture matrix under STAGE_2D_PROJECTIVE on a non-flat
 *  mesh.
 * @tc RotateX
 *  Tests NormalMap TexGen with 90 deg X-rotation texture matrix under STAGE_2D_PROJECTIVE on a non-flat mesh.
 * @tc RotateY
 *  Tests NormalMap TexGen with 90 deg Y-rotation texture matrix under STAGE_2D_PROJECTIVE on a non-flat mesh.
 * @tc RotateZ
 *  Tests NormalMap TexGen with 90 deg Z-rotation texture matrix under STAGE_2D_PROJECTIVE on a non-flat mesh.
 * @tc Negate
 *  Tests NormalMap TexGen with -1.0 negation texture matrix under STAGE_2D_PROJECTIVE on a non-flat mesh.
 * @tc Arbitrary
 *  Tests NormalMap TexGen with arbitrary 4x4 texture matrix under STAGE_2D_PROJECTIVE on a non-flat mesh.
 *
 * @tc PassThrough_MatrixOff
 *  Tests NormalMap TexGen with texture matrix disabled under STAGE_PASS_THROUGH summed with a positive texture on a
 * non-flat mesh.
 * @tc PassThrough_Identity
 *  Tests NormalMap TexGen with identity texture matrix under STAGE_PASS_THROUGH summed with a positive texture on a
 * non-flat mesh.
 * @tc PassThrough_Double
 *  Tests NormalMap TexGen with 2x scaling texture matrix under STAGE_PASS_THROUGH summed with a positive texture on a
 * non-flat mesh.
 * @tc PassThrough_Half
 *  Tests NormalMap TexGen with 0.5x scaling texture matrix under STAGE_PASS_THROUGH summed with a positive texture on a
 * non-flat mesh.
 * @tc PassThrough_ShiftHPlus
 *  Tests NormalMap TexGen with +0.5 horizontal translation texture matrix under STAGE_PASS_THROUGH summed with a
 *  positive texture on a non-flat mesh.
 * @tc PassThrough_ShiftHMinus
 *  Tests NormalMap TexGen with -0.5 horizontal translation texture matrix under STAGE_PASS_THROUGH summed with a
 *  positive texture on a non-flat mesh, evaluating coordinate clamping across curved surface normals.
 * @tc PassThrough_ShiftVPlus
 *  Tests NormalMap TexGen with +0.5 vertical translation texture matrix under STAGE_PASS_THROUGH summed with a positive
 *  texture on a non-flat mesh.
 * @tc PassThrough_ShiftVMinus
 *  Tests NormalMap TexGen with -0.5 vertical translation texture matrix under STAGE_PASS_THROUGH summed with a positive
 *  texture on a non-flat mesh, evaluating coordinate clamping across curved surface normals.
 * @tc PassThrough_RotateX
 *  Tests NormalMap TexGen with 90 deg X-rotation texture matrix under STAGE_PASS_THROUGH summed with a positive texture
 *  on a non-flat mesh.
 * @tc PassThrough_RotateY
 *  Tests NormalMap TexGen with 90 deg Y-rotation texture matrix under STAGE_PASS_THROUGH summed with a positive texture
 *  on a non-flat mesh.
 * @tc PassThrough_RotateZ
 *  Tests NormalMap TexGen with 90 deg Z-rotation texture matrix under STAGE_PASS_THROUGH summed with a positive texture
 *  on a non-flat mesh.
 * @tc PassThrough_Negate
 *  Tests NormalMap TexGen with -1.0 negation texture matrix under STAGE_PASS_THROUGH summed with a positive texture on
 *  a non-flat mesh, evaluating whether negative coordinates clamp to zero or darken below the positive texture.
 * @tc PassThrough_Arbitrary
 *  Tests NormalMap TexGen with arbitrary 4x4 texture matrix under STAGE_PASS_THROUGH summed with a positive texture on
 *  a non-flat mesh.
 */
TexgenNormalMapTests::TexgenNormalMapTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Texgen normal map", config) {
  // Infinite viewer model test
  tests_["Basic_InfiniteViewer"] = [this]() {
    matrix4_t matrix;
    SetBaseNormalMatrix(matrix);
    Test("Basic_InfiniteViewer", matrix, true, TestHost::STAGE_2D_PROJECTIVE,
         NV097_SET_TEXGEN_VIEW_MODEL_INFINITE_VIEWER);
  };

  // STAGE_2D_PROJECTIVE matrix tests
  for (const auto &cfg : kMatrixConfigs) {
    std::string test_name = cfg.name;
    tests_[test_name] = [this, test_name, &cfg]() {
      matrix4_t matrix;
      cfg.setup(matrix);
      Test(test_name, matrix, cfg.matrix_enable, TestHost::STAGE_2D_PROJECTIVE,
           NV097_SET_TEXGEN_VIEW_MODEL_LOCAL_VIEWER);
    };
  }

  // STAGE_PASS_THROUGH matrix tests
  for (const auto &cfg : kMatrixConfigs) {
    std::string test_name = std::string("PassThrough_") + cfg.name;
    tests_[test_name] = [this, test_name, &cfg]() {
      matrix4_t matrix;
      cfg.setup(matrix);
      Test(test_name, matrix, cfg.matrix_enable, TestHost::STAGE_PASS_THROUGH,
           NV097_SET_TEXGEN_VIEW_MODEL_LOCAL_VIEWER);
    };
  }
}

void TexgenNormalMapTests::Initialize() {
  TestSuite::Initialize();

  auto model = LightControlTestMeshSuzanneModel();
  vertex_buffer_ = host_.AllocateVertexBuffer(model.GetVertexCount());
  model.PopulateVertexBuffer(vertex_buffer_);

  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  host_.SetTextureStageEnabled(0, true);
  host_.SetTextureStageEnabled(1, false);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  // Stage 0: 2D projective gradient texture (used by STAGE_2D_PROJECTIVE tests)
  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R8G8B8A8), 0);
  {
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetBorderColor(0xFF7F007F);
    texture_stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
    texture_stage.SetUWrap(TextureStage::WRAP_REPEAT);
    texture_stage.SetVWrap(TextureStage::WRAP_REPEAT);
  }

  GenerateSwizzledRGBATestPattern(host_.GetTextureMemoryForStage(0), kTextureWidth, kTextureHeight);

  // Stage 1: Positive checkerboard texture (used by STAGE_PASS_THROUGH tests)
  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R8G8B8A8), 1);
  {
    auto &texture_stage = host_.GetTextureStage(1);
    texture_stage.SetBorderColor(0xFF7F007F);
    texture_stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
    texture_stage.SetTexgenS(TextureStage::TG_DISABLE);
    texture_stage.SetTexgenT(TextureStage::TG_DISABLE);
    texture_stage.SetTexgenR(TextureStage::TG_DISABLE);
    texture_stage.SetTextureMatrixEnable(false);
  }

  GenerateSwizzledRGBACheckerboard(host_.GetTextureMemoryForStage(1), 0, 0, kTextureWidth, kTextureHeight,
                                   kTextureWidth * 4, 0xFF606060, 0xFF909090);

  host_.SetCombinerControl(1, true, true);
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
}

void TexgenNormalMapTests::Deinitialize() {
  auto &stage0 = host_.GetTextureStage(0);
  stage0.SetUWrap(TextureStage::WRAP_CLAMP_TO_EDGE);
  stage0.SetVWrap(TextureStage::WRAP_CLAMP_TO_EDGE);
  stage0.SetTexgenS(TextureStage::TG_DISABLE);
  stage0.SetTexgenT(TextureStage::TG_DISABLE);
  stage0.SetTexgenR(TextureStage::TG_DISABLE);
  stage0.SetTextureMatrixEnable(false);

  auto &stage1 = host_.GetTextureStage(1);
  stage1.SetTexgenS(TextureStage::TG_DISABLE);
  stage1.SetTexgenT(TextureStage::TG_DISABLE);
  stage1.SetTexgenR(TextureStage::TG_DISABLE);
  stage1.SetTextureMatrixEnable(false);

  host_.SetTextureStageEnabled(0, false);
  host_.SetTextureStageEnabled(1, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);
  host_.SetupTextureStages();

  vertex_buffer_.reset();
  TestSuite::Deinitialize();
}

void TexgenNormalMapTests::Test(const std::string &test_name, const matrix4_t &matrix, bool matrix_enable,
                                TestHost::ShaderStageProgram stage_program, uint32_t view_model) {
  host_.PrepareDraw(0xFE202020);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_TEXGEN_VIEW_MODEL, view_model);
  Pushbuffer::End();

  if (stage_program == TestHost::STAGE_PASS_THROUGH) {
    host_.SetTextureStageEnabled(1, true);
    host_.SetShaderStageProgram(TestHost::STAGE_PASS_THROUGH, TestHost::STAGE_2D_PROJECTIVE);

    host_.SetCombinerControl(1, true, true);
    // Combiner stage 0:
    // AB = Tex1 (positive texture) * 1.0
    // CD = SIGNED_IDENTITY(Tex0) (pass-through coordinates) * 1.0
    // Sum = AB + CD = Tex1 + SIGNED_IDENTITY(Tex0)
    host_.SetInputColorCombiner(0, TestHost::ColorInput(TestHost::SRC_TEX1, TestHost::MAP_UNSIGNED_IDENTITY),
                                TestHost::ColorInput(TestHost::SRC_ZERO, TestHost::MAP_UNSIGNED_INVERT),
                                TestHost::ColorInput(TestHost::SRC_TEX0, TestHost::MAP_SIGNED_IDENTITY),
                                TestHost::ColorInput(TestHost::SRC_ZERO, TestHost::MAP_UNSIGNED_INVERT));
    host_.SetOutputColorCombiner(0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0, false, false,
                                 TestHost::SM_SUM);
    host_.SetInputAlphaCombiner(0, TestHost::ZeroInput(), TestHost::ZeroInput());
    host_.SetOutputAlphaCombiner(0, TestHost::DST_DISCARD);

    host_.SetFinalCombiner0Just(TestHost::SRC_R0);
    host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
  } else {
    host_.SetTextureStageEnabled(1, false);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

    host_.SetCombinerControl(1, true, true);
    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
  }

  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetTexgenS(TextureStage::TG_NORMAL_MAP);
  texture_stage.SetTexgenT(TextureStage::TG_NORMAL_MAP);
  texture_stage.SetTexgenR(TextureStage::TG_NORMAL_MAP);
  texture_stage.SetTextureMatrixEnable(matrix_enable);
  MatrixCopyMatrix(texture_stage.GetTextureMatrix(), matrix);
  if (!matrix_enable) {
    Pushbuffer::Begin();
    Pushbuffer::Push4x4Matrix(NV097_SET_TEXTURE_MATRIX, matrix[0]);
    Pushbuffer::End();
  }

  host_.SetupTextureStages();
  host_.SetVertexBuffer(vertex_buffer_);
  host_.DrawArrays(TestHost::POSITION | TestHost::NORMAL | TestHost::DIFFUSE | TestHost::SPECULAR);

  pb_print("%s\n", test_name.c_str());
  if (matrix_enable) {
    const float *val = texture_stage.GetTextureMatrix()[0];
    for (auto i = 0; i < 4; ++i, val += 4) {
      pb_print_with_floats("%.3f %.3f %.3f %.3f\n", val[0], val[1], val[2], val[3]);
    }
  } else {
    pb_print("Matrix: Disabled\n");
  }
  pb_draw_text_screen();

  FinishDraw(test_name);
}
