#include "set_vertex_data_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "vertex_buffer.h"

static constexpr SetVertexDataTests::SetFunction kTests[] = {
    SetVertexDataTests::FUNC_2F_M, SetVertexDataTests::FUNC_4F_M, SetVertexDataTests::FUNC_2S,
    SetVertexDataTests::FUNC_4UB,  SetVertexDataTests::FUNC_4S_M,
};

SetVertexDataTests::SetVertexDataTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "SetVertexData", config) {
  for (auto saturate_sign : {false, true}) {
    for (auto set_func : kTests) {
      std::string name = MakeTestName(set_func, saturate_sign);
      Color diffuse{0.25f, 1.0f, 0.5f, 0.75f};
      tests_[name] = [this, set_func, diffuse, saturate_sign]() { this->Test(set_func, diffuse, saturate_sign); };
    }
  }
}

void SetVertexDataTests::Initialize() {
  TestSuite::Initialize();

  host_.SetVertexShaderProgram(nullptr);
  CreateGeometry();
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
}

void SetVertexDataTests::CreateGeometry() {
  float left = -2.75f;
  float right = 2.75f;
  float top = 1.75f;
  float bottom = -1.75f;
  float mid_width = 0.0f;
  float mid_height = 0.0f;

  float z = 1.0f;

  {
    float one_pos[3] = {left, top, z};
    float two_pos[3] = {mid_width, top, z};
    float three_pos[3] = {left + (mid_width - left) * 0.5f, bottom, z};
    diffuse_buffer_ = host_.AllocateVertexBuffer(3);
    diffuse_buffer_->DefineTriangle(0, one_pos, two_pos, three_pos);
  }
  {
    float one_pos[3] = {mid_width + (right - mid_width) * 0.5f, top, z};
    float two_pos[3] = {right, mid_height, z};
    float three_pos[3] = {mid_width, mid_height, z};
    lit_buffer_ = host_.AllocateVertexBuffer(3);
    lit_buffer_->DefineTriangle(0, one_pos, two_pos, three_pos);
  }
  {
    float one_pos[3] = {mid_width, mid_height, z};
    float two_pos[3] = {right, mid_height, z};
    float three_pos[3] = {mid_width + (right - mid_width) * 0.5f, bottom, z};
    lit_buffer_negative_ = host_.AllocateVertexBuffer(3);
    lit_buffer_negative_->DefineTriangle(0, one_pos, two_pos, three_pos);
  }
}

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

  {
    p = pb_push3(p, NV097_SET_LIGHT_AMBIENT_COLOR, 0, 0, 0);
    p = pb_push3f(p, NV097_SET_LIGHT_DIFFUSE_COLOR, 0.0f, 0.25f, 0.8f);
    p = pb_push3(p, NV097_SET_LIGHT_SPECULAR_COLOR, 0, 0, 0);
    p = pb_push1(p, NV097_SET_LIGHT_LOCAL_RANGE, 0x7149f2ca);  // 1e30
    p = pb_push3(p, NV097_SET_LIGHT_INFINITE_HALF_VECTOR, 0, 0, 0);
    p = pb_push3f(p, NV097_SET_LIGHT_INFINITE_DIRECTION, 1.0f, 0.0f, 0.0f);
  }

  {
    p = pb_push3(p, NV097_SET_LIGHT_AMBIENT_COLOR + 128, 0, 0, 0);
    p = pb_push3f(p, NV097_SET_LIGHT_DIFFUSE_COLOR + 128, 0.8f, 0.25f, 0.33f);
    p = pb_push3(p, NV097_SET_LIGHT_SPECULAR_COLOR + 128, 0, 0, 0);
    p = pb_push1(p, NV097_SET_LIGHT_LOCAL_RANGE + 128, 0x7149f2ca);  // 1e30
    p = pb_push3(p, NV097_SET_LIGHT_INFINITE_HALF_VECTOR + 128, 0, 0, 0);
    p = pb_push3f(p, NV097_SET_LIGHT_INFINITE_DIRECTION + 128, -1.0f, 0.0f, 0.0f);
  }

  pb_end(p);
}

void SetVertexDataTests::Test(SetFunction func, const Color& diffuse, bool saturate_signed) {
  static constexpr uint32_t kBackgroundColor = 0xFF303030;
  host_.PrepareDraw(kBackgroundColor);

  auto get_sign = [saturate_signed](float val) {
    if (val == 0.0f) {
      return 0;
    }

    if (val > 0.0f) {
      if (saturate_signed) {
        return 0x7FFF;
      }

      return 1;
    }

    if (saturate_signed) {
      return 0x8000;
    }

    return 0xFFFF;
  };

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, false);

  switch (func) {
    case FUNC_4UB:
      p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + (4 * NV2A_VERTEX_ATTR_DIFFUSE), diffuse.AsBGRA());
      break;

    case FUNC_4F_M:
      p = pb_push4f(p, NV097_SET_VERTEX_DATA4F_M + (16 * NV2A_VERTEX_ATTR_DIFFUSE), diffuse.b, diffuse.g, diffuse.r,
                    diffuse.a);
      break;

    case FUNC_2S: {
      auto rg_pair = get_sign(diffuse.r) + (get_sign(diffuse.g) << 16);
      p = pb_push1(p, NV097_SET_VERTEX_DATA2S + (4 * NV2A_VERTEX_ATTR_DIFFUSE), rg_pair);
    } break;

    case FUNC_2F_M:
      p = pb_push2f(p, NV097_SET_VERTEX_DATA2F_M + (8 * NV2A_VERTEX_ATTR_DIFFUSE), diffuse.b, diffuse.g);
      break;

    case FUNC_4S_M: {
      auto rg_pair = get_sign(diffuse.r) + (get_sign(diffuse.g) << 16);
      auto ba_pair = get_sign(diffuse.b) + (get_sign(diffuse.a) << 16);
      p = pb_push2(p, NV097_SET_VERTEX_DATA4S_M + (8 * NV2A_VERTEX_ATTR_DIFFUSE), rg_pair, ba_pair);
    } break;
  }

  pb_end(p);

  SetLightAndMaterial();

  host_.SetVertexBuffer(diffuse_buffer_);
  host_.DrawInlineBuffer(host_.POSITION);

  p = pb_begin();
  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, true);
  p = pb_push1(p, NV097_SET_LIGHT_CONTROL, 0x10001);
  p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK,
               NV097_SET_LIGHT_ENABLE_MASK_LIGHT0_INFINITE | NV097_SET_LIGHT_ENABLE_MASK_LIGHT1_INFINITE);

  // Set diffuse to pure white so the resultant color is just from the light.
  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + (4 * NV2A_VERTEX_ATTR_DIFFUSE), 0xFFFFFFFF);
  pb_end(p);

  auto set_normal = [func, get_sign](float x, float y, float z) {
    auto p = pb_begin();
    switch (func) {
      case FUNC_4UB: {
        uint32_t value = ((uint32_t)(x * 255.0f) & 0xFF) + (((uint32_t)(y * 255.0f) & 0xFF) << 8) +
                         (((uint32_t)(z * 255.0f) & 0xFF) << 16);
        p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + (4 * NV2A_VERTEX_ATTR_NORMAL), value);
      } break;

      case FUNC_4F_M:
        p = pb_push4f(p, NV097_SET_VERTEX_DATA4F_M + (16 * NV2A_VERTEX_ATTR_NORMAL), x, y, z, 1.0f);
        break;

      case FUNC_2S: {
        uint32_t xy = get_sign(x) + (get_sign(y) << 16);
        p = pb_push1(p, NV097_SET_VERTEX_DATA2S + (4 * NV2A_VERTEX_ATTR_NORMAL), xy);
      } break;

      case FUNC_2F_M:
        p = pb_push2f(p, NV097_SET_VERTEX_DATA2F_M + (8 * NV2A_VERTEX_ATTR_NORMAL), x, y);
        break;

      case FUNC_4S_M: {
        uint32_t xy = (get_sign(x) + (get_sign(y) << 16));
        uint32_t zw = (get_sign(z) + (1 << 16));
        p = pb_push2(p, NV097_SET_VERTEX_DATA4S_M + (8 * NV2A_VERTEX_ATTR_NORMAL), xy, zw);
      } break;
    }
    pb_end(p);
  };

  set_normal(1.0f, 0.0f, 0.0f);
  host_.SetVertexBuffer(lit_buffer_);
  host_.DrawInlineBuffer(host_.POSITION);

  set_normal(-1.0f, 0.0f, 0.0f);
  host_.SetVertexBuffer(lit_buffer_negative_);
  host_.DrawInlineBuffer(host_.POSITION);

  std::string name = MakeTestName(func, saturate_signed);
  pb_print("%s\n", name.c_str());

  pb_printat(6, 17, (char*)"Diffuse");
  pb_printat(7, 35, (char*)" 1 Normal");
  pb_printat(9, 35, (char*)"-1 Normal");

  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, name);
}

std::string SetVertexDataTests::MakeTestName(SetFunction func, bool saturate_sign) {
  switch (func) {
    case FUNC_2F_M:
      return "SET_VERTEX_DATA2F_M";

    case FUNC_4F_M:
      return "SET_VERTEX_DATA4F_M";

    case FUNC_2S:
      if (saturate_sign) {
        return "SET_VERTEX_DATA2S-7FFF";
      }
      return "SET_VERTEX_DATA2S-0001";

    case FUNC_4UB:
      return "SET_VERTEX_DATA4UB";

    case FUNC_4S_M:
      if (saturate_sign) {
        return "SET_VERTEX_DATA4S_M-7FFF";
      }
      return "SET_VERTEX_DATA4S_M-0001";
  }
}
