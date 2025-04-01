#include "material_color_source_tests.h"

#include <debug_output.h>
#include <light.h>
#include <pbkit/pbkit.h>

#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "test_host.h"
#include "vertex_buffer.h"

static constexpr uint32_t kCheckerboardA = 0xFF202020;
static constexpr uint32_t kCheckerboardB = 0xFF909090;

static constexpr vector_t kDirectionalLightDir{0.f, 0.f, 1.f, 1.f};
// As this test is most interested in color sources rather than light interaction, ensure all light multipliers are 1.0.
static constexpr vector_t kLightAmbientColor{1.f, 1.f, 1.f, 0.f};
static constexpr vector_t kLightDiffuseColor{1.f, 1.f, 1.f, 0.f};
static constexpr vector_t kLightSpecularColor{1.f, 1.f, 1.f, 0.f};

// Colors should be chosen such that the brightest possible combination is not oversaturated.
static constexpr float kMaterialAlpha = 0.75f;
static constexpr vector_t kSceneAmbient{0.2f, 0.2f, 0.2f, 1.f};

static constexpr vector_t kVertexDiffuse{0.1f, 0.4f, 0.1f, 0.25f};
static constexpr vector_t kVertexSpecular{0.4f, 0.1f, 0.1f, 0.25f};

static constexpr vector_t kMaterialAmbient{0.1f, 0.1f, 0.1f, 0.f};
static constexpr vector_t kMaterialDiffuse{0.f, 0.4f, 0.f, 0.f};
static constexpr vector_t kMaterialSpecular{0.4f, 0.f, 0.f, 0.f};
static constexpr vector_t kMaterialEmissive{0.f, 0.f, 0.4f, 0.f};

static constexpr float kQuadWidth = 128.f;
static constexpr float kQuadHeight = 96.f;

void MaterialColorSourceTests::Initialize() {
  TestSuite::Initialize();

  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_CONTROL0, NV097_SET_CONTROL0_TEXTURE_PERSPECTIVE_ENABLE);
    pb_end(p);
  }

  host_.SetCombinerControl(1);

  host_.SetOutputColorCombiner(0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0);
  host_.SetOutputAlphaCombiner(0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0);

  host_.SetFinalCombiner0Just(TestHost::SRC_R0);
  host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_R0, true,
                          false, /*specular_add_invert_r0*/ false, /* specular_add_invert_v1*/ false,
                          /* specular_clamp */ true);
}
/**
 * Initializes the test suite and creates test cases.
 *
 * @tc FromMaterial
 *   Draws 9 quads with all colors taken from the material settings.
 *   Each quad is divided into rows: the top is just diffuse, the center is just specular, the bottom is both. It is
 *   also divided into columns: the left column retains material alpha, the right forces alpha to 1.0.
 *
 * @tc FromVertexDiffuse
 *   Draws 9 quads with some colors taken from the vertex diffuse color {0.f, 0.25f, 0.5f, 0.25f}.
 *   Each quad is divided into rows: the top is just diffuse, the center is just specular, the bottom is both. It is
 *   also divided into columns: the left column retains material alpha, the right forces alpha to 1.0.
 *
 * @tc FromVertexSpecular
 *   Draws 9 quads with some colors taken from the vertex specular color {0.5f, 0.25f, 0.f, 0.25f}.
 *   Each quad is divided into rows: the top is just diffuse, the center is just specular, the bottom is both. It is
 *   also divided into columns: the left column retains material alpha, the right forces alpha to 1.0.
 *
 * @tc FromMaterial_matemission0_15
 *   Draws 9 quads with all colors taken from the material settings.
 *   Each quad is divided into rows: the top is just diffuse, the center is just specular, the bottom is both. It is
 *   also divided into columns: the left column retains material alpha, the right forces alpha to 1.0.
 *   NV097_SET_MATERIAL_EMISSION is set to (0.15, 0.15, 0.15).
 *
 * @tc FromVertexDiffuse_matemission0_15
 *   Draws 9 quads with some colors taken from the vertex diffuse color {0.f, 0.25f, 0.5f, 0.25f}.
 *   Each quad is divided into rows: the top is just diffuse, the center is just specular, the bottom is both. It is
 *   also divided into columns: the left column retains material alpha, the right forces alpha to 1.0.
 *   NV097_SET_MATERIAL_EMISSION is set to (0.15, 0.15, 0.15).
 *
 * @tc FromVertexSpecular_matemission0_15
 *   Draws 9 quads with some colors taken from the vertex specular color {0.5f, 0.25f, 0.f, 0.25f}.
 *   Each quad is divided into rows: the top is just diffuse, the center is just specular, the bottom is both. It is
 *   also divided into columns: the left column retains material alpha, the right forces alpha to 1.0.
 *   NV097_SET_MATERIAL_EMISSION is set to (0.15, 0.15, 0.15).
 *
 * @tc FromMaterial_matemission0_5
 *   Draws 9 quads with all colors taken from the material settings.
 *   Each quad is divided into rows: the top is just diffuse, the center is just specular, the bottom is both. It is
 *   also divided into columns: the left column retains material alpha, the right forces alpha to 1.0.
 *   NV097_SET_MATERIAL_EMISSION is set to (0.5, 0.5, 0.5).
 *
 * @tc FromVertexDiffuse_matemission0_5
 *   Draws 9 quads with some colors taken from the vertex diffuse color {0.f, 0.25f, 0.5f, 0.25f}.
 *   Each quad is divided into rows: the top is just diffuse, the center is just specular, the bottom is both. It is
 *   also divided into columns: the left column retains material alpha, the right forces alpha to 1.0.
 *   NV097_SET_MATERIAL_EMISSION is set to (0.5, 0.5, 0.5).
 *
 * @tc FromVertexSpecular_matemission0_5
 *   Draws 9 quads with some colors taken from the vertex specular color {0.5f, 0.25f, 0.f, 0.25f}.
 *   Each quad is divided into rows: the top is just diffuse, the center is just specular, the bottom is both. It is
 *   also divided into columns: the left column retains material alpha, the right forces alpha to 1.0.
 *   NV097_SET_MATERIAL_EMISSION is set to (0.5, 0.5, 0.5).
 *
 * @tc FromMaterial_matemission1_0
 *   Draws 9 quads with all colors taken from the material settings.
 *   Each quad is divided into rows: the top is just diffuse, the center is just specular, the bottom is both. It is
 *   also divided into columns: the left column retains material alpha, the right forces alpha to 1.0.
 *   Each quad is broken into thirds: the top is just diffuse, the center is just specular, the bottom is both.
 *   NV097_SET_MATERIAL_EMISSION is set to (1, 1, 1).
 *
 * @tc FromVertexDiffuse_matemission1_0
 *   Draws 9 quads with some colors taken from the vertex diffuse color {0.f, 0.25f, 0.5f, 0.25f}.
 *   Each quad is divided into rows: the top is just diffuse, the center is just specular, the bottom is both. It is
 *   also divided into columns: the left column retains material alpha, the right forces alpha to 1.0.
 *   NV097_SET_MATERIAL_EMISSION is set to (1, 1, 1).
 *
 * @tc FromVertexSpecular_matemission1_0
 *   Draws 9 quads with some colors taken from the vertex specular color {0.5f, 0.25f, 0.f, 0.25f}.
 *   Each quad is divided into rows: the top is just diffuse, the center is just specular, the bottom is both. It is
 *   also divided into columns: the left column retains material alpha, the right forces alpha to 1.0.
 *   NV097_SET_MATERIAL_EMISSION is set to (1, 1, 1).
 *
 */
MaterialColorSourceTests::MaterialColorSourceTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Material color source", config) {
  for (auto source : {SOURCE_MATERIAL, SOURCE_DIFFUSE, SOURCE_SPECULAR}) {
    std::string name = MakeTestName(source);
    tests_[name] = [this, source, name]() {
      vector_t material_emission{0.f, 0.f, 0.f, 0.f};
      this->Test(name, source, material_emission);
    };

    auto new_name = name + "_matemission0_15";
    tests_[new_name] = [this, source, new_name]() {
      vector_t material_emission{0.15f, 0.15f, 0.15f, 0.f};
      this->Test(new_name, source, material_emission);
    };

    new_name = name + "_matemission0_5";
    tests_[new_name] = [this, source, new_name]() {
      vector_t material_emission{0.5f, 0.5f, 0.5f, 0.f};
      this->Test(new_name, source, material_emission);
    };

    new_name = name + "_matemission1_0";
    tests_[new_name] = [this, source, new_name]() {
      vector_t material_emission{1.f, 1.f, 1.f, 1.f};
      this->Test(new_name, source, material_emission);
    };
  }
}

static void SetCombiner(TestHost& host, bool diffuse, bool specular, bool force_opaque = false) {
  auto diffuse_multiplier = diffuse ? TestHost::MAP_UNSIGNED_INVERT : TestHost::MAP_UNSIGNED_IDENTITY;
  auto specular_multiplier = specular ? TestHost::MAP_UNSIGNED_INVERT : TestHost::MAP_UNSIGNED_IDENTITY;

  host.SetInputColorCombiner(0, TestHost::SRC_DIFFUSE, false, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                             false, diffuse_multiplier, TestHost::SRC_SPECULAR, false, TestHost::MAP_UNSIGNED_IDENTITY,
                             TestHost::SRC_ZERO, false, specular_multiplier);

  if (force_opaque) {
    host.SetInputAlphaCombiner(0, TestHost::SRC_ZERO, true, TestHost::MAP_UNSIGNED_INVERT, TestHost::SRC_ZERO, true,
                               TestHost::MAP_UNSIGNED_INVERT, TestHost::SRC_ZERO, true, TestHost::MAP_UNSIGNED_INVERT,
                               TestHost::SRC_ZERO, true, TestHost::MAP_UNSIGNED_INVERT);
  } else {
    host.SetInputAlphaCombiner(0, TestHost::SRC_DIFFUSE, true, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                               false, diffuse_multiplier, TestHost::SRC_SPECULAR, true, TestHost::MAP_UNSIGNED_IDENTITY,
                               TestHost::SRC_ZERO, false, specular_multiplier);
  }
}

static void SetLightAndMaterial(const vector_t& material_emission) {
  auto p = pb_begin();

  float r, g, b;

  // This intentionally departs from how the XDK composes colors to make it easier to reason about how various commands
  // impact the final color. See material_color_tests for XDK approximation.

  r = kSceneAmbient[0] + kMaterialEmissive[0];
  g = kSceneAmbient[1] + kMaterialEmissive[1];
  b = kSceneAmbient[2] + kMaterialEmissive[2];
  p = pb_push3f(p, NV097_SET_SCENE_AMBIENT_COLOR, r, g, b);

  p = pb_push1f(p, NV097_SET_MATERIAL_ALPHA, kMaterialAlpha);
  p = pb_push3fv(p, NV097_SET_LIGHT_AMBIENT_COLOR, kMaterialAmbient);
  p = pb_push3fv(p, NV097_SET_LIGHT_DIFFUSE_COLOR, kMaterialDiffuse);
  p = pb_push3fv(p, NV097_SET_LIGHT_SPECULAR_COLOR, kMaterialSpecular);

  // material.Power = 125.0f;
  p = pb_push3(p, NV097_SET_SPECULAR_PARAMS,
               0xBF78DF9C,  // -0.972162
               0xC04D3531,  // -3.20637
               0x404EFD4A   // 3.23421
  );
  p = pb_push3(p, NV097_SET_SPECULAR_PARAMS + 0x0C,
               0xBF71F52E,  // -0.945147
               0xC048FA21,  // -3.14027
               0x404C7CD6   // 3.19512
  );

  p = pb_push3fv(p, NV097_SET_MATERIAL_EMISSION, material_emission);

  pb_end(p);
}

static uint32_t SetupLights(TestHost& host) {
  DirectionalLight light(0, kDirectionalLightDir);
  light.SetAmbient(kLightAmbientColor);
  light.SetDiffuse(kLightDiffuseColor);
  light.SetSpecular(kLightSpecularColor);
  vector_t eye{0.0f, 0.0f, -7.0f, 1.0f};
  vector_t at{0.0f, 0.0f, 0.0f, 1.0f};

  vector_t look_dir{0.f, 0.f, 0.f, 1.f};
  VectorSubtractVector(at, eye, look_dir);
  VectorNormalize(look_dir);
  light.Commit(host, look_dir);

  return light.light_enable_mask();
}

static void Unproject(vector_t& world_point, TestHost& host, float x, float y, float z) {
  vector_t screen_point{x, y, z, 1.f};
  host.UnprojectPoint(world_point, screen_point, z);
}

static void DrawQuad(TestHost& host, float left, float top, float z) {
  auto draw = [&host, z](float left, float top, float right, float bottom) {
    host.Begin(TestHost::PRIMITIVE_QUADS);

    vector_t world_point{0.f, 0.f, 0.f, 1.f};

    host.SetDiffuse(kVertexDiffuse);
    host.SetSpecular(kVertexSpecular);
    host.SetNormal(0.f, 0.f, -1.f);

    Unproject(world_point, host, left, top, z);
    host.SetVertex(world_point);

    Unproject(world_point, host, right, top, z);
    host.SetVertex(world_point);

    Unproject(world_point, host, right, bottom, z);
    host.SetVertex(world_point);

    Unproject(world_point, host, left, bottom, z);
    host.SetVertex(world_point);

    host.End();
  };

  float opaque_start = floorf(left + kQuadWidth / 2.f);
  const auto right = left + kQuadWidth;

  auto segment_top = top;
  const auto segment_height = floorf(kQuadHeight / 3.f);

  SetCombiner(host, true, false);
  draw(left, segment_top, opaque_start, segment_top + segment_height);
  SetCombiner(host, true, false, true);
  draw(opaque_start, segment_top, right, segment_top + segment_height);
  segment_top += segment_height;

  SetCombiner(host, false, true);
  draw(left, segment_top, opaque_start, segment_top + segment_height);
  SetCombiner(host, false, true, true);
  draw(opaque_start, segment_top, right, segment_top + segment_height);
  segment_top += segment_height;

  SetCombiner(host, true, true);
  draw(left, segment_top, opaque_start, segment_top + segment_height);
  SetCombiner(host, true, true, true);
  draw(opaque_start, segment_top, right, segment_top + segment_height);
}

static void DrawColorSwatch(TestHost& host, float left, float top, float right, float bottom, const vector_t& color,
                            bool ignore_alpha = true) {
  host.Begin(TestHost::PRIMITIVE_QUADS);

  vector_t world_point{0.f, 0.f, 0.f, 1.f};

  if (ignore_alpha) {
    vector_t adjusted_color{color[0], color[1], color[2], 1.f};
    host.SetDiffuse(adjusted_color);
  } else {
    host.SetDiffuse(color);
  }

  Unproject(world_point, host, left, top, 0.f);
  host.SetVertex(world_point);
  Unproject(world_point, host, right, top, 0.f);
  host.SetVertex(world_point);
  Unproject(world_point, host, right, bottom, 0.f);
  host.SetVertex(world_point);
  Unproject(world_point, host, left, bottom, 0.f);
  host.SetVertex(world_point);

  host.End();
}

static void DrawLegend(TestHost& host, float left, float top, float right, float bottom) {
  auto p = pb_begin();
  // Draw a legend of colors along the left side.
  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, false);
  pb_end(p);

  SetCombiner(host, true, false);

  const auto height = floorf((bottom - top) / 7.f);
  auto draw_quad = [&host, left, right, height](float t, const vector_t& color) {
    DrawColorSwatch(host, left, t, right, t + height, color);
  };

  float t = top;
  draw_quad(t, kVertexDiffuse);
  t += height;

  draw_quad(t, kVertexSpecular);
  t += height;

  draw_quad(t, kSceneAmbient);
  t += height;

  draw_quad(t, kMaterialDiffuse);
  t += height;

  draw_quad(t, kMaterialSpecular);
  t += height;

  draw_quad(t, kMaterialEmissive);
  t += height;

  draw_quad(t, kMaterialAmbient);

  pb_printat(4, 0, "vD");
  pb_printat(5, 0, "vS");
  pb_printat(6, 0, "sA");
  pb_printat(7, 0, "mD");
  pb_printat(8, 0, "mS");
  pb_printat(9, 0, "mE");
  pb_printat(10, 0, "mA");
}

void MaterialColorSourceTests::Test(const std::string& name, SourceMode source_mode,
                                    const vector_t& material_emission) {
  static constexpr uint32_t kBackgroundColor = 0xFF303030;
  host_.PrepareDraw(kBackgroundColor);

  host_.DrawCheckerboardUnproject(kCheckerboardA, kCheckerboardB, 20);

  auto light_mode_bitvector = SetupLights(host_);

  {
    auto p = pb_begin();

    p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK, light_mode_bitvector);
    p = pb_push1(p, NV097_SET_POINT_PARAMS_ENABLE, false);

    p = pb_push1(p, NV097_SET_LIGHT_CONTROL,
                 NV097_SET_LIGHT_CONTROL_V_SEPARATE_SPECULAR | NV097_SET_LIGHT_CONTROL_V_ALPHA_FROM_MATERIAL_SPECULAR);
    p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, true);
    p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, true);
    pb_end(p);
  }

  SetLightAndMaterial(material_emission);

  static constexpr auto kLeftColumn = 126;
  static constexpr auto kMidColumn = kLeftColumn + kQuadWidth + 4.f;
  static constexpr auto kRightColumn = kMidColumn + kQuadWidth + 4.f;
  static constexpr auto kTopRow = 100.f;
  static constexpr auto kMidRow = kTopRow + kQuadHeight + 4.f;
  static constexpr auto kBottomRow = kMidRow + kQuadHeight + 4.f;

  auto render_quad = [this, source_mode](float left, float top, bool diffuse, bool specular, bool emissive,
                                         bool ambient) {
    uint32_t source_selector = NV097_SET_COLOR_MATERIAL_ALL_FROM_MATERIAL;

    if (source_mode == SOURCE_DIFFUSE) {
      source_selector += NV097_SET_COLOR_MATERIAL_DIFFUSE_FROM_VERTEX_DIFFUSE * diffuse;
      source_selector += NV097_SET_COLOR_MATERIAL_SPECULAR_FROM_VERTEX_DIFFUSE * specular;
      source_selector += NV097_SET_COLOR_MATERIAL_EMISSIVE_FROM_VERTEX_DIFFUSE * emissive;
      source_selector += NV097_SET_COLOR_MATERIAL_AMBIENT_FROM_VERTEX_DIFFUSE * ambient;
    } else if (source_mode == SOURCE_SPECULAR) {
      source_selector += NV097_SET_COLOR_MATERIAL_DIFFUSE_FROM_VERTEX_SPECULAR * diffuse;
      source_selector += NV097_SET_COLOR_MATERIAL_SPECULAR_FROM_VERTEX_SPECULAR * specular;
      source_selector += NV097_SET_COLOR_MATERIAL_EMISSIVE_FROM_VERTEX_SPECULAR * emissive;
      source_selector += NV097_SET_COLOR_MATERIAL_AMBIENT_FROM_VERTEX_SPECULAR * ambient;
    }

    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_COLOR_MATERIAL, source_selector);
    pb_end(p);
    DrawQuad(host_, left, top, 0.f);
  };

  render_quad(kLeftColumn, kTopRow, true, false, false, false);
  render_quad(kMidColumn, kTopRow, true, true, false, false);
  render_quad(kRightColumn, kTopRow, false, true, false, false);

  render_quad(kLeftColumn, kMidRow, true, false, true, false);
  render_quad(kMidColumn, kMidRow, true, true, true, true);
  render_quad(kRightColumn, kMidRow, false, true, false, true);

  render_quad(kLeftColumn, kBottomRow, false, false, true, false);
  render_quad(kMidColumn, kBottomRow, false, false, true, true);
  render_quad(kRightColumn, kBottomRow, false, false, false, true);

  DrawLegend(host_, 16.f, 120.f, 126.f - 16.f, 296.f);

  pb_printat(0, 0, "Src: %s\n", name.c_str());
  pb_printat(2, 17, (char*)"Diffuse");
  pb_printat(2, 35, (char*)" Specular");
  pb_printat(15, 17, (char*)"Emissive");
  pb_printat(15, 36, (char*)"Ambient");

  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

std::string MaterialColorSourceTests::MakeTestName(SourceMode source_mode) {
  switch (source_mode) {
    case SOURCE_MATERIAL:
      return "FromMaterial";

    case SOURCE_DIFFUSE:
      return "FromVertexDiffuse";

    case SOURCE_SPECULAR:
      return "FromVertexSpecular";
  }

  ASSERT(!"Unsupported SourceMode");
}
