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
static constexpr vector_t kLightAmbientColor{0.05f, 0.05f, 0.05f, 0.f};
static constexpr vector_t kLightDiffuseColor{1.f, 1.f, 0.f, 0.f};
static constexpr vector_t kLightSpecularColor{0.f, 0.f, 1.f, 0.f};

static constexpr vector_t kVertexDiffuse{1.f, 0.5f, 0.f, 0.25f};
static constexpr vector_t kVertexSpecular{0.f, 0.5f, 1.f, 0.25f};

static constexpr float kQuadWidth = 192.f;
static constexpr float kQuadHeight = 128.f;

struct MaterialColors {
  Color diffuse;
  Color specular;
  Color ambient;
  Color emissive;
  float specular_power{0.0f};
};

struct LightColors {
  Color diffuse;
  Color specular;
  Color ambient;
};

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc FromMaterial
 *   Draws 4 quads with all colors taken from the material settings.
 *
 * @tc FromVertexDiffuse
 *   Draws 4 quads with all colors taken from the vertex diffuse color (1, 0.5, 0, 0.25).
 *
 * @tc FromVertexSpecular
 *   Draws 4 quads with all colors taken from the vertex specular color (0, 0.5, 1, 0.25).
 *
 * @tc FromMaterial_matemission0_15
 *   Draws 4 quads with all colors taken from the material settings.
 *   NV097_SET_MATERIAL_EMISSION is set to (0.15, 0.15, 0.15).
 *
 * @tc FromVertexDiffuse_matemission0_15
 *   Draws 4 quads with all colors taken from the vertex diffuse color (1, 0.5, 0, 0.25).
 *   NV097_SET_MATERIAL_EMISSION is set to (0.15, 0.15, 0.15).
 *
 * @tc FromVertexSpecular_matemission0_15
 *   Draws 4 quads with all colors taken from the vertex specular color (0, 0.5, 1, 0.25).
 *   NV097_SET_MATERIAL_EMISSION is set to (0.15, 0.15, 0.15).
 *
 * @tc FromMaterial_matemission0_5
 *   Draws 4 quads with all colors taken from the material settings.
 *   NV097_SET_MATERIAL_EMISSION is set to (0.5, 0.5, 0.5).
 *
 * @tc FromVertexDiffuse_matemission0_5
 *   Draws 4 quads with all colors taken from the vertex diffuse color (1, 0.5, 0, 0.25).
 *   NV097_SET_MATERIAL_EMISSION is set to (0.5, 0.5, 0.5).
 *
 * @tc FromVertexSpecular_matemission0_5
 *   Draws 4 quads with all colors taken from the vertex specular color (0, 0.5, 1, 0.25).
 *   NV097_SET_MATERIAL_EMISSION is set to (0.5, 0.5, 0.5).
 *
 * @tc FromMaterial_matemission1_0
 *   Draws 4 quads with all colors taken from the material settings.
 *   NV097_SET_MATERIAL_EMISSION is set to (1, 1, 1).
 *
 * @tc FromVertexDiffuse_matemission1_0
 *   Draws 4 quads with all colors taken from the vertex diffuse color (1, 0.5, 0, 0.25).
 *   NV097_SET_MATERIAL_EMISSION is set to (1, 1, 1).
 *
 * @tc FromVertexSpecular_matemission1_0
 *   Draws 4 quads with all colors taken from the vertex specular color (0, 0.5, 1, 0.25).
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
  host_.SetInputColorCombiner(0, TestHost::SRC_DIFFUSE, false, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                              false, TestHost::MAP_UNSIGNED_INVERT, TestHost::SRC_SPECULAR, false,
                              TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO, false,
                              TestHost::MAP_UNSIGNED_INVERT);

  host_.SetInputAlphaCombiner(0, TestHost::SRC_DIFFUSE, true, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                              false, TestHost::MAP_UNSIGNED_INVERT, TestHost::SRC_SPECULAR, true,
                              TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO, false,
                              TestHost::MAP_UNSIGNED_INVERT);

  host_.SetOutputColorCombiner(0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0);
  host_.SetOutputAlphaCombiner(0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0);

  host_.SetFinalCombiner0Just(TestHost::SRC_R0);
  host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_R0, true,
                          false, /*specular_add_invert_r0*/ false, /* specular_add_invert_v1*/ false,
                          /* specular_clamp */ true);
}

//! Approximates how DirectX sets registers from a D3D8MATERIAL struct and light settings.
static void XDKSetLightAndMaterial(const Color& scene_ambient, const MaterialColors& material, const LightColors& light,
                                   const vector_t& material_emission) {
  auto p = pb_begin();

  float r, g, b;

  // NV097_SET_SCENE_AMBIENT_COLOR is a combination of scene ambient (D3DRS_AMBIENT), material ambient, and material
  // emissive:
  // (scene.Ambient.rgb * material.Ambient.rgb + material.Emissive.rgb) => NV097_SET_SCENE_AMBIENT_COLOR
  r = scene_ambient.r * material.ambient.r + material.emissive.r;
  g = scene_ambient.g * material.ambient.g + material.emissive.g;
  b = scene_ambient.b * material.ambient.b + material.emissive.b;
  p = pb_push3f(p, NV097_SET_SCENE_AMBIENT_COLOR, r, g, b);

  // NV097_SET_LIGHT_AMBIENT_COLOR is a product of scene ambient (D3DRS_AMBIENT) and light ambient
  // (scene.Ambient.rgb * light.Ambient.rgb) => NV097_SET_LIGHT_AMBIENT_COLOR
  r = scene_ambient.r * light.ambient.r;
  g = scene_ambient.g * light.ambient.g;
  b = scene_ambient.b * light.ambient.b;
  p = pb_push3f(p, NV097_SET_LIGHT_AMBIENT_COLOR, r, g, b);

  // material.Diffuse.rgb * light.Diffuse.rgb => NV097_SET_LIGHT_DIFFUSE_COLOR
  r = material.diffuse.r * light.diffuse.r;
  g = material.diffuse.g * light.diffuse.g;
  b = material.diffuse.b * light.diffuse.b;
  p = pb_push3f(p, NV097_SET_LIGHT_DIFFUSE_COLOR, r, g, b);

  // material.Diffuse.a => NV097_SET_MATERIAL_ALPHA (Depending on NV097_SET_LIGHT_DIFFUSE_COLOR, assuming it's set up to
  // come from material and not diffuse/specular)
  p = pb_push1f(p, NV097_SET_MATERIAL_ALPHA, material.diffuse.a);

  // material.Specular.rgb * light.Specular.rgb => NV097_SET_LIGHT_SPECULAR_COLOR
  r = material.specular.r * light.specular.r;
  g = material.specular.g * light.specular.g;
  b = material.specular.b * light.specular.b;
  p = pb_push3f(p, NV097_SET_LIGHT_SPECULAR_COLOR, r, g, b);

  // material.a // ignored?
  // material.Power is used to specify NV097_SET_SPECULAR_PARAMS?
  // material.Power = 125.0f;
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS, 0xBF78DF9C);       // -0.972162
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 4, 0xC04D3531);   // -3.20637
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 8, 0x404EFD4A);   // 3.23421
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 12, 0xBF71F52E);  // -0.945147
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 16, 0xC048FA21);  // -3.14027
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 20, 0x404C7CD6);  // 3.19512

  /*
  // material.Power = 10.0f;
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS, 0xBF34DCE5);  // -0.706496
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 4, 0xC020743F);  // -2.5071
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 8, 0x40333D06);  // 2.8006
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 12, 0xBF003612);  // -0.500825
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 16, 0xBFF852A5);  // -1.94002
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 20, 0x401C1BCE);  // 2.4392
 */

  // In a typical material setup this will be all 0's
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

static void DrawQuad(TestHost& host, float left, float top, float z) {
  auto unproject = [&host](vector_t& world_point, float x, float y, float z) {
    vector_t screen_point{x, y, z, 1.f};
    host.UnprojectPoint(world_point, screen_point, z);
  };

  const auto right = left + kQuadWidth;
  const auto bottom = top + kQuadHeight;

  host.Begin(TestHost::PRIMITIVE_QUADS);

  vector_t world_point{0.f, 0.f, 0.f, 1.f};

  host.SetDiffuse(kVertexDiffuse);
  host.SetSpecular(kVertexSpecular);

  host.SetNormal(-0.099014754297667f, -0.099014754297667, -0.990147542976674f);
  unproject(world_point, left, top, z);
  host.SetVertex(world_point);

  host.SetNormal(0.099014754297667f, -0.099014754297667, -0.990147542976674f);
  unproject(world_point, right, top, z);
  host.SetVertex(world_point);

  host.SetNormal(0.099014754297667f, 0.099014754297667, -0.990147542976674f);
  unproject(world_point, right, bottom, z);
  host.SetVertex(world_point);

  host.SetNormal(-0.099014754297667f, 0.099014754297667, -0.990147542976674f);
  unproject(world_point, left, bottom, z);
  host.SetVertex(world_point);

  host.End();
}

void MaterialColorSourceTests::Test(const std::string& name, SourceMode source_mode,
                                    const vector_t& material_emission) {
  static constexpr uint32_t kBackgroundColor = 0xFF303030;
  host_.PrepareDraw(kBackgroundColor);

  host_.DrawCheckerboardUnproject(kCheckerboardA, kCheckerboardB, 24);

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

  {
    Color scene_ambient{0.25, 0.25, 0.25, 1.0};

    MaterialColors material{
        {0.75f, 0.0f, 0.50f, 1.0f},  // Diffuse
        {0.0f, 1.0f, 0.0f, 0.0f},    // Specular
        {0.05f, 0.0f, 0.15f, 0.0f},  // Ambient
        {0.0f, 0.0f, 0.0f, 0.0f}     // Emissive
    };

    LightColors light{
        {1.0f, 1.0f, 1.0f, 0.0f},  // Diffuse
        {1.0f, 1.0f, 1.0f, 0.0f},  // Specular
        {1.0f, 1.0f, 1.0f, 0.0f},  // Ambient
    };

    XDKSetLightAndMaterial(scene_ambient, material, light, material_emission);
  }

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

  pb_print("Src: %s\n", name.c_str());
  pb_printat(2, 17, (char*)"Diffuse");
  pb_printat(2, 35, (char*)" Specular");
  pb_printat(15, 17, (char*)"Ambient");
  pb_printat(15, 36, (char*)"Emissive");
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
