#include "attribute_explicit_setter_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "vertex_buffer.h"

// clang format off
static constexpr uint32_t kShader[] = {
#include "shaders/attribute_carryover_test.inl"
};
// clang format on

static constexpr AttributeExplicitSetterTests::TestConfig kTestConfigs[] = {
    {"Setters-alpha", false},
    {"Setters-visible", true},
};

static TestHost::VertexAttribute TestAttributeToVertexAttribute(AttributeExplicitSetterTests::Attribute attribute);

AttributeExplicitSetterTests::AttributeExplicitSetterTests(TestHost& host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Attrib setter") {
  for (auto& config : kTestConfigs) {
    tests_[config.test_name] = [this, &config]() { this->Test(config); };
  }
}

void AttributeExplicitSetterTests::Initialize() {
  TestSuite::Initialize();

  auto shader = std::make_shared<PrecalculatedVertexShader>(false);
  shader->SetShaderOverride(kShader, sizeof(kShader));

  // Send shader constants (see attribute_carryover_test.inl)
  // const c[2] = 1 0 2 3
  shader->SetUniformF(1, 1, 0, 2, 3);
  // const c[3] = 8 9 10 11
  shader->SetUniformF(2, 8, 9, 10, 11);
  // const c[4] = 1 0 0.75
  shader->SetUniformF(3, 1.0f, 0.0f, 0.75f);

  host_.SetShaderProgram(shader);

  CreateGeometry();
}

void AttributeExplicitSetterTests::CreateGeometry() {
  const auto fb_width = static_cast<float>(host_.GetFramebufferWidth());
  const auto fb_height = static_cast<float>(host_.GetFramebufferHeight());

  uint32_t num_quads = 1;
  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(6 * num_quads);
  buffer->SetPositionIncludesW();

  Color black{0.0f, 0.0f, 0.0f, 1.0f};

  uint32_t idx = 0;
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
  host_.SetAlphaBlendEnabled();

  if (config.force_blend_alpha) {
    // Render a background to visualize alpha values.
    auto shader = host_.GetShaderProgram();
    shader->SetUniformF(0, ATTR_DIFFUSE);
    shader->PrepareDraw();
    host_.DrawArrays();
  }

  test_triangle_width = 45;
  test_triangle_height = 40;

  float x = left;
  float y = top;

#define DO_DRAW(method, attrib)              \
  do {                                       \
    Draw(x, y, (method), (attrib));          \
    x += test_triangle_width + 7;            \
    if (x > (right - test_triangle_width)) { \
      y += test_triangle_height + 32;        \
      x = left;                              \
    }                                        \
  } while (0)

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
  DO_DRAW(set_normal_3f, ATTR_NORMAL);
  pb_printat(1, 11, (char*)"n3f");

  auto set_normal_3s = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetNormal3S(0x7FFF, 0, 0);
        break;

      case 1:
        host_.SetNormal3S(0, 0x1FFF, 0);
        break;

      case 2:
        host_.SetNormal3S(0, 0, 0x5000);
        break;
    }
  };
  DO_DRAW(set_normal_3s, ATTR_NORMAL);
  pb_printat(1, 17, (char*)"n3s");

  // Diffuse

  auto set_diffuse_4i = [this](int index) {
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
  DO_DRAW(set_diffuse_4i, ATTR_DIFFUSE);
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
  DO_DRAW(set_diffuse_3f, ATTR_DIFFUSE);
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
  DO_DRAW(set_diffuse_4f, ATTR_DIFFUSE);
  pb_printat(1, 32, (char*)"d4f");

  // Specular

  auto set_specular_4i = [this](int index) {
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
  DO_DRAW(set_specular_4i, ATTR_SPECULAR);
  pb_printat(1, 37, (char*)"s4i");

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
  DO_DRAW(set_specular_3f, ATTR_SPECULAR);
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
  DO_DRAW(set_specular_4f, ATTR_SPECULAR);
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
  DO_DRAW(set_tex0_4f, ATTR_TEX0);
  pb_printat(4, 17, (char*)"t04f");

  auto set_tex0_2i = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord0i(0x4FFF, 0);
        break;

      case 1:
        host_.SetTexCoord0i(0, 0xFFFF);
        break;

      case 2:
        host_.SetTexCoord0i(1, 0x8000);
        break;
    }
  };
  DO_DRAW(set_tex0_2i, ATTR_TEX0);
  pb_printat(4, 22, (char*)"t02s");

  auto set_tex0_4i = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord0i(0x7FFF, 0, 1, 0x7FFF);
        break;

      case 1:
        host_.SetTexCoord0i(0, 0x7FFF, 0, 0x5FFF);
        break;

      case 2:
        host_.SetTexCoord0i(0x1000, 0x3000, 0x6000, 0x4FFF);
        break;
    }
  };
  DO_DRAW(set_tex0_4i, ATTR_TEX0);
  pb_printat(4, 27, (char*)"t04i");

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
  DO_DRAW(set_tex0_2f, ATTR_TEX0);
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
  DO_DRAW(set_tex1_2f, ATTR_TEX1);
  pb_printat(4, 37, (char*)"t12f");

  auto set_tex1_2i = [this](int index) {
    switch (index) {
      default:
      case 0:
        // NaN as a 16-bit float.
        host_.SetTexCoord1i(0x7FFF, 0);
        break;

      case 1:
        // 0.00781 as a 16-bit float
        host_.SetTexCoord1i(0, 0x1FFF);
        break;

      case 2:
        // 0.5 as a 16-bit float.
        host_.SetTexCoord1i(0x3800, 0x3800);
        break;
    }
  };
  DO_DRAW(set_tex1_2i, ATTR_TEX1);
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
  DO_DRAW(set_tex1_4f, ATTR_TEX1);
  pb_printat(7, 11, (char*)"t14f");

  auto set_tex1_4i = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord1i(0x7FFF, 0, 0, 0x7FFF);
        break;

      case 1:
        host_.SetTexCoord1i(0, 0x7FFF, 0, 0x5FFF);
        break;

      case 2:
        host_.SetTexCoord1i(0x1000, 0x3000, 0, 0x4FFF);
        break;
    }
  };
  DO_DRAW(set_tex1_4i, ATTR_TEX1);
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
  DO_DRAW(set_tex2_2f, ATTR_TEX2);
  pb_printat(7, 22, (char*)"t22f");

  auto set_tex2_2i = [this](int index) {
    switch (index) {
      default:
      case 0:
        // NaN as a 32-bit float >> 16.
        host_.SetTexCoord2i(0x7FFF, 0);
        break;

      case 1:
        // 0.5 as a 32-bit float >> 16.
        host_.SetTexCoord2i(0, 0x3F00);
        break;

      case 2:
        // 2.524355e-29 as a 32-bit float >> 16
        // 4.656613e-10 as a 32-bit float >> 16
        host_.SetTexCoord2i(0x1000, 0x3000);
        break;
    }
  };
  DO_DRAW(set_tex2_2i, ATTR_TEX2);
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
  DO_DRAW(set_tex2_4f, ATTR_TEX2);
  pb_printat(7, 32, (char*)"t24f");

  auto set_tex2_4i = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord2i(0x7FFF, 0, 0, 0x7FFF);
        break;

      case 1:
        host_.SetTexCoord2i(0, 0x7FFF, 0, 0x5FFF);
        break;

      case 2:
        host_.SetTexCoord2i(0x1000, 0x3000, 0, 0x4FFF);
        break;
    }
  };
  DO_DRAW(set_tex2_4i, ATTR_TEX2);
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
  DO_DRAW(set_tex3_2f, ATTR_TEX3);
  pb_printat(7, 43, (char*)"t32f");

  auto set_tex3_2i = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord3i(0x7FFF, 0);
        break;

      case 1:
        host_.SetTexCoord3i(0, 0x3FFF);
        break;

      case 2:
        host_.SetTexCoord3i(1, 1);
        break;
    }
  };
  DO_DRAW(set_tex3_2i, ATTR_TEX3);
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
  DO_DRAW(set_tex3_4f, ATTR_TEX3);
  pb_printat(10, 17, (char*)"t34f");

  auto set_tex3_4i = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord3i(0x7FFF, 0, 0, 0x7FFF);
        break;

      case 1:
        host_.SetTexCoord3i(0, 0x7FFF, 0, 0x5FFF);
        break;

      case 2:
        host_.SetTexCoord3i(0x1000, 0x3000, 0, 0x4FFF);
        break;
    }
  };
  DO_DRAW(set_tex3_4i, ATTR_TEX3);
  pb_printat(10, 22, (char*)"t34s");

#undef DO_DRAW

  pb_printat(0, 0, (char*)config.test_name);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, config.test_name);
}

void AttributeExplicitSetterTests::Draw(float x, float y, const std::function<void(int)>& attribute_setter,
                                        Attribute test_attribute) {
  auto shader = host_.GetShaderProgram();
  shader->SetUniformF(0, test_attribute);
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

static TestHost::VertexAttribute TestAttributeToVertexAttribute(AttributeExplicitSetterTests::Attribute attribute) {
  switch (attribute) {
    case AttributeExplicitSetterTests::ATTR_WEIGHT:
      return TestHost::WEIGHT;
    case AttributeExplicitSetterTests::ATTR_NORMAL:
      return TestHost::NORMAL;
    case AttributeExplicitSetterTests::ATTR_DIFFUSE:
      return TestHost::DIFFUSE;
    case AttributeExplicitSetterTests::ATTR_SPECULAR:
      return TestHost::SPECULAR;
    case AttributeExplicitSetterTests::ATTR_FOG_COORD:
      return TestHost::FOG_COORD;
    case AttributeExplicitSetterTests::ATTR_POINT_SIZE:
      return TestHost::POINT_SIZE;
    case AttributeExplicitSetterTests::ATTR_BACK_DIFFUSE:
      return TestHost::BACK_DIFFUSE;
    case AttributeExplicitSetterTests::ATTR_BACK_SPECULAR:
      return TestHost::BACK_SPECULAR;
    case AttributeExplicitSetterTests::ATTR_TEX0:
      return TestHost::TEXCOORD0;
    case AttributeExplicitSetterTests::ATTR_TEX1:
      return TestHost::TEXCOORD1;
    case AttributeExplicitSetterTests::ATTR_TEX2:
      return TestHost::TEXCOORD2;
    case AttributeExplicitSetterTests::ATTR_TEX3:
      return TestHost::TEXCOORD3;
  }
}
