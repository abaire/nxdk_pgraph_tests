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

static constexpr uint32_t kProvokingVertex[] = {
    NV097_SET_FLAT_SHADE_PROVOKING_VERTEX_FIRST,
    NV097_SET_FLAT_SHADE_PROVOKING_VERTEX_LAST,
};

static constexpr const char kFixedUntextured[] = "Fixed";
static constexpr const char kFixedTextured[] = "FixedTex";
static constexpr const char kUntextured[] = "Prog";
static constexpr const char kTextured[] = "ProgTex";

static constexpr TestHost::DrawPrimitive kPrimitives[] = {
    TestHost::PRIMITIVE_TRIANGLES, TestHost::PRIMITIVE_TRIANGLE_STRIP, TestHost::PRIMITIVE_TRIANGLE_FAN,
    TestHost::PRIMITIVE_QUADS,     TestHost::PRIMITIVE_QUAD_STRIP,     TestHost::PRIMITIVE_POLYGON,
};

static constexpr float kLeft = -2.75f;
static constexpr float kRight = 2.75f;
static constexpr float kTop = 1.75f;
static constexpr float kBottom = -1.75f;

constexpr uint32_t kTextureSize = 64;

std::string MakeTestName(const char* prefix, uint32_t shade_model, uint32_t provoking_vertex, TestHost::DrawPrimitive);

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
  for (auto primitive : kPrimitives) {
    for (auto provoking_vertex : kProvokingVertex) {
      for (auto model : kShadeModel) {
        {
          std::string name = MakeTestName(kFixedUntextured, model, provoking_vertex, primitive);
          tests_[name] = [this, model, provoking_vertex, primitive]() {
            this->TestShadeModelFixed(model, provoking_vertex, primitive, false);
          };
          name = "W_" + name;
          tests_[name] = [this, model, provoking_vertex, primitive]() {
            this->TestShadeModelFixed_W(model, provoking_vertex, primitive, false, 0.5f, 0.05f);
          };
        }
        {
          std::string name = MakeTestName(kFixedTextured, model, provoking_vertex, primitive);
          tests_[name] = [this, model, provoking_vertex, primitive]() {
            this->TestShadeModelFixed(model, provoking_vertex, primitive, true);
          };
          name = "W_" + name;
          tests_[name] = [this, model, provoking_vertex, primitive]() {
            this->TestShadeModelFixed_W(model, provoking_vertex, primitive, true, 0.5f, 0.05f);
          };
        }
        {
          std::string name = MakeTestName(kUntextured, model, provoking_vertex, primitive);
          tests_[name] = [this, model, provoking_vertex, primitive]() {
            this->TestShadeModel(model, provoking_vertex, primitive, false);
          };
        }
        {
          std::string name = MakeTestName(kTextured, model, provoking_vertex, primitive);
          tests_[name] = [this, model, provoking_vertex, primitive]() {
            this->TestShadeModel(model, provoking_vertex, primitive, true);
          };
        }
      }
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

static constexpr float kTestNormals[][3] = {
    {0.5773502691896258f, -0.5773502691896258f, 0.5773502691896258f},
    {0.0f, 0.0f, 1.0f},
    {0.4082482904638631f, 0.4082482904638631f, 0.8164965809277261f},
    {-0.66667f, 0.66667f, 0.3333333f},
    {0.3015, 0.3015, 0.9045},
    {0.4851, 0.4851, 0.7276},
    {0.6247, 0.6247, 0.4685},
    {0.2673, 0.5345, 0.8018},
    {0.6767, 0.6767, 0.29},
    {0.7317, 0.3049, 0.6097},
    {0.6172, 0.7715, 0.1543},
    {0.6527, 0.272, 0.7071},
    {0.7861, 0.3276, 0.5241},
    {0.6509, 0.6509, 0.3906},
};

static constexpr uint32_t kTestDiffuse[] = {
    0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFFCCCCCC, 0xFFFF33CC, 0xFFFFCC33, 0xFFCCFF33,
    0xFF33FFCC, 0xFF33CCFF, 0xFFCC33FF, 0xFF991111, 0xFF119911, 0xFF111199, 0xFF666666,
};

static void add_vertex(TestHost& host, float x, float y, uint32_t index, float u, float v, float w = 1.0f) {
  host.SetNormal(kTestNormals[index][0], kTestNormals[index][1], kTestNormals[index][2]);
  host.SetDiffuse(kTestDiffuse[index]);
  host.SetTexCoord0(u, v);
  host.SetVertex(x, y, 0.1f, w);
};

static void DrawTriangles(TestHost& host, float w, float w_inc) {
  host.Begin(TestHost::PRIMITIVE_TRIANGLES);
  uint32_t index = 0;
  add_vertex(host, kLeft, kTop, index++, 0.0f, 0.0f, w);
  w += w_inc;
  add_vertex(host, 0.0f, kTop, index++, 1.0f, 0.0f, w);
  w += w_inc;
  add_vertex(host, kLeft, 0.0f, index++, 0.0f, 1.0f, w);
  w += w_inc;

  add_vertex(host, kRight, kTop, index++, 1.0f, 0.0f, w);
  w += w_inc;
  add_vertex(host, 0.10f, kBottom, index++, 0.1f, 1.0f, w);
  w += w_inc;
  add_vertex(host, 0.25f, 0.0f, index++, 0.25f, 0.5f, w);
  w += w_inc;

  add_vertex(host, -0.4f, kBottom, index++, 0.5f, 1.0f, w);
  w += w_inc;
  add_vertex(host, -1.4f, -1.4, index++, 0.2f, 0.5f, w);
  w += w_inc;
  add_vertex(host, 0.0f, 0.0f, index++, 1.0f, 0.0f, w);
  host.End();
}

static void DrawTriangleStrip(TestHost& host, float w, float w_inc) {
  host.Begin(TestHost::PRIMITIVE_TRIANGLE_STRIP);
  uint32_t index = 0;

  add_vertex(host, kLeft, 0.0f, index++, 0.0f, 0.0f, w);
  w += w_inc;
  add_vertex(host, -2.25f, kTop, index++, 0.0f, 1.0f, w);
  w += w_inc;
  add_vertex(host, -2.0f, kBottom, index++, 0.0f, 0.0f, w);
  w += w_inc;

  add_vertex(host, -1.3f, 1.6, index++, 1.15f, 0.25f, w);
  w += w_inc;

  add_vertex(host, 0.0f, -1.5f, index++, 1.3f, 0.75f, w);
  w += w_inc;

  add_vertex(host, 0.4f, 1.0f, index++, 1.7f, 0.33f, w);
  w += w_inc;

  add_vertex(host, 1.4f, -0.6f, index++, 0.7f, 0.7f, w);
  w += w_inc;

  add_vertex(host, kRight, kTop, index++, 0.5f, 0.5f, w);
  host.End();
}

static void DrawTriangleFan(TestHost& host, float w, float w_inc) {
  host.Begin(TestHost::PRIMITIVE_TRIANGLE_FAN);
  uint32_t index = 0;
  add_vertex(host, 0.0f, -0.75f, index++, 1.0f, 1.0f, w);
  w += w_inc;
  add_vertex(host, -2.25f, kBottom, index++, 0.0f, 0.0f, w);
  w += w_inc;
  add_vertex(host, -2.0f, kTop, index++, 0.0f, 1.0f, w);
  w += w_inc;

  add_vertex(host, -0.6f, 0.65f, index++, 1.15f, 0.25f, w);
  w += w_inc;

  add_vertex(host, 0.0f, 1.5f, index++, 1.3f, 0.75f, w);
  w += w_inc;

  add_vertex(host, 0.4f, 1.0f, index++, 1.7f, 0.33f, w);
  w += w_inc;

  add_vertex(host, 2.4f, 0.6f, index++, 0.7f, 0.7f, w);
  w += w_inc;

  add_vertex(host, kRight, kBottom, index++, 0.5f, 0.5f, w);
  host.End();
}

static void DrawQuads(TestHost& host, float w, float w_inc) {
  host.Begin(TestHost::PRIMITIVE_QUADS);
  uint32_t index = 0;

  add_vertex(host, kLeft, kTop, index++, 0.0f, 0.0f, w);
  w += w_inc;
  add_vertex(host, -0.4f, kTop, index++, 1.0f, 0.0f, w);
  w += w_inc;
  add_vertex(host, -0.4f, kBottom, index++, 1.0f, 1.0f, w);
  w += w_inc;
  add_vertex(host, kLeft, kBottom, index++, 0.0f, 1.0f, w);
  w += w_inc;

  add_vertex(host, 0.15f, kTop, index++, 0.0f, 0.0f, w);
  w += w_inc;
  add_vertex(host, kRight, kTop, index++, 1.0f, 0.0f, w);
  w += w_inc;
  add_vertex(host, kRight, kBottom, index++, 1.0f, 1.0f, w);
  w += w_inc;
  add_vertex(host, 0.0f, kBottom, index++, 0.0f, 1.0f, w);

  host.End();
}

static void DrawQuadStrip(TestHost& host, float w, float w_inc) {
  host.Begin(TestHost::PRIMITIVE_QUAD_STRIP);
  uint32_t index = 0;
  add_vertex(host, kLeft, kBottom, index++, 0.33f, 0.33f, w);
  w += w_inc;
  add_vertex(host, kLeft, kTop, index++, 1.0f, 1.0f, w);
  w += w_inc;
  add_vertex(host, 0.0f, -1.35f, index++, 0.7f, 0.1f, w);
  w += w_inc;
  add_vertex(host, 0.0f, 1.0f, index++, 0.0f, 0.9f, w);
  w += w_inc;

  add_vertex(host, kRight, kBottom, index++, 0.33f, 0.33f, w);
  w += w_inc;
  add_vertex(host, kRight, kTop, index++, 0.0f, 0.7f, w);
  host.End();
}

static void DrawPolygon(TestHost& host, float w, float w_inc) {
  host.Begin(TestHost::PRIMITIVE_POLYGON);
  uint32_t index = 0;
  add_vertex(host, kLeft, kBottom, index++, 0.33f, 0.33f, w);
  w += w_inc;
  add_vertex(host, -1.4f, 1.1f, index++, 0.7f, 0.1f, w);
  w += w_inc;
  add_vertex(host, -0.3f, kTop, index++, 0.1f, 0.7f, w);
  w += w_inc;
  add_vertex(host, 2.0f, 0.3f, index++, 0.0f, 0.9f, w);
  w += w_inc;
  add_vertex(host, kRight, -1.5f, index++, 0.7f, 0.7f, w);
  host.End();
}

static void Draw(TestHost& host, TestHost::DrawPrimitive primitive, float w = 1.0f, float w_inc = 0.0f) {
  switch (primitive) {
    case TestHost::PRIMITIVE_LINES:
    case TestHost::PRIMITIVE_POINTS:
    case TestHost::PRIMITIVE_LINE_LOOP:
    case TestHost::PRIMITIVE_LINE_STRIP:
      ASSERT(!"Not implemented");
      break;

    case TestHost::PRIMITIVE_TRIANGLES:
      DrawTriangles(host, w, w_inc);
      break;

    case TestHost::PRIMITIVE_TRIANGLE_STRIP:
      DrawTriangleStrip(host, w, w_inc);
      break;

    case TestHost::PRIMITIVE_TRIANGLE_FAN:
      DrawTriangleFan(host, w, w_inc);
      break;

    case TestHost::PRIMITIVE_QUADS:
      DrawQuads(host, w, w_inc);
      break;

    case TestHost::PRIMITIVE_QUAD_STRIP:
      DrawQuadStrip(host, w, w_inc);
      break;

    case TestHost::PRIMITIVE_POLYGON:
      DrawPolygon(host, w, w_inc);
      break;
  }
}

void ShadeModelTests::TestShadeModelFixed(uint32_t model, uint32_t provoking_vertex, TestHost::DrawPrimitive primitive,
                                          bool texture) {
  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  std::string name = MakeTestName(texture ? kFixedTextured : kFixedUntextured, model, provoking_vertex, primitive);
  static constexpr uint32_t kBackgroundColor = 0xFF2C302E;
  host_.PrepareDraw(kBackgroundColor);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, true);
  p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, true);
  p = pb_push1(p, NV097_SET_SHADE_MODEL, model);
  p = pb_push1(p, NV097_SET_FLAT_SHADE_PROVOKING_VERTEX, provoking_vertex);
  p = pb_push1(p, NV097_SET_LIGHT_CONTROL, 0x10001);
  pb_end(p);

  if (texture) {
    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);
    host_.SetTextureStageEnabled(0, true);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
    auto& texture_stage = host_.GetTextureStage(0);
    texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
    texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
    host_.SetupTextureStages();
  }

  Draw(host_, primitive);

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

static void SetShader(TestHost& host_) {
  float depth_buffer_max_value = host_.GetMaxDepthBufferValue();
  auto shader = std::make_shared<PerspectiveVertexShader>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight(),
                                                          0.0f, depth_buffer_max_value, M_PI * 0.25f, 1.0f, 200.0f);
  {
    shader->SetLightingEnabled(false);
    shader->SetUse4ComponentTexcoords();
    shader->SetUseD3DStyleViewport();
    vector_t camera_position = {0.0f, 0.0f, -7.0f, 1.0f};
    vector_t camera_look_at = {0.0f, 0.0f, 0.0f, 1.0f};
    shader->LookAt(camera_position, camera_look_at);
  }
  host_.SetVertexShaderProgram(shader);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
}

void ShadeModelTests::TestShadeModel(uint32_t model, uint32_t provoking_vertex, TestHost::DrawPrimitive primitive,
                                     bool texture) {
  SetShader(host_);

  std::string name = MakeTestName(texture ? kTextured : kUntextured, model, provoking_vertex, primitive);
  static constexpr uint32_t kBackgroundColor = 0xFF2C302E;
  host_.PrepareDraw(kBackgroundColor);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, false);
  p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, true);
  p = pb_push1(p, NV097_SET_SHADE_MODEL, model);
  p = pb_push1(p, NV097_SET_FLAT_SHADE_PROVOKING_VERTEX, provoking_vertex);
  p = pb_push1(p, NV097_SET_LIGHT_CONTROL, 0x10001);
  pb_end(p);

  if (texture) {
    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);
    host_.SetTextureStageEnabled(0, true);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
    auto& texture_stage = host_.GetTextureStage(0);
    texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
    texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
    host_.SetupTextureStages();
  }

  Draw(host_, primitive);

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

void ShadeModelTests::TestShadeModelFixed_W(uint32_t model, uint32_t provoking_vertex,
                                            TestHost::DrawPrimitive primitive, bool texture, float w, float w_inc) {
  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  std::string name =
      "W_" + MakeTestName(texture ? kFixedTextured : kFixedUntextured, model, provoking_vertex, primitive);
  static constexpr uint32_t kBackgroundColor = 0xFF2C302E;
  host_.PrepareDraw(kBackgroundColor);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, true);
  p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, true);
  p = pb_push1(p, NV097_SET_SHADE_MODEL, model);
  p = pb_push1(p, NV097_SET_FLAT_SHADE_PROVOKING_VERTEX, provoking_vertex);
  p = pb_push1(p, NV097_SET_LIGHT_CONTROL, 0x10001);
  pb_end(p);

  if (texture) {
    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);
    host_.SetTextureStageEnabled(0, true);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
    auto& texture_stage = host_.GetTextureStage(0);
    texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
    texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
    host_.SetupTextureStages();
  }

  Draw(host_, primitive, w, w_inc);

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

void ShadeModelTests::Deinitialize() {
  TestSuite::Deinitialize();
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_FLAT_SHADE_PROVOKING_VERTEX, NV097_SET_FLAT_SHADE_PROVOKING_VERTEX_FIRST);
  pb_end(p);
}

std::string MakeTestName(const char* prefix, uint32_t shade_model, uint32_t provoking_vertex,
                         TestHost::DrawPrimitive primitive) {
  const char* primitive_name;
  switch (primitive) {
    case TestHost::PRIMITIVE_TRIANGLES:
      primitive_name = "Tri";
      break;

    case TestHost::PRIMITIVE_TRIANGLE_STRIP:
      primitive_name = "TriStrip";
      break;

    case TestHost::PRIMITIVE_TRIANGLE_FAN:
      primitive_name = "TriFan";
      break;

    case TestHost::PRIMITIVE_QUADS:
      primitive_name = "Quad";
      break;

    case TestHost::PRIMITIVE_QUAD_STRIP:
      primitive_name = "QuadStrip";
      break;

    case TestHost::PRIMITIVE_POLYGON:
      primitive_name = "Poly";
      break;

    default:
      ASSERT(!"Not implemented");
  }

  char buf[32] = {0};
  snprintf(buf, sizeof(buf), "%s_%s_%s_%s", prefix, primitive_name,
           shade_model == NV097_SET_SHADE_MODEL_SMOOTH ? "Smooth" : "Flat",
           provoking_vertex == NV097_SET_FLAT_SHADE_PROVOKING_VERTEX_FIRST ? "First" : "Last");
  return buf;
}
