#include "material_color_tests.h"

#include <pbkit/pbkit.h>

#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "test_host.h"
#include "vertex_buffer.h"

static constexpr uint32_t kDiffuseSource[] = {
    NV097_SET_COLOR_MATERIAL_DIFFUSE_FROM_MATERIAL,
    NV097_SET_COLOR_MATERIAL_DIFFUSE_FROM_VERTEX_DIFFUSE,
    NV097_SET_COLOR_MATERIAL_DIFFUSE_FROM_VERTEX_SPECULAR,
};

static constexpr uint32_t kSpecularSource[] = {
    NV097_SET_COLOR_MATERIAL_SPECULAR_FROM_MATERIAL,
    NV097_SET_COLOR_MATERIAL_SPECULAR_FROM_VERTEX_DIFFUSE,
    NV097_SET_COLOR_MATERIAL_SPECULAR_FROM_VERTEX_SPECULAR,
};

static constexpr uint32_t kEmissiveSource[] = {
    NV097_SET_COLOR_MATERIAL_EMISSIVE_FROM_MATERIAL,
    NV097_SET_COLOR_MATERIAL_EMISSIVE_FROM_VERTEX_DIFFUSE,
    NV097_SET_COLOR_MATERIAL_EMISSIVE_FROM_VERTEX_SPECULAR,
};

static constexpr uint32_t kAmbientSource[] = {
    NV097_SET_COLOR_MATERIAL_AMBIENT_FROM_MATERIAL,
    NV097_SET_COLOR_MATERIAL_AMBIENT_FROM_VERTEX_DIFFUSE,
    NV097_SET_COLOR_MATERIAL_AMBIENT_FROM_VERTEX_SPECULAR,
};

MaterialColorTests::MaterialColorTests(TestHost& host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Material color") {
  for (auto diffuse : kDiffuseSource) {
    for (auto specular : kSpecularSource) {
      for (auto emissive : kEmissiveSource) {
        for (auto ambient : kAmbientSource) {
          std::string name = MakeTestName(diffuse, specular, emissive, ambient);
          auto test = [this, diffuse, specular, emissive, ambient]() {
            this->Test(diffuse, specular, emissive, ambient);
          };

          tests_[name] = test;
        }
      }
    }
  }
}

void MaterialColorTests::Initialize() {
  TestSuite::Initialize();
  // NV097_SET_TEXTURE_FORMAT_COLOR_SZ_X8R8G8B8
  const TextureFormatInfo& texture_format = kTextureFormats[3];
  host_.SetTextureFormat(texture_format);

  host_.SetShaderProgram(nullptr);

  host_.SetDepthBufferFormat(NV097_SET_SURFACE_FORMAT_ZETA_Z16);
  host_.SetDepthBufferFloatMode(false);

  host_.SetTextureStageEnabled(0, false);

  CreateGeometry();

  MATRIX matrix;
  matrix_unit(matrix);
  host_.SetFixedFunctionModelViewMatrix(matrix);
  host_.SetFixedFunctionProjectionMatrix(matrix);
}

void MaterialColorTests::Deinitialize() {
  host_.SetTextureStageEnabled(0, true);
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, 0);
  p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, 0);
  pb_end(p);
  TestSuite::Deinitialize();
}

void MaterialColorTests::CreateGeometry() {
  auto fb_width = static_cast<float>(host_.GetFramebufferWidth());
  auto fb_height = static_cast<float>(host_.GetFramebufferHeight());

  float left = -1.0f * floorf(fb_width / 4.0f);
  float right = floorf(fb_width / 4.0f);
  float top = -1.0f * floorf(fb_height / 3.0f);
  float bottom = floorf(fb_height / 3.0f);
  float mid_width = left + (right - left) * 0.5f;

  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(6);

  Color ul{0.4f, 0.1f, 0.1f, 0.25f};
  Color ll{0.0f, 1.0f, 0.0f, 1.0f};
  Color lr{0.0f, 0.0f, 1.0f, 1.0f};
  Color ur{0.5f, 0.5f, 0.5f, 1.0f};

  Color ul_s{0.0f, 1.0f, 0.0f, 0.5f};
  Color ll_s{1.0f, 0.0f, 0.0f, 0.1f};
  Color lr_s{1.0f, 1.0f, 0.0f, 0.5f};
  Color ur_s{0.0f, 1.0f, 1.0f, 0.75f};

  buffer->DefineQuad(0, left + 10, top + 4, mid_width + 10, bottom - 10, 10.0f, 10.0f, 10.0f, 10.0f, ul, ll, lr, ur,
                     ul_s, ll_s, lr_s, ur_s);
  // Point normals for half the quad away from the camera.
  Vertex* v = buffer->Lock();
  v[0].normal[2] = -1.0f;
  v[1].normal[2] = -1.0f;
  v[2].normal[2] = -1.0f;
  buffer->Unlock();

  ul.SetRGBA(1.0f, 1.0f, 0.0f, 1.0f);
  ul_s.SetRGBA(1.0f, 0.0f, 0.0f, 0.25f);

  ll.SetGreyA(0.5, 1.0f);
  ll_s.SetRGBA(0.3f, 0.3f, 1.0f, 1.0f);

  ur.SetRGBA(0.0f, 0.3f, 0.8f, 0.15f);
  ur_s.SetRGBA(0.9f, 0.9f, 0.4f, 0.33);

  lr.SetRGBA(1.0f, 0.0f, 0.0f, 0.75f);
  lr_s.SetRGBA(0.95f, 0.5f, 0.8f, 0.05);
  buffer->DefineQuad(1, mid_width - 10, top + 4, right - 10, bottom - 10, 10.0f, 10.0f, 10.0f, 10.0f, ul, ll, lr, ur,
                     ul_s, ll_s, lr_s, ur_s);
}

void MaterialColorTests::Test(uint32_t diffuse_source, uint32_t specular_source, uint32_t emissive_source,
                              uint32_t ambient_source) {
  static constexpr uint32_t kBackgroundColor = 0xFF303030;
  host_.PrepareDraw(kBackgroundColor);

  auto p = pb_begin();

  // Set up a directional light.
  p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK, NV097_SET_LIGHT_ENABLE_MASK_LIGHT0_INFINITE);

  // NV097_SET_SCENE_AMBIENT_COLOR is a combination of scene ambient (D3DRS_AMBIENT), material ambient, and material
  // emissive:
  // (scene.Ambient.rgb * material.Ambient.rgb + material.Emissive.rgb) => NV097_SET_SCENE_AMBIENT_COLOR
  p = pb_push3f(p, NV097_SET_SCENE_AMBIENT_COLOR, 0, 0, 0);

  // NV097_SET_LIGHT_AMBIENT_COLOR is a product of scene ambient (D3DRS_AMBIENT) and light ambient
  // (scene.Ambient.rgb * light.Ambient.rgb) => NV097_SET_LIGHT_AMBIENT_COLOR
  p = pb_push3(p, NV097_SET_LIGHT_AMBIENT_COLOR, 0, 0, 0);

  // material.Diffuse.rgb => NV097_SET_LIGHT_DIFFUSE_COLOR
  p = pb_push3f(p, NV097_SET_LIGHT_DIFFUSE_COLOR, 0.2f, 0.6f, 0.8f);

  // material.Diffuse.a => NV097_SET_MATERIAL_ALPHA (Depending on NV097_SET_LIGHT_DIFFUSE_COLOR, assuming it's set up to
  // come from material and not diffuse/specular)
  p = pb_push1f(p, NV097_SET_MATERIAL_ALPHA, 1.0f);

  // material.Specular.rgb => NV097_SET_LIGHT_SPECULAR_COLOR
  p = pb_push3f(p, NV097_SET_LIGHT_SPECULAR_COLOR, 0.15f, 0.55f, 0.75f);

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

  p = pb_push1(p, NV097_SET_LIGHT_LOCAL_RANGE, 0x7149f2ca);  // 1e30
  p = pb_push3(p, NV097_SET_LIGHT_INFINITE_HALF_VECTOR, 0, 0, 0);
  p = pb_push3f(p, NV097_SET_LIGHT_INFINITE_DIRECTION, 0.0f, 0.0f, 1.0f);

  p = pb_push1(p, NV097_SET_CONTROL0, 0x100001);

  p = pb_push1(p, NV10_TCL_PRIMITIVE_3D_POINT_PARAMETERS_ENABLE, 0x0);

  p = pb_push1(p, NV097_SET_LIGHT_CONTROL, 0x10001);

  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, true);
  p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, true);

  p = pb_push1(p, NV097_SET_COLOR_MATERIAL, diffuse_source);

  pb_end(p);

  host_.DrawArrays(host_.POSITION | host_.NORMAL | host_.DIFFUSE | host_.SPECULAR);

  //  std::string source = ComponentSourceName(diffuse_source);
  //  pb_print("Src: %s\n", source.c_str());
  //  pb_print("Alpha: %g\n", material_alpha);
  //  pb_draw_text_screen();
  //
  //  std::string name = MakeTestName(diffuse_source, material_alpha);
  //  host_.FinishDrawAndSave(output_dir_, name);
}

std::string MaterialColorTests::MakeTestName(uint32_t diffuse_source, uint32_t specular_source,
                                             uint32_t emissive_source, uint32_t ambient_source) {
  std::string ret = "D";
  switch (diffuse_source) {
    case NV097_SET_COLOR_MATERIAL_DIFFUSE_FROM_MATERIAL:
      ret += "m";

    case NV097_SET_COLOR_MATERIAL_DIFFUSE_FROM_VERTEX_DIFFUSE:
      ret += "c0";

    case NV097_SET_COLOR_MATERIAL_DIFFUSE_FROM_VERTEX_SPECULAR:
      ret += "c1";
  }

  ret += "_S";
  switch (specular_source) {
    case NV097_SET_COLOR_MATERIAL_SPECULAR_FROM_MATERIAL:
      ret += "m";

    case NV097_SET_COLOR_MATERIAL_SPECULAR_FROM_VERTEX_DIFFUSE:
      ret += "c0";

    case NV097_SET_COLOR_MATERIAL_SPECULAR_FROM_VERTEX_SPECULAR:
      ret += "c1";
  }

  ret += "_E";

  return std::move(ret);
}
