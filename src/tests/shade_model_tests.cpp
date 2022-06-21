#include "shade_model_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/perspective_vertex_shader.h"
#include "shaders/precalculated_vertex_shader.h"
#include "texture_generator.h"
#include "vertex_buffer.h"

static constexpr uint32_t kShadeModel[] = {
    NV097_SET_SHADE_MODEL_FLAT,
    NV097_SET_SHADE_MODEL_SMOOTH,
};

static constexpr const char kFixedUntextured[] = "Fixed";
static constexpr const char kFixedTextured[] = "FixedTex";
static constexpr const char kUntextured[] = "Prog";
static constexpr const char kTextured[] = "ProgTex";

constexpr uint32_t kTextureSize = 64;

std::string MakeTestName(const char *prefix, uint32_t shade_model);

static void SetLightAndMaterial() {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS, 0xbf34dce5);
  p = pb_push1(p, 0x09e4, 0xc020743f);
  p = pb_push1(p, 0x09e8, 0x40333d06);
  p = pb_push1(p, 0x09ec, 0xbf003612);
  p = pb_push1(p, 0x09f0, 0xbff852a5);
  p = pb_push1(p, 0x09f4, 0x401c1bce);

  p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_ALL_FROM_MATERIAL);
  p = pb_push3(p, NV097_SET_SCENE_AMBIENT_COLOR, 0x0, 0x0, 0x0);
  p = pb_push3(p, NV097_SET_MATERIAL_EMISSION, 0x0, 0x0, 0x0);
  p = pb_push1f(p, NV097_SET_MATERIAL_ALPHA, 1.0f);

  p = pb_push3(p, NV097_SET_LIGHT_AMBIENT_COLOR, 0, 0, 0);
  p = pb_push3f(p, NV097_SET_LIGHT_DIFFUSE_COLOR, 0.0f, 1.0f, 0.7f);
  p = pb_push3(p, NV097_SET_LIGHT_SPECULAR_COLOR, 0, 0, 0);
  p = pb_push1(p, NV097_SET_LIGHT_LOCAL_RANGE, 0x7149f2ca);  // 1e30
  p = pb_push3(p, NV097_SET_LIGHT_INFINITE_HALF_VECTOR, 0, 0, 0);
  p = pb_push3f(p, NV097_SET_LIGHT_INFINITE_DIRECTION, 0.0f, 0.0f, 1.0f);

  p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK, NV097_SET_LIGHT_ENABLE_MASK_LIGHT0_INFINITE);

  pb_end(p);
}

ShadeModelTests::ShadeModelTests(TestHost& host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Shade model") {
  for (auto model : kShadeModel) {
    {
      std::string name = MakeTestName(kFixedUntextured, model);
      tests_[name] = [this, model]() { this->TestShadeModelFixed(model, false); };
    }
    {
      std::string name = MakeTestName(kFixedTextured, model);
      tests_[name] = [this, model]() { this->TestShadeModelFixed(model, true); };
    }
    {
      std::string name = MakeTestName(kUntextured, model);
      tests_[name] = [this, model]() { this->TestShadeModel(model, false); };
    }
    {
      std::string name = MakeTestName(kTextured, model);
      tests_[name] = [this, model]() { this->TestShadeModel(model, true); };
    }
  }
}

void ShadeModelTests::Initialize() {
  TestSuite::Initialize();
  SetLightAndMaterial();

  constexpr uint32_t kCheckerSize = 8;
  static constexpr uint32_t kCheckerboardA = 0xFF20C0C0;
  static constexpr uint32_t kCheckerboardB = 0xFF000070;
  GenerateSwizzledRGBACheckerboard(host_.GetTextureMemory(), 0, 0, kTextureSize, kTextureSize, kTextureSize * 4,
                                   kCheckerboardA, kCheckerboardB, kCheckerSize);
}

void ShadeModelTests::TestShadeModelFixed(uint32_t model, bool texture) {
  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  std::string name = MakeTestName(texture ? kFixedUntextured : kFixedTextured, model);
  static constexpr uint32_t kBackgroundColor = 0xFF2C302E;
  host_.PrepareDraw(kBackgroundColor);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, true);
  p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, true);
  p = pb_push1(p, NV097_SET_SHADE_MODEL, model);
  p = pb_push1(p, NV097_SET_LIGHT_CONTROL, 0x10001);
  pb_end(p);

  if (texture) {
    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);
    host_.SetTextureStageEnabled(0, true);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
    texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
    host_.SetupTextureStages();
  }

  {
    static constexpr float kLeft = -2.75f;
    static constexpr float kRight = 2.75f;
    static constexpr float kTop = 1.75f;
    static constexpr float kBottom = -1.75f;

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetNormal(0.5773502691896258f, -0.5773502691896258f, 0.5773502691896258f);
    host_.SetTexCoord0(0.0f, 0.0f);
    host_.SetVertex(kLeft, kTop, 0.1f, 1.0f);

    host_.SetNormal(0.0f, 0.0f, 1.0f);
    host_.SetTexCoord0(1.0f, 0.0f);
    host_.SetVertex(kRight, kTop, 0.1f, 1.0f);

    host_.SetNormal(0.4082482904638631f, 0.4082482904638631f, 0.8164965809277261f);
    host_.SetTexCoord0(1.0f, 1.0f);
    host_.SetVertex(kRight, kBottom, 0.1f, 1.0f);

    host_.SetNormal(-0.66667f, 0.66667f, 0.3333333f);
    host_.SetTexCoord0(0.0f, 1.0f);
    host_.SetVertex(kLeft, kBottom, 0.1f, 1.0f);
    host_.End();
  }

  if (texture) {
    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
    host_.SetTextureStageEnabled(0, false);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE);
  }

  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();
  host_.FinishDraw(allow_saving_, output_dir_, name);
}

static void SetShader(TestHost &host_) {
  float depth_buffer_max_value = host_.GetMaxDepthBufferValue();
  auto shader = std::make_shared<PerspectiveVertexShader>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight(),
                                                          0.0f, depth_buffer_max_value, M_PI * 0.25f, -1.0f, 1.0f, 1.0f,
                                                          -1.0f, 1.0f, 200.0f);
  {
    shader->SetLightingEnabled(false);
    shader->SetUse4ComponentTexcoords();
    shader->SetUseD3DStyleViewport();
    VECTOR camera_position = {0.0f, 0.0f, -7.0f, 1.0f};
    VECTOR camera_look_at = {0.0f, 0.0f, 0.0f, 1.0f};
    shader->LookAt(camera_position, camera_look_at);
  }
  host_.SetVertexShaderProgram(shader);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
}

void ShadeModelTests::TestShadeModel(uint32_t model, bool texture) {
  SetShader(host_);

  std::string name = MakeTestName(texture ? kTextured : kUntextured, model);
  static constexpr uint32_t kBackgroundColor = 0xFF2C302E;
  host_.PrepareDraw(kBackgroundColor);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, true);
  p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, true);
  p = pb_push1(p, NV097_SET_SHADE_MODEL, model);
  p = pb_push1(p, NV097_SET_LIGHT_CONTROL, 0x10001);
  pb_end(p);

  if (texture) {
    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);
    host_.SetTextureStageEnabled(0, true);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
    texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
    host_.SetupTextureStages();
  }

  {
    static constexpr float kLeft = -2.75f;
    static constexpr float kRight = 2.75f;
    static constexpr float kTop = 1.75f;
    static constexpr float kBottom = -1.75f;

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0xFFFF0000);
    host_.SetTexCoord0(0.0f, 0.0f);
    host_.SetVertex(kLeft, kTop, 0.1f, 1.0f);

    host_.SetDiffuse(0xFF00FF00);
    host_.SetTexCoord0(1.0f, 0.0f);
    host_.SetVertex(kRight, kTop, 0.1f, 1.0f);

    host_.SetDiffuse(0xFF0000FF);
    host_.SetTexCoord0(1.0f, 1.0f);
    host_.SetVertex(kRight, kBottom, 0.1f, 1.0f);

    host_.SetDiffuse(0xFFCCCCCC);
    host_.SetTexCoord0(0.0f, 1.0f);
    host_.SetVertex(kLeft, kBottom, 0.1f, 1.0f);
    host_.End();
  }

  if (texture) {
    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
    host_.SetTextureStageEnabled(0, false);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE);
  }

  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();
  host_.FinishDraw(allow_saving_, output_dir_, name);
}

std::string MakeTestName(const char *prefix, uint32_t shade_model) {
  char buf[32] = {0};
  snprintf(buf, sizeof(buf), "%s_%s", prefix, shade_model == NV097_SET_SHADE_MODEL_SMOOTH ? "Smooth" : "Flat");
  return buf;
}
