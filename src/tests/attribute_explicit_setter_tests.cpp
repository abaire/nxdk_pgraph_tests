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

static constexpr const char kTestName[] = "Setters";

static TestHost::VertexAttribute TestAttributeToVertexAttribute(AttributeExplicitSetterTests::Attribute attribute);

AttributeExplicitSetterTests::AttributeExplicitSetterTests(TestHost& host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Attrib setter") {
  tests_[kTestName] = [this]() { this->Test(); };
}

void AttributeExplicitSetterTests::Initialize() {
  TestSuite::Initialize();

  auto shader = std::make_shared<PrecalculatedVertexShader>(false);
  shader->SetShaderOverride(kShader, sizeof(kShader));

  // Send shader constants (see attribute_carryover_test.inl)
  // const c[1] = 1 0 2 3
  shader->SetUniformF(1, 1, 0, 2, 3);
  // const c[2] = 8 9 10 11
  shader->SetUniformF(2, 8, 9, 10, 11);
  // const c[3] = 1 0 0.75
  shader->SetUniformF(3, 1.0f, 0.0f, 0.75f);

  host_.SetShaderProgram(shader);
}

/*
void AttributeExplicitSetterTests::CreateGeometry(TestHost::DrawPrimitive primitive) {
  bleed_buffer_.reset();
  test_buffer_.reset();

  auto fb_width = static_cast<float>(host_.GetFramebufferWidth());
  auto fb_height = static_cast<float>(host_.GetFramebufferHeight());

  const float left = floorf(fb_width / 5.0f);
  const float right = left + (fb_width - left * 2.0f);
  const float top = floorf(fb_height / 6.0f);
  const float bottom = top + (fb_height - top * 2.0f);
  const float mid_width = left + (right - left) * 0.5f;
  const float z = 3.0f;

  {
    bleed_buffer_ = host_.AllocateVertexBuffer(3);
    float one_pos[3] = {left, top, z};
    float two_pos[3] = {mid_width, top, z};
    float three_pos[3] = {left + (mid_width - left) * 0.5f, bottom, z};
    bleed_buffer_->DefineTriangle(0, one_pos, two_pos, three_pos);
    auto vertex = bleed_buffer_->Lock();

    // Flag the vertex used in inline-element mode to all green (and a reddish weight).
    vertex[1].weight[0] = 0.66f;
    vertex[1].SetNormal(0.0f, 1.0f, 0.0f);
    vertex[1].SetDiffuse(0.0f, 1.0f, 0.0f, 1.0f);
    vertex[1].SetSpecular(0.0f, 1.0f, 0.0f, 1.0f);
    vertex[1].SetTexCoord0(0.0f, 1.0f);
    vertex[1].SetTexCoord1(0.0f, 1.0f);
    vertex[1].SetTexCoord2(0.0f, 1.0f);
    vertex[1].SetTexCoord3(0.0f, 1.0f);

    // The last vertex will have its attributes set by the test code, set a default diffuse.
    vertex[2].SetDiffuse(1.0f, 0.0f, 1.0f, 1.0f);
    bleed_buffer_->Unlock();
  }

  {
    test_buffer_ = host_.AllocateVertexBuffer(3);
    float one_pos[3] = {mid_width, top, z};
    float two_pos[3] = {right, top, z};
    float three_pos[3] = {mid_width + (right - mid_width) * 0.5f, bottom, z};
    test_buffer_->DefineTriangle(0, one_pos, two_pos, three_pos);
  }
}
*/

void AttributeExplicitSetterTests::Test() {
  const auto fb_width = static_cast<float>(host_.GetFramebufferWidth());
  const auto fb_height = static_cast<float>(host_.GetFramebufferHeight());
  const float left = floorf(fb_width / 5.0f);
  const float right = left + (fb_width - left * 2.0f);
  const float top = floorf(fb_height / 6.0f);
  const float bottom = top + (fb_height - top * 2.0f);
  const float width = right - left;
  const float height = bottom - top;

  static constexpr uint32_t kBackgroundColor = 0xFF444444;
  host_.PrepareDraw(kBackgroundColor);

  test_triangle_width = 30;
  test_triangle_height = 30;

  float x = left;
  float y = top;

#define DO_DRAW(method, attrib)            \
  Draw(x, y, (method), (attrib));          \
  x += test_triangle_width + 5;            \
  if (x > (right - test_triangle_width)) { \
    y += test_triangle_height + 5;         \
    x = left;                              \
  }

  auto set_normal_3f = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetNormal(1.0f, 0.0f, 0.0f);
        break;

      case 1:
        host_.SetNormal(0.0f, 1.0f, 0.0f);
        break;

      case 2:
        host_.SetNormal(0.0f, 0.0f, 1.0f);
        break;
    }
  };
  DO_DRAW(set_normal_3f, ATTR_NORMAL);

  // set_normal_3i

  auto set_diffuse_4f = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetDiffuse(0.0f, 0.0f, 1.0f, 0.5f);
        break;

      case 1:
        host_.SetDiffuse(1.0f, 0.0f, 0.0f, 0.75f);
        break;

      case 2:
        host_.SetDiffuse(0.0f, 1.0f, 0.0f, 1.0f);
        break;
    }
  };
  DO_DRAW(set_diffuse_4f, ATTR_DIFFUSE);

  // set_diffuse_3f

  // set_diffuse_4i

  auto set_specular_4f = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetSpecular(0.0f, 1.0f, 0.0f, 1.0f);
        break;

      case 1:
        host_.SetSpecular(0.0f, 0.0f, 1.0f, 0.5f);
        break;

      case 2:
        host_.SetSpecular(1.0f, 0.0f, 0.0f, 0.75f);
        break;
    }
  };
  DO_DRAW(set_specular_4f, ATTR_SPECULAR);

  // set_specular_3f

  // set_specular_4i

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
        host_.SetTexCoord0(0.25f, 0.45f);
        break;
    }
  };
  DO_DRAW(set_tex0_2f, ATTR_TEX0);

  // set_tex0_2i

  auto set_tex0_4f = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord0(0.0f, 1.0f, 0.0f, 1.0f);
        break;

      case 1:
        host_.SetTexCoord0(0.0f, 0.0f, 1.0f, 0.5f);
        break;

      case 2:
        host_.SetTexCoord0(1.0f, 0.0f, 0.0f, 0.75f);
        break;
    }
  };
  DO_DRAW(set_tex0_4f, ATTR_TEX0);

  auto set_tex1_2f = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord1(0.25f, 0.45f);
        break;

      case 1:
        host_.SetTexCoord1(1.0f, 0.0f);
        break;

      case 2:
        host_.SetTexCoord1(0.0f, 1.0f);
        break;
    }
  };
  DO_DRAW(set_tex1_2f, ATTR_TEX1);

  // set_tex1_2i

  auto set_tex1_4f = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord1(0.0f, 0.0f, 1.0f, 0.5f);
        break;

      case 1:
        host_.SetTexCoord1(1.0f, 0.0f, 0.0f, 0.75f);
        break;

      case 2:
        host_.SetTexCoord1(0.0f, 1.0f, 0.0f, 1.0f);
        break;
    }
  };
  DO_DRAW(set_tex1_4f, ATTR_TEX1);

  auto set_tex2_2f = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord2(0.5f, 0.8f);
        break;

      case 1:
        host_.SetTexCoord2(0.45f, 0.2f);
        break;

      case 2:
        host_.SetTexCoord2(0.0f, 1.0f);
        break;
    }
  };
  DO_DRAW(set_tex2_2f, ATTR_TEX2);

  // set_tex2_2i

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
        host_.SetTexCoord2(0.0f, 1.0f, 0.0f, 1.0f);
        break;
    }
  };
  DO_DRAW(set_tex2_4f, ATTR_TEX2);

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
        host_.SetTexCoord3(0.25f, 0.45f);
        break;
    }
  };
  DO_DRAW(set_tex3_2f, ATTR_TEX3);

  // set_tex3_2i

  auto set_tex3_4f = [this](int index) {
    switch (index) {
      default:
      case 0:
        host_.SetTexCoord3(0.0f, 1.0f, 0.0f, 1.0f);
        break;

      case 1:
        host_.SetTexCoord3(1.0f, 0.0f, 0.0f, 0.75f);
        break;

      case 2:
        host_.SetTexCoord3(0.0f, 0.0f, 1.0f, 0.5f);
        break;
    }
  };
  DO_DRAW(set_tex3_4f, ATTR_TEX3);

#undef DO_DRAW

  host_.FinishDraw(allow_saving_, output_dir_, kTestName);
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
