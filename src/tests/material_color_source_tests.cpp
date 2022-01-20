#include "material_color_source_tests.h"

#include <pbkit/pbkit.h>

#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "test_host.h"
#include "vertex_buffer.h"

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

MaterialColorSourceTests::MaterialColorSourceTests(TestHost& host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Material color source") {
  for (auto source : {SOURCE_MATERIAL, SOURCE_DIFFUSE, SOURCE_SPECULAR}) {
    std::string name = MakeTestName(source);
    auto test = [this, source]() { this->Test(source); };

    tests_[name] = test;
  }
}

void MaterialColorSourceTests::Initialize() {
  TestSuite::Initialize();

  host_.SetShaderProgram(nullptr);
  CreateGeometry();
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
}

void MaterialColorSourceTests::CreateGeometry() {
  float left = -2.75f;
  float right = 2.75f;
  float top = 1.75f;
  float bottom = -1.75f;
  float mid_width = 0.0f;
  float mid_height = 0.0f;

  float z = 1.0f;

  VECTOR normal{0.0f, 0.0f, 1.0f, 1.0f};

  Color diffuse{0.0f, 1.0f, 0.0f, 1.0f};
  Color specular{0.0f, 0.0f, 1.0f, 1.0f};

  {
    float one_pos[3] = {left + (mid_width - left) * 0.5f, top, z};
    float two_pos[3] = {mid_width, mid_height, z};
    float three_pos[3] = {left, mid_height, z};

    diffuse_buffer_ = host_.AllocateVertexBuffer(3);
    diffuse_buffer_->DefineTriangle(0, one_pos, two_pos, three_pos, normal, normal, normal);
    for (auto index : {0, 1, 2}) {
      diffuse_buffer_->SetDiffuse(index, diffuse);
      diffuse_buffer_->SetSpecular(index, specular);
    }
  }

  {
    float one_pos[3] = {mid_width + (right - mid_width) * 0.5f, top, z};
    float two_pos[3] = {right, mid_height, z};
    float three_pos[3] = {mid_width, mid_height, z};
    specular_buffer_ = host_.AllocateVertexBuffer(3);
    specular_buffer_->DefineTriangle(0, one_pos, two_pos, three_pos, normal, normal, normal);
    for (auto index : {0, 1, 2}) {
      specular_buffer_->SetDiffuse(index, diffuse);
      specular_buffer_->SetSpecular(index, specular);
    }
  }

  {
    float one_pos[3] = {left, mid_height, z};
    float two_pos[3] = {mid_width, mid_height, z};
    float three_pos[3] = {left + (mid_width - left) * 0.5f, bottom, z};
    ambient_buffer_ = host_.AllocateVertexBuffer(3);
    ambient_buffer_->DefineTriangle(0, one_pos, two_pos, three_pos, normal, normal, normal);
    for (auto index : {0, 1, 2}) {
      ambient_buffer_->SetDiffuse(index, diffuse);
      ambient_buffer_->SetSpecular(index, specular);
    }
  }

  {
    float one_pos[3] = {mid_width, mid_height, z};
    float two_pos[3] = {right, mid_height, z};
    float three_pos[3] = {mid_width + (right - mid_width) * 0.5f, bottom, z};
    emissive_buffer_ = host_.AllocateVertexBuffer(3);
    emissive_buffer_->DefineTriangle(0, one_pos, two_pos, three_pos, normal, normal, normal);
    for (auto index : {0, 1, 2}) {
      emissive_buffer_->SetDiffuse(index, diffuse);
      emissive_buffer_->SetSpecular(index, specular);
    }
  }
}

static void SetLightAndMaterial(const Color& scene_ambient, const MaterialColors& material, const LightColors& light) {
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

  // material.a // Ignored? Maybe it goes into NV097_SET_SPECULAR_PARAMS?
  // material.Power // Maybe it goes into NV097_SET_SPECULAR_PARAMS?
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

  // It seems like material emission is already incorporated into scene_ambient_color and is set to 0.
  p = pb_push3f(p, NV097_SET_MATERIAL_EMISSION, 0, 0, 0);

  pb_end(p);
}

void MaterialColorSourceTests::Test(SourceMode source_mode) {
  static constexpr uint32_t kBackgroundColor = 0xFF303030;
  host_.PrepareDraw(kBackgroundColor);

  auto p = pb_begin();

  // Set up a directional light.
  p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK, NV097_SET_LIGHT_ENABLE_MASK_LIGHT0_INFINITE);

  p = pb_push1(p, NV097_SET_LIGHT_LOCAL_RANGE, 0x7149f2ca);  // 1e30
  p = pb_push3(p, NV097_SET_LIGHT_INFINITE_HALF_VECTOR, 0, 0, 0);
  p = pb_push3f(p, NV097_SET_LIGHT_INFINITE_DIRECTION, 0.0f, 0.0f, 1.0f);

  p = pb_push1(p, NV097_SET_CONTROL0, 0x100001);

  p = pb_push1(p, NV10_TCL_PRIMITIVE_3D_POINT_PARAMETERS_ENABLE, false);

  p = pb_push1(p, NV097_SET_LIGHT_CONTROL, 0x10001);

  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, true);
  p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, true);
  pb_end(p);

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
    SetLightAndMaterial(scene_ambient, material, light);
  }
  p = pb_begin();
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
  host_.SetVertexBuffer(diffuse_buffer_);
  host_.DrawArrays(host_.POSITION | host_.NORMAL | host_.DIFFUSE | host_.SPECULAR);

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
  host_.SetVertexBuffer(specular_buffer_);
  host_.DrawArrays(host_.POSITION | host_.NORMAL | host_.DIFFUSE | host_.SPECULAR);

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
  host_.SetVertexBuffer(emissive_buffer_);
  host_.DrawArrays(host_.POSITION | host_.NORMAL | host_.DIFFUSE | host_.SPECULAR);

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
  host_.SetVertexBuffer(ambient_buffer_);
  host_.DrawArrays(host_.POSITION | host_.NORMAL | host_.DIFFUSE | host_.SPECULAR);

  std::string name = MakeTestName(source_mode);
  pb_print("Src: %s\n", name.c_str());
  pb_printat(7, 17, (char*)"Diffuse");
  pb_printat(7, 35, (char*)" Specular");
  pb_printat(9, 17, (char*)"Ambient");
  pb_printat(9, 36, (char*)"Emissive");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, name);
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
}
