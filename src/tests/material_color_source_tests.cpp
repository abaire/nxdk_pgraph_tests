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

static constexpr float kMaterialAlpha = 0.75f;
static constexpr vector_t kVertexDiffuse{0.f, 0.5f, 0.75f, 0.25f};
static constexpr vector_t kVertexSpecular{0.75f, 0.5f, 0.f, 0.25f};

static constexpr float kQuadWidth = 192.f;
static constexpr float kQuadHeight = 128.f;

struct MaterialColors {
  Color diffuse;
  Color specular;
  Color ambient;
  Color emissive;
  float specular_power{0.0f};
};

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc FromMaterial
 *   Draws 4 quads with all colors taken from the material settings.
 *   Each quad is broken into thirds: the top is just diffuse, the center is just specular, the bottom is both.
 *
 * @tc FromVertexDiffuse
 *   Draws 4 quads with all colors taken from the vertex diffuse color (1, 0.5, 0, 0.25).
 *   Each quad is broken into thirds: the top is just diffuse, the center is just specular, the bottom is both.
 *
 * @tc FromVertexSpecular
 *   Draws 4 quads with all colors taken from the vertex specular color (0, 0.5, 1, 0.25).
 *   Each quad is broken into thirds: the top is just diffuse, the center is just specular, the bottom is both.
 *
 * @tc FromMaterial_matemission0_15
 *   Draws 4 quads with all colors taken from the material settings.
 *   Each quad is broken into thirds: the top is just diffuse, the center is just specular, the bottom is both.
 *   NV097_SET_MATERIAL_EMISSION is set to (0.15, 0.15, 0.15).
 *
 * @tc FromVertexDiffuse_matemission0_15
 *   Draws 4 quads with all colors taken from the vertex diffuse color (1, 0.5, 0, 0.25).
 *   Each quad is broken into thirds: the top is just diffuse, the center is just specular, the bottom is both.
 *   NV097_SET_MATERIAL_EMISSION is set to (0.15, 0.15, 0.15).
 *
 * @tc FromVertexSpecular_matemission0_15
 *   Draws 4 quads with all colors taken from the vertex specular color (0, 0.5, 1, 0.25).
 *   Each quad is broken into thirds: the top is just diffuse, the center is just specular, the bottom is both.
 *   NV097_SET_MATERIAL_EMISSION is set to (0.15, 0.15, 0.15).
 *
 * @tc FromMaterial_matemission0_5
 *   Draws 4 quads with all colors taken from the material settings.
 *   Each quad is broken into thirds: the top is just diffuse, the center is just specular, the bottom is both.
 *   NV097_SET_MATERIAL_EMISSION is set to (0.5, 0.5, 0.5).
 *
 * @tc FromVertexDiffuse_matemission0_5
 *   Draws 4 quads with all colors taken from the vertex diffuse color (1, 0.5, 0, 0.25).
 *   Each quad is broken into thirds: the top is just diffuse, the center is just specular, the bottom is both.
 *   NV097_SET_MATERIAL_EMISSION is set to (0.5, 0.5, 0.5).
 *
 * @tc FromVertexSpecular_matemission0_5
 *   Draws 4 quads with all colors taken from the vertex specular color (0, 0.5, 1, 0.25).
 *   Each quad is broken into thirds: the top is just diffuse, the center is just specular, the bottom is both.
 *   NV097_SET_MATERIAL_EMISSION is set to (0.5, 0.5, 0.5).
 *
 * @tc FromMaterial_matemission1_0
 *   Draws 4 quads with all colors taken from the material settings.
 *   Each quad is broken into thirds: the top is just diffuse, the center is just specular, the bottom is both.
 *   NV097_SET_MATERIAL_EMISSION is set to (1, 1, 1).
 *
 * @tc FromVertexDiffuse_matemission1_0
 *   Draws 4 quads with all colors taken from the vertex diffuse color (1, 0.5, 0, 0.25).
 *   Each quad is broken into thirds: the top is just diffuse, the center is just specular, the bottom is both.
 *   NV097_SET_MATERIAL_EMISSION is set to (1, 1, 1).
 *
 * @tc FromVertexSpecular_matemission1_0
 *   Draws 4 quads with all colors taken from the vertex specular color (0, 0.5, 1, 0.25).
 *   Each quad is broken into thirds: the top is just diffuse, the center is just specular, the bottom is both.
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

static void SetCombiner(TestHost& host, bool diffuse, bool specular) {
  auto diffuse_multiplier = diffuse ? TestHost::MAP_UNSIGNED_INVERT : TestHost::MAP_UNSIGNED_IDENTITY;
  auto specular_multiplier = specular ? TestHost::MAP_UNSIGNED_INVERT : TestHost::MAP_UNSIGNED_IDENTITY;

  host.SetInputColorCombiner(0, TestHost::SRC_DIFFUSE, false, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                             false, diffuse_multiplier, TestHost::SRC_SPECULAR, false, TestHost::MAP_UNSIGNED_IDENTITY,
                             TestHost::SRC_ZERO, false, specular_multiplier);

  host.SetInputAlphaCombiner(0, TestHost::SRC_DIFFUSE, true, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO, false,
                             diffuse_multiplier, TestHost::SRC_SPECULAR, true, TestHost::MAP_UNSIGNED_IDENTITY,
                             TestHost::SRC_ZERO, false, specular_multiplier);
}

static void SetLightAndMaterial(const Color& scene_ambient, const MaterialColors& material,
                                const vector_t& material_emission) {
  auto p = pb_begin();

  float r, g, b;

  // This intentionally departs from how the XDK composes colors to make it easier to reason about how various commands
  // impact the final color. See material_color_tests for XDK approximation.

  r = scene_ambient.r + material.emissive.r;
  g = scene_ambient.g + material.emissive.g;
  b = scene_ambient.b + material.emissive.b;
  p = pb_push3f(p, NV097_SET_SCENE_AMBIENT_COLOR, r, g, b);

  p = pb_push1f(p, NV097_SET_MATERIAL_ALPHA, kMaterialAlpha);
  p = pb_push3f(p, NV097_SET_LIGHT_AMBIENT_COLOR, material.ambient.r, material.ambient.g, material.ambient.b);
  p = pb_push3f(p, NV097_SET_LIGHT_DIFFUSE_COLOR, material.diffuse.r, material.diffuse.g, material.diffuse.b);
  p = pb_push3f(p, NV097_SET_LIGHT_SPECULAR_COLOR, material.specular.r, material.specular.g, material.specular.b);

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
  const auto right = left + kQuadWidth;
  auto draw = [&host, left, right, z](float top, float bottom) {
    host.Begin(TestHost::PRIMITIVE_QUADS);

    vector_t world_point{0.f, 0.f, 0.f, 1.f};

    host.SetDiffuse(kVertexDiffuse);
    host.SetSpecular(kVertexSpecular);

    host.SetNormal(-0.099014754297667f, -0.099014754297667, -0.990147542976674f);
    Unproject(world_point, host, left, top, z);
    host.SetVertex(world_point);

    host.SetNormal(0.099014754297667f, -0.099014754297667, -0.990147542976674f);
    Unproject(world_point, host, right, top, z);
    host.SetVertex(world_point);

    host.SetNormal(0.099014754297667f, 0.099014754297667, -0.990147542976674f);
    Unproject(world_point, host, right, bottom, z);
    host.SetVertex(world_point);

    host.SetNormal(-0.099014754297667f, 0.099014754297667, -0.990147542976674f);
    Unproject(world_point, host, left, bottom, z);
    host.SetVertex(world_point);

    host.End();
  };

  auto segment_top = top;
  const auto segment_height = floorf(kQuadHeight / 3.f);

  SetCombiner(host, true, false);
  draw(segment_top, segment_top + segment_height);
  segment_top += segment_height;

  SetCombiner(host, false, true);
  draw(segment_top, segment_top + segment_height);
  segment_top += segment_height;

  SetCombiner(host, true, true);
  draw(segment_top, segment_top + segment_height);
}

static void DrawLegend(TestHost& host, float left, float top, float right, float bottom, const vector_t& scene_ambient,
                       const vector_t& material_diffuse, const vector_t& material_specular,
                       const vector_t& material_emissive, const vector_t& material_ambient) {
  auto p = pb_begin();
  // Draw a legend of colors along the left side.
  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, false);
  pb_end(p);

  SetCombiner(host, true, false);

  const auto height = floorf((bottom - top) / 7.f);
  auto draw_quad = [&host, left, right, height](float t, const vector_t& color) {
    host.Begin(TestHost::PRIMITIVE_QUADS);

    vector_t world_point{0.f, 0.f, 0.f, 1.f};

    host.SetDiffuse(color);
    Unproject(world_point, host, left, t, 0.f);
    host.SetVertex(world_point);
    Unproject(world_point, host, right, t, 0.f);
    host.SetVertex(world_point);
    Unproject(world_point, host, right, t + height, 0.f);
    host.SetVertex(world_point);
    Unproject(world_point, host, left, t + height, 0.f);
    host.SetVertex(world_point);

    host.End();
  };

  float t = top;
  draw_quad(t, kVertexDiffuse);
  t += height;

  draw_quad(t, kVertexSpecular);
  t += height;

  draw_quad(t, scene_ambient);
  t += height;

  draw_quad(t, material_diffuse);
  t += height;

  draw_quad(t, material_specular);
  t += height;

  draw_quad(t, material_emissive);
  t += height;

  draw_quad(t, material_ambient);
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

  Color scene_ambient{0.25, 0.25, 0.25, 1.0};

  MaterialColors material{
      {0.75f, 0.f, 0.f, 0.f},   // Diffuse
      {0.f, 0.75f, 0.f, 0.f},   // Specular
      {0.1f, 0.1f, 0.1f, 0.f},  // Ambient
      {0.f, 0.f, 0.75f, 0.f},   // Emissive
  };

  SetLightAndMaterial(scene_ambient, material, material_emission);

  auto p = pb_begin();
  switch (source_mode) {
    case SOURCE_MATERIAL:
      p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_ALL_FROM_MATERIAL);
      break;

    case SOURCE_DIFFUSE:
      p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_DIFFUSE_FROM_VERTEX_DIFFUSE);
      break;

    case SOURCE_SPECULAR:
      p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_DIFFUSE_FROM_VERTEX_SPECULAR);
      break;
  }
  pb_end(p);
  DrawQuad(host_, 126.f, 114.f, 0.f);

  p = pb_begin();
  switch (source_mode) {
    case SOURCE_MATERIAL:
      p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_ALL_FROM_MATERIAL);
      break;

    case SOURCE_DIFFUSE:
      p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_SPECULAR_FROM_VERTEX_DIFFUSE);
      break;

    case SOURCE_SPECULAR:
      p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_SPECULAR_FROM_VERTEX_SPECULAR);
      break;
  }
  pb_end(p);
  DrawQuad(host_, 130.f + kQuadWidth, 114.f, 0.f);

  p = pb_begin();
  switch (source_mode) {
    case SOURCE_MATERIAL:
      p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_ALL_FROM_MATERIAL);
      break;

    case SOURCE_DIFFUSE:
      p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_EMISSIVE_FROM_VERTEX_DIFFUSE);
      break;

    case SOURCE_SPECULAR:
      p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_EMISSIVE_FROM_VERTEX_SPECULAR);
      break;
  }
  pb_end(p);
  DrawQuad(host_, 126.f, 114.f + kQuadHeight + 4.f, 0.f);

  p = pb_begin();
  switch (source_mode) {
    case SOURCE_MATERIAL:
      p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_ALL_FROM_MATERIAL);
      break;

    case SOURCE_DIFFUSE:
      p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_AMBIENT_FROM_VERTEX_DIFFUSE);
      break;

    case SOURCE_SPECULAR:
      p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_AMBIENT_FROM_VERTEX_SPECULAR);
      break;
  }
  pb_end(p);
  DrawQuad(host_, 130.f + kQuadWidth, 114.f + kQuadHeight + 4.f, 0.f);

  {
    vector_t legend_scene_ambient{scene_ambient.r, scene_ambient.g, scene_ambient.b, 1.f};
    vector_t material_diffuse{material.diffuse.r, material.diffuse.g, material.diffuse.b, kMaterialAlpha};
    vector_t material_specular{material.specular.r, material.specular.g, material.specular.b, 1.f};
    vector_t material_ambient{material.ambient.r, material.ambient.g, material.ambient.b, 1.f};
    vector_t material_emissive{material.emissive.r, material.emissive.g, material.emissive.b, 1.f};
    DrawLegend(host_, 16.f, 114.4, 126.f - 16.f, 118.4 + kQuadHeight * 2.f, legend_scene_ambient, material_diffuse,
               material_specular, material_emissive, material_ambient);
  }

  pb_print("Src: %s\n", name.c_str());
  pb_printat(2, 17, (char*)"Diffuse");
  pb_printat(2, 35, (char*)" Specular");
  pb_printat(15, 17, (char*)"Emissive");
  pb_printat(15, 36, (char*)"Ambient");

  pb_printat(4, 0, "vD");
  pb_printat(5, 0, "vS");
  pb_printat(7, 0, "sA");
  pb_printat(8, 0, "mD");
  pb_printat(10, 0, "mS");
  pb_printat(11, 0, "mE");
  pb_printat(13, 0, "mA");

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
