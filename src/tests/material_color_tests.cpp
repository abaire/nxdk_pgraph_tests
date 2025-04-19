#include "material_color_tests.h"

#include <pbkit/pbkit.h>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "test_host.h"
#include "vertex_buffer.h"

// Naming convention:
// AAA_MMM_LL
//   scene_ambient, material_ambient, light_ambient
//   material_diffuse, material_specular, material_emissive
//   light_diffuse, light_specular
//
// R = Red 1.0
// r = Red 0.5
// G = Green 1.0
// g = Green 0.5
// B = Blue 1.0
// b = Blue 0.5
// 0 = Black
// 3 = Grey 0.3, 0.3, 0.3
// 5 = Grey 0.5, 0.5, 0.5
// 1 = White
struct TestCase {
  const char name[32]{0};
  float specular_power{100.0f};

  static void SetColor(char color_code, Color& color) {
    switch (color_code) {
      case 'R':
        color.SetRGBA(1.0f, 0.0f, 0.0f, 1.0f);
        return;

      case 'r':
        color.SetRGBA(0.5f, 0.0f, 0.0f, 1.0f);
        return;

      case 'G':
        color.SetRGBA(0.0f, 1.0f, 0.0f, 1.0f);
        return;

      case 'g':
        color.SetRGBA(0.0f, 0.5f, 0.0f, 1.0f);
        return;

      case 'B':
        color.SetRGBA(0.0f, 0.0f, 1.0f, 1.0f);
        return;

      case 'b':
        color.SetRGBA(0.0f, 0.0f, 0.5f, 1.0f);
        return;

      case '0':
        color.SetGrey(0.0f);
        return;

      case '1':
        color.SetGrey(1.0f);
        return;

      case '2':
        color.SetGrey(0.25f);
        return;

      case '3':
        color.SetGrey(0.33f);
        return;

      case '5':
        color.SetGrey(0.5f);
        return;

      case '6':
        color.SetGrey(0.66f);
        return;

      case '7':
        color.SetGrey(0.75f);
        return;

      default:
        ASSERT(!"Invalid color command code");
        return;
    }
  }

  MaterialColorTests::TestConfig BuildConfig() const {
    MaterialColorTests::TestConfig ret;

    memcpy(ret.name, name, sizeof(ret.name));

    SetColor(name[0], ret.scene_ambient);
    SetColor(name[1], ret.material_ambient);
    SetColor(name[2], ret.light_ambient);

    SetColor(name[4], ret.material_diffuse);
    SetColor(name[5], ret.material_specular);
    SetColor(name[6], ret.material_emissive);

    SetColor(name[8], ret.light_diffuse);
    SetColor(name[9], ret.light_specular);

    ret.material_specular_power = specular_power;

    return ret;
  }
};

// clang-format off

static constexpr TestCase kTests[] = {
    {
        // All color from material_diffuse (red) with a white light
        "001_RG0_11",
        100.0f,
    },
    {
        // All color from material_diffuse (red) with a blue-only light
        "011_rG0_B1",
        100.0f,
    },
    {
        // All color from material_diffuse (red) with a white light + material_emissive (green)
        "011_rBg_11",
        100.0f,
    },
    {
        // 33% from scene_ambient * white light +
        // material_diffuse (red) with a white light
        "301_rG0_11",
        100.0f,
    },
    {
        // 33% from scene_ambient * white light +
        // material_diffuse (red) with a white light
        "310_rG0_11",
        100.0f,
    },
    {
        // 33% from scene_ambient * white light +
        // 33% from scene_ambient * white material +
        // material_diffuse (red) with a white light
        "311_rG0_11",
        100.0f,
    },
};
// clang-format on

MaterialColorTests::MaterialColorTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Material color", config) {
  for (const auto& test_case : kTests) {
    auto config = test_case.BuildConfig();
    auto test = [this, config]() { this->Test(config); };
    tests_[config.name] = test;
  }
}

void MaterialColorTests::Initialize() {
  TestSuite::Initialize();

  host_.SetVertexShaderProgram(nullptr);
  CreateGeometry();
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
}

void MaterialColorTests::CreateGeometry() {
  float left = -2.75f;
  float right = 2.75f;
  float top = 1.75f;
  float bottom = -1.75f;

  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(6);

  buffer->DefineBiTri(0, left, top, right, bottom);

  // Point normals for half the quad away from the camera.
  Vertex* v = buffer->Lock();
  v[0].normal[2] = -1.0f;
  v[1].normal[2] = -1.0f;
  v[2].normal[2] = -1.0f;
  buffer->Unlock();
}

void MaterialColorTests::Test(TestConfig config) {
  static constexpr uint32_t kBackgroundColor = 0xFF303030;
  host_.PrepareDraw(kBackgroundColor);

  Pushbuffer::Begin();

  Pushbuffer::Push(NV097_SET_LIGHT_ENABLE_MASK, NV097_SET_LIGHT_ENABLE_MASK_LIGHT0_INFINITE);

  Pushbuffer::Push(NV097_SET_LIGHT_LOCAL_RANGE, 0x7149f2ca);  // 1e30
  Pushbuffer::Push(NV097_SET_LIGHT_INFINITE_HALF_VECTOR, 0, 0, 0);
  Pushbuffer::PushF(NV097_SET_LIGHT_INFINITE_DIRECTION, 0.0f, 0.0f, 1.0f);

  Pushbuffer::Push(NV097_SET_CONTROL0, 0x100001);

  Pushbuffer::Push(NV10_TCL_PRIMITIVE_3D_POINT_PARAMETERS_ENABLE, false);

  Pushbuffer::Push(NV097_SET_LIGHT_CONTROL, 0x10001);

  Pushbuffer::Push(NV097_SET_LIGHTING_ENABLE, true);
  Pushbuffer::Push(NV097_SET_SPECULAR_ENABLE, true);

  // Setup colors similar to XDK behavior.
  float r, g, b;

  // NV097_SET_SCENE_AMBIENT_COLOR is a combination of scene ambient (D3DRS_AMBIENT), material ambient, and material
  // emissive:
  // (scene.Ambient.rgb * material.Ambient.rgb + material.Emissive.rgb) => NV097_SET_SCENE_AMBIENT_COLOR
  r = config.scene_ambient.r * config.material_ambient.r + config.material_emissive.r;
  g = config.scene_ambient.g * config.material_ambient.g + config.material_emissive.g;
  b = config.scene_ambient.b * config.material_ambient.b + config.material_emissive.b;
  Pushbuffer::PushF(NV097_SET_SCENE_AMBIENT_COLOR, r, g, b);

  // NV097_SET_LIGHT_AMBIENT_COLOR is a product of scene ambient (D3DRS_AMBIENT) and light ambient
  // (scene.Ambient.rgb * light.Ambient.rgb) => NV097_SET_LIGHT_AMBIENT_COLOR
  r = config.scene_ambient.r * config.light_ambient.r;
  g = config.scene_ambient.g * config.light_ambient.g;
  b = config.scene_ambient.b * config.light_ambient.b;
  Pushbuffer::PushF(NV097_SET_LIGHT_AMBIENT_COLOR, r, g, b);

  // material.Diffuse.rgb * light.Diffuse.rgb => NV097_SET_LIGHT_DIFFUSE_COLOR
  r = config.material_diffuse.r * config.light_diffuse.r;
  g = config.material_diffuse.g * config.light_diffuse.g;
  b = config.material_diffuse.b * config.light_diffuse.b;
  Pushbuffer::PushF(NV097_SET_LIGHT_DIFFUSE_COLOR, r, g, b);

  // material.Diffuse.a => NV097_SET_MATERIAL_ALPHA (Depending on NV097_SET_LIGHT_DIFFUSE_COLOR, assuming it's set up to
  // come from material and not diffuse/specular)
  Pushbuffer::PushF(NV097_SET_MATERIAL_ALPHA, config.material_diffuse.a);

  // material.Specular.rgb * light.Specular.rgb => NV097_SET_LIGHT_SPECULAR_COLOR
  r = config.material_specular.r * config.light_specular.r;
  g = config.material_specular.g * config.light_specular.g;
  b = config.material_specular.b * config.light_specular.b;
  Pushbuffer::PushF(NV097_SET_LIGHT_SPECULAR_COLOR, r, g, b);

  // material.a // Ignored? Maybe it goes into NV097_SET_SPECULAR_PARAMS?
  // material.Power = 125.0f;
  Pushbuffer::Push(NV097_SET_SPECULAR_PARAMS, 0xBF78DF9C);       // -0.972162
  Pushbuffer::Push(NV097_SET_SPECULAR_PARAMS + 4, 0xC04D3531);   // -3.20637
  Pushbuffer::Push(NV097_SET_SPECULAR_PARAMS + 8, 0x404EFD4A);   // 3.23421
  Pushbuffer::Push(NV097_SET_SPECULAR_PARAMS + 12, 0xBF71F52E);  // -0.945147
  Pushbuffer::Push(NV097_SET_SPECULAR_PARAMS + 16, 0xC048FA21);  // -3.14027
  Pushbuffer::Push(NV097_SET_SPECULAR_PARAMS + 20, 0x404C7CD6);  // 3.19512

  /*
  // material.Power = 10.0f;
  Pushbuffer::Push( NV097_SET_SPECULAR_PARAMS, 0xBF34DCE5);  // -0.706496
  Pushbuffer::Push( NV097_SET_SPECULAR_PARAMS + 4, 0xC020743F);  // -2.5071
  Pushbuffer::Push( NV097_SET_SPECULAR_PARAMS + 8, 0x40333D06);  // 2.8006
  Pushbuffer::Push( NV097_SET_SPECULAR_PARAMS + 12, 0xBF003612);  // -0.500825
  Pushbuffer::Push( NV097_SET_SPECULAR_PARAMS + 16, 0xBFF852A5);  // -1.94002
  Pushbuffer::Push( NV097_SET_SPECULAR_PARAMS + 20, 0x401C1BCE);  // 2.4392
 */

  // Material emission is already incorporated into scene_ambient_color above and is set to 0. In cases where ambient
  // or emissive is taken from vertex colors rather than the material, this would be non-zero.
  Pushbuffer::PushF(NV097_SET_MATERIAL_EMISSION, 0, 0, 0);

  Pushbuffer::End();

  host_.DrawArrays(host_.POSITION | host_.NORMAL);

  pb_print("%s\n", config.name);
  pb_printat(2, 25, (char*)" Away from light");
  pb_printat(15, 17, (char*)"Towards light");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, config.name);
}
