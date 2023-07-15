#include "attribute_explicit_setter_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "vertex_buffer.h"

// clang format off
static constexpr uint32_t kShader[] = {
#include "attribute_explicit_setter_tests.inl"
};
// clang format on

static constexpr AttributeExplicitSetterTests::TestConfig kTestConfigs[] = {
    {"Setters-alpha", false},
    {"Setters-visible", true},
};

// static TestHost::VertexAttribute TestAttributeToVertexAttribute(AttributeExplicitSetterTests::Attribute attribute);

AttributeExplicitSetterTests::AttributeExplicitSetterTests(TestHost& host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Attrib setter") {
  for (auto& config : kTestConfigs) {
    tests_[config.test_name] = [this, &config]() { this->Test(config); };
  }
}

void AttributeExplicitSetterTests::Initialize() {
  TestSuite::Initialize();

  auto shader = std::make_shared<PrecalculatedVertexShader>();
  shader->SetShaderOverride(kShader, sizeof(kShader));

  // Send shader constants (see attribute_carryover_test.inl)
  // const c[3] = 1 0 2 3
  shader->SetUniformF(3, 1, 0, 2, 3);
  // const c[4] = 8 9 10 11
  shader->SetUniformF(4, 8, 9, 10, 11);
  // const c[5] = 1 0 0.75
  shader->SetUniformF(5, 1.0f, 0.0f, 0.75f);

  host_.SetVertexShaderProgram(shader);

  CreateGeometry();
}

void AttributeExplicitSetterTests::CreateGeometry() {
  const auto fb_width = static_cast<float>(host_.GetFramebufferWidth());
  const auto fb_height = static_cast<float>(host_.GetFramebufferHeight());

  uint32_t num_quads = 1;
  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(6 * num_quads);
  buffer->SetPositionIncludesW();

  Color black{0.0f, 0.0f, 0.0f, 1.0f};

  float z = 10.0f;
  buffer->DefineBiTri(0, 0, 0, fb_width, fb_height, z, z, z, z, black, black, black, black);
}

void AttributeExplicitSetterTests::Test(const TestConfig& config) {
  const auto fb_width = static_cast<float>(host_.GetFramebufferWidth());
  const auto fb_height = static_cast<float>(host_.GetFramebufferHeight());
  const float left = floorf(fb_width / 5.0f);
  const float right = left + (fb_width - left * 2.0f);
  const float top = floorf(fb_height / 6.0f);

  static constexpr uint32_t kBackgroundColor = 0xFF333333;
  host_.PrepareDraw(kBackgroundColor);
  host_.SetBlend();

  if (config.force_blend_alpha) {
    // Render a background to visualize alpha values.
    auto shader = host_.GetShaderProgram();
    shader->SetUniformF(0, ATTR_DIFFUSE);
    shader->SetUniformF(1, 0.0f, 1.0f);
    shader->PrepareDraw();
    host_.DrawArrays();
  }

  test_triangle_width = 45;
  test_triangle_height = 40;

  float x = left;
  float y = top;

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_NORMALIZATION_ENABLE, false);
  pb_end(p);

#define DO_DRAW(method, attrib, mask, bias, multiplier)           \
  do {                                                            \
    Draw(x, y, (method), (attrib), (mask), (bias), (multiplier)); \
    x += test_triangle_width + 7;                                 \
    if (x > (right - test_triangle_width)) {                      \
      y += test_triangle_height + 32;                             \
      x = left;                                                   \
    }                                                             \
  } while (0)

#define F2S(val) ((int16_t)((val)*65535.0f - 32768.0f))

  // Normals

  auto set_normal_3f = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetNormal(1.0f, 0.0f, 0.0f);
        break;

      case 1:
        host_.SetNormal(0.5f, 0.5f, 0.8f);
        break;

      case 2:
        host_.SetNormal(0.0f, 0.0f, 1.0f);
        break;
    }
  };
  DO_DRAW(set_normal_3f, ATTR_NORMAL, 0x7, 0.0f, 1.0f);
  pb_printat(1, 11, (char*)"n3f");

  auto set_normal_3s = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetNormal3S(32767, -32768, 0);
        break;

      case 1:
        host_.SetNormal3S(1024, 16536, -32768);
        break;

      case 2:
        host_.SetNormal3S(-32768, -4500, 24000);
        break;
    }
  };
  DO_DRAW(set_normal_3s, ATTR_NORMAL, 0x7, 1.0f, 0.5f);
  pb_printat(1, 17, (char*)"n3s");

  // Diffuse

  auto set_diffuse_4ub = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetDiffuse(0xFFFF0000);
        break;

      case 1:
        host_.SetDiffuse(0xFF0000FF);
        break;

      case 2:
        host_.SetDiffuse(0xFF00FF00);
        break;
    }
  };
  DO_DRAW(set_diffuse_4ub, ATTR_DIFFUSE, 0xF, 0.0f, 1.0f);
  pb_printat(1, 22, (char*)"d4i");

  auto set_diffuse_3f = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetDiffuse(0.0f, 0.0f, 1.0f);
        break;

      case 1:
        host_.SetDiffuse(1.0f, 0.0f, 0.0f);
        break;

      case 2:
        host_.SetDiffuse(0.0f, 1.0f, 0.0f);
        break;
    }
  };
  DO_DRAW(set_diffuse_3f, ATTR_DIFFUSE, 0xF, 0.0f, 1.0f);
  pb_printat(1, 27, (char*)"d3f");

  auto set_diffuse_4f = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetDiffuse(0.0f, 0.0f, 1.0f, 0.75f);
        break;

      case 1:
        host_.SetDiffuse(1.0f, 0.0f, 0.0f, 0.5f);
        break;

      case 2:
        host_.SetDiffuse(0.0f, 1.0f, 0.0f, 0.45f);
        break;
    }
  };
  DO_DRAW(set_diffuse_4f, ATTR_DIFFUSE, 0xF, 0.0f, 1.0f);
  pb_printat(1, 32, (char*)"d4f");

  // Specular

  auto set_specular_4ub = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetSpecular(0xFF00FF00);
        break;

      case 1:
        host_.SetSpecular(0xFFFF0000);
        break;

      case 2:
        host_.SetSpecular(0xFF0000FF);
        break;
    }
  };
  DO_DRAW(set_specular_4ub, ATTR_SPECULAR, 0xF, 0.0f, 1.0f);
  pb_printat(1, 37, (char*)"s4s");

  // Specular
  auto set_specular_3f = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetSpecular(0.0f, 1.0f, 0.0f);
        break;

      case 1:
        host_.SetSpecular(0.0f, 0.0f, 1.0f);
        break;

      case 2:
        host_.SetSpecular(1.0f, 0.0f, 0.0f);
        break;
    }
  };
  DO_DRAW(set_specular_3f, ATTR_SPECULAR, 0x7, 0.0f, 1.0f);
  pb_printat(1, 43, (char*)"s3f");

  auto set_specular_4f = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetSpecular(0.0f, 1.0f, 0.0f, 0.75f);
        break;

      case 1:
        host_.SetSpecular(0.0f, 0.0f, 1.0f, 0.5f);
        break;

      case 2:
        host_.SetSpecular(1.0f, 0.0f, 0.0f, 0.45f);
        break;
    }
  };
  DO_DRAW(set_specular_4f, ATTR_SPECULAR, 0xF, 0.0f, 1.0f);
  pb_printat(4, 11, (char*)"s4f");

  // TextureCoord0

  auto set_tex0_4f = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord0(0.0f, 1.0f, 0.0f, 0.75f);
        break;

      case 1:
        host_.SetTexCoord0(0.0f, 0.0f, 1.0f, 0.5f);
        break;

      case 2:
        host_.SetTexCoord0(1.0f, 0.0f, 0.5f, 0.45f);
        break;
    }
  };
  DO_DRAW(set_tex0_4f, ATTR_TEX0, 0xF, 0.0f, 1.0f);
  pb_printat(4, 17, (char*)"t04f");

  auto set_tex0_2s = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord0S(F2S(0.25f), F2S(0.0f));
        break;

      case 1:
        host_.SetTexCoord0S(F2S(0.0f), F2S(0.75f));
        break;

      case 2:
        host_.SetTexCoord0S(F2S(1.0f), F2S(1.0f));
        break;
    }
  };
  DO_DRAW(set_tex0_2s, ATTR_TEX0, 0x3, 32768.0f, 1.0f / 65535.0f);
  pb_printat(4, 22, (char*)"t02s");

  auto set_tex0_4s = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord0S(F2S(1.0f), F2S(0.0f), F2S(1.0f), F2S(1.0f));
        break;

      case 1:
        host_.SetTexCoord0S(F2S(0.0f), F2S(1.0f), F2S(0.0f), F2S(0.5f));
        break;

      case 2:
        host_.SetTexCoord0S(F2S(0.5f), F2S(0.5f), F2S(0.5f), F2S(0.75f));
        break;
    }
  };
  DO_DRAW(set_tex0_4s, ATTR_TEX0, 0xF, 32768.0f, 1.0f / 65535.0f);
  pb_printat(4, 27, (char*)"t04s");

  auto set_tex0_2f = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord0(1.0f, 0.0f);
        break;

      case 1:
        host_.SetTexCoord0(0.0f, 1.0f);
        break;

      case 2:
        host_.SetTexCoord0(0.65f, 0.65f);
        break;
    }
  };
  DO_DRAW(set_tex0_2f, ATTR_TEX0, 0x3, 0.0f, 1.0f);
  pb_printat(4, 32, (char*)"t02f");

  // TextureCoord1

  auto set_tex1_2f = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord1(1.0f, 0.0f);
        break;

      case 1:
        host_.SetTexCoord1(0.0f, 1.0f);
        break;

      case 2:
        host_.SetTexCoord1(0.65f, 0.65f);
        break;
    }
  };
  DO_DRAW(set_tex1_2f, ATTR_TEX1, 0x3, 0.0f, 1.0f);
  pb_printat(4, 37, (char*)"t12f");

  auto set_tex1_2s = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord1S(F2S(1.0f), F2S(0.0f));
        break;

      case 1:
        host_.SetTexCoord1S(F2S(0.0f), F2S(1.0f));
        break;

      case 2:
        host_.SetTexCoord1S(F2S(0.4f), F2S(0.7f));
        break;
    }
  };
  DO_DRAW(set_tex1_2s, ATTR_TEX1, 0x3, 32768.0f, 1.0f / 65535.0f);
  pb_printat(4, 43, (char*)"t12s");

  auto set_tex1_4f = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord1(0.0f, 0.0f, 1.0f, 0.75f);
        break;

      case 1:
        host_.SetTexCoord1(1.0f, 0.0f, 0.0f, 0.5f);
        break;

      case 2:
        host_.SetTexCoord1(0.0f, 1.0f, 0.0f, 0.45f);
        break;
    }
  };
  DO_DRAW(set_tex1_4f, ATTR_TEX1, 0xF, 0.0f, 1.0f);
  pb_printat(7, 11, (char*)"t14f");

  auto set_tex1_4s = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord1S(F2S(0.0f), F2S(0.0f), F2S(1.0f), F2S(0.75f));
        break;

      case 1:
        host_.SetTexCoord1S(F2S(1.0f), F2S(0.0f), F2S(0.0f), F2S(0.5f));
        break;

      case 2:
        host_.SetTexCoord1S(F2S(0.0f), F2S(1.0f), F2S(0.0f), F2S(0.45f));
        break;
    }
  };
  DO_DRAW(set_tex1_4s, ATTR_TEX1, 0xF, 32768.0f, 1.0f / 65535.0f);
  pb_printat(7, 17, (char*)"t14s");

  // TextureCoord2

  auto set_tex2_2f = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord2(1.0f, 0.0f);
        break;

      case 1:
        host_.SetTexCoord2(0.0f, 1.0f);
        break;

      case 2:
        host_.SetTexCoord2(0.65f, 0.65f);
        break;
    }
  };
  DO_DRAW(set_tex2_2f, ATTR_TEX2, 0x3, 0.0f, 1.0f);
  pb_printat(7, 22, (char*)"t22f");

  auto set_tex2_2i = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord2S(F2S(1.0f), F2S(0.0f));
        break;

      case 1:
        // 0.5 as a 32-bit float >> 16.
        host_.SetTexCoord2S(F2S(0.0f), F2S(1.0f));
        break;

      case 2:
        host_.SetTexCoord2S(F2S(0.6f), F2S(0.6f));
        break;
    }
  };
  DO_DRAW(set_tex2_2i, ATTR_TEX2, 0x3, 32768.0f, 1.0f / 65535.0f);
  pb_printat(7, 27, (char*)"t22s");

  auto set_tex2_4f = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord2(1.0f, 0.0f, 0.0f, 0.75f);
        break;

      case 1:
        host_.SetTexCoord2(0.0f, 0.0f, 1.0f, 0.5f);
        break;

      case 2:
        host_.SetTexCoord2(0.0f, 1.0f, 0.0f, 0.45f);
        break;
    }
  };
  DO_DRAW(set_tex2_4f, ATTR_TEX2, 0xF, 0.0f, 1.0f);
  pb_printat(7, 32, (char*)"t24f");

  auto set_tex2_4s = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord2S(F2S(1.0f), F2S(0.0f), F2S(0.0f), F2S(1.0f));
        break;

      case 1:
        host_.SetTexCoord2S(F2S(0.0f), F2S(1.0f), F2S(0.0f), F2S(1.0f));
        break;

      case 2:
        host_.SetTexCoord2S(F2S(0.0f), F2S(0.0f), F2S(1.0f), F2S(1.0f));
        break;
    }
  };
  DO_DRAW(set_tex2_4s, ATTR_TEX2, 0xF, 32768.0f, 1.0f / 65535.0f);
  pb_printat(7, 37, (char*)"t24s");

  // TextureCoord3

  auto set_tex3_2f = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord3(1.0f, 0.0f);
        break;

      case 1:
        host_.SetTexCoord3(0.0f, 1.0f);
        break;

      case 2:
        host_.SetTexCoord3(0.65f, 0.65f);
        break;
    }
  };
  DO_DRAW(set_tex3_2f, ATTR_TEX3, 0x3, 0.0f, 1.0f);
  pb_printat(7, 43, (char*)"t32f");

  auto set_tex3_2s = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord3S(F2S(1.0f), F2S(0.0f));
        break;

      case 1:
        host_.SetTexCoord3S(F2S(0.0f), F2S(1.0f));
        break;

      case 2:
        host_.SetTexCoord3S(F2S(0.3f), F2S(0.3f));
        break;
    }
  };
  DO_DRAW(set_tex3_2s, ATTR_TEX3, 0x3, 32768.0f, 1.0f / 65535.0f);
  pb_printat(10, 11, (char*)"t32s");

  auto set_tex3_4f = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord3(0.0f, 1.0f, 0.0f, 0.75f);
        break;

      case 1:
        host_.SetTexCoord3(1.0f, 0.0f, 0.0f, 0.5f);
        break;

      case 2:
        host_.SetTexCoord3(0.0f, 0.0f, 1.0f, 0.45f);
        break;
    }
  };
  DO_DRAW(set_tex3_4f, ATTR_TEX3, 0xF, 0.0f, 1.0f);
  pb_printat(10, 17, (char*)"t34f");

  auto set_tex3_4s = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord3S(F2S(1.0f), F2S(0.0f), F2S(0.0f), F2S(1.0f));
        break;

      case 1:
        host_.SetTexCoord3S(F2S(0.0f), F2S(1.0f), F2S(0.0f), F2S(1.0f));
        break;

      case 2:
        host_.SetTexCoord3S(F2S(0.0f), F2S(0.1f), F2S(1.0f), F2S(0.8f));
        break;
    }
  };
  DO_DRAW(set_tex3_4s, ATTR_TEX3, 0xF, 32768.0f, 1.0f / 65535.0f);
  pb_printat(10, 22, (char*)"t34s");

#undef F2S
#undef DO_DRAW

  pb_printat(0, 0, (char*)config.test_name);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, config.test_name);
}

void AttributeExplicitSetterTests::Draw(float x, float y, const std::function<void(int)>& attribute_setter,
                                        Attribute test_attribute, int mask, float bias, float multiplier) {
  auto shader = host_.GetShaderProgram();
  shader->SetUniformF(0, test_attribute);
  float float_mask[4];
  float_mask[0] = (mask & 0x01) != 0;
  float_mask[1] = (mask & 0x02) != 0;
  float_mask[2] = (mask & 0x04) != 0;
  float_mask[3] = (mask & 0x08) != 0;
  shader->SetUniform4F(1, float_mask);
  shader->SetUniformF(2, bias, multiplier);
  shader->PrepareDraw();

  const float z = 3.0f;
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_BEGIN_END, TestHost::PRIMITIVE_TRIANGLES);
  pb_end(p);

  attribute_setter(0);
  host_.SetVertex(x, y, z);

  attribute_setter(1);
  host_.SetVertex(x + test_triangle_width, y, z);

  attribute_setter(2);
  host_.SetVertex(x + (test_triangle_width * 0.5f), y + test_triangle_height, z);

  p = pb_begin();
  p = pb_push1(p, NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
  pb_end(p);
}

// static TestHost::VertexAttribute TestAttributeToVertexAttribute(AttributeExplicitSetterTests::Attribute attribute) {
//   switch (attribute) {
//     case AttributeExplicitSetterTests::ATTR_WEIGHT:
//       return TestHost::WEIGHT;
//     case AttributeExplicitSetterTests::ATTR_NORMAL:
//       return TestHost::NORMAL;
//     case AttributeExplicitSetterTests::ATTR_DIFFUSE:
//       return TestHost::DIFFUSE;
//     case AttributeExplicitSetterTests::ATTR_SPECULAR:
//       return TestHost::SPECULAR;
//     case AttributeExplicitSetterTests::ATTR_FOG_COORD:
//       return TestHost::FOG_COORD;
//     case AttributeExplicitSetterTests::ATTR_POINT_SIZE:
//       return TestHost::POINT_SIZE;
//     case AttributeExplicitSetterTests::ATTR_BACK_DIFFUSE:
//       return TestHost::BACK_DIFFUSE;
//     case AttributeExplicitSetterTests::ATTR_BACK_SPECULAR:
//       return TestHost::BACK_SPECULAR;
//     case AttributeExplicitSetterTests::ATTR_TEX0:
//       return TestHost::TEXCOORD0;
//     case AttributeExplicitSetterTests::ATTR_TEX1:
//       return TestHost::TEXCOORD1;
//     case AttributeExplicitSetterTests::ATTR_TEX2:
//       return TestHost::TEXCOORD2;
//     case AttributeExplicitSetterTests::ATTR_TEX3:
//       return TestHost::TEXCOORD3;
//   }
// }
