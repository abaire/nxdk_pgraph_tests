#include "attribute_carryover_tests.h"

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

static constexpr TestHost::DrawPrimitive kPrimitives[] = {
    TestHost::PRIMITIVE_LINES,
    TestHost::PRIMITIVE_TRIANGLES,
};

static constexpr AttributeCarryoverTests::DrawMode kDrawModes[] = {
    AttributeCarryoverTests::DRAW_ARRAYS,
    AttributeCarryoverTests::DRAW_INLINE_BUFFERS,
    AttributeCarryoverTests::DRAW_INLINE_ARRAYS,
    AttributeCarryoverTests::DRAW_INLINE_ELEMENTS,
};

static constexpr AttributeCarryoverTests::Attribute kTestAttributes[]{
    //    AttributeCarryoverTests::ATTR_WEIGHT,
    AttributeCarryoverTests::ATTR_NORMAL,
    AttributeCarryoverTests::ATTR_DIFFUSE,
    AttributeCarryoverTests::ATTR_SPECULAR,
    //    AttributeCarryoverTests::ATTR_FOG_COORD,  // Not supported by cg.
    //    AttributeCarryoverTests::ATTR_POINT_SIZE,  // cg errors indicating not available on hardware.
    //    AttributeCarryoverTests::ATTR_BACK_DIFFUSE,  // Not supported by cg.
    //    AttributeCarryoverTests::ATTR_BACK_SPECULAR,  // Not supported by cg.
    AttributeCarryoverTests::ATTR_TEX0,
    AttributeCarryoverTests::ATTR_TEX1,
    AttributeCarryoverTests::ATTR_TEX2,
    AttributeCarryoverTests::ATTR_TEX3,
};

static constexpr const float kTestValues[][4] = {
    {0.0f, 0.0f, 1.0f, 1.0f},
    {0.5f, 0.0f, 0.0f, 1.0f},
};

static TestHost::VertexAttribute TestAttributeToVertexAttribute(AttributeCarryoverTests::Attribute attribute);

AttributeCarryoverTests::AttributeCarryoverTests(TestHost& host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Attrib carryover") {
  for (auto draw_mode : kDrawModes) {
    for (auto primitive : kPrimitives) {
      for (auto attr : kTestAttributes) {
        for (auto value : kTestValues) {
          std::string name = MakeTestName(primitive, attr, value, draw_mode);
          tests_[name] = [this, primitive, attr, value, draw_mode]() { this->Test(primitive, attr, value, draw_mode); };
        }
      }
    }
  }
}

void AttributeCarryoverTests::Initialize() {
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

void AttributeCarryoverTests::Deinitialize() {
  bleed_buffer_.reset();
  test_buffer_.reset();
  TestSuite::Deinitialize();
}

void AttributeCarryoverTests::CreateGeometry(TestHost::DrawPrimitive primitive) {
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

  if (primitive == TestHost::PRIMITIVE_LINES) {
    {
      bleed_buffer_ = host_.AllocateVertexBuffer(2);

      auto vertex = bleed_buffer_->Lock();
      vertex->SetPosition(left, top, z);

      // Flag the vertex used in inline-element mode to all green (and a reddish weight).
      vertex->weight[0] = 0.66f;
      vertex->SetNormal(0.0f, 1.0f, 0.0f);
      vertex->SetDiffuse(0.0f, 1.0f, 0.0f, 1.0f);
      vertex->SetSpecular(0.0f, 1.0f, 0.0f, 1.0f);
      vertex->SetTexCoord0(0.0f, 1.0f);
      vertex->SetTexCoord1(0.0f, 1.0f);
      vertex->SetTexCoord2(0.0f, 1.0f);
      vertex->SetTexCoord3(0.0f, 1.0f);
      ++vertex;

      // The last vertex will have its attributes set by the test code, set a default diffuse.
      vertex->SetPosition(right, top, z);
      vertex->SetDiffuse(1.0f, 0.0f, 1.0f, 1.0f);
      ++vertex;
      bleed_buffer_->Unlock();
    }

    {
      test_buffer_ = host_.AllocateVertexBuffer(2);
      auto vertex = test_buffer_->Lock();
      vertex->SetPosition(left, bottom, z);
      ++vertex;
      vertex->SetPosition(right, bottom, z);
      ++vertex;
      test_buffer_->Unlock();
    }

    index_buffer_.clear();
    index_buffer_.push_back(1);  // Intentionally specify the vertices out of order.
    index_buffer_.push_back(0);
  } else {
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

    index_buffer_.clear();
    index_buffer_.push_back(2);  // Intentionally specify the vertices out of order.
    index_buffer_.push_back(0);
    index_buffer_.push_back(1);
  }
}

void AttributeCarryoverTests::Test(TestHost::DrawPrimitive primitive, AttributeCarryoverTests::Attribute test_attribute,
                                   const float* attribute_value, AttributeCarryoverTests::DrawMode draw_mode) {
  auto shader = host_.GetShaderProgram();

  static constexpr uint32_t kBackgroundColor = 0xFF444444;
  CreateGeometry(primitive);
  host_.PrepareDraw(kBackgroundColor);

  auto draw = [this, draw_mode, primitive](uint32_t additional_fields) {
    uint32_t attributes = host_.POSITION | additional_fields;
    switch (draw_mode) {
      case DRAW_ARRAYS:
        host_.DrawArrays(attributes, primitive);
        break;

      case DRAW_INLINE_BUFFERS:
        host_.DrawInlineBuffer(attributes, primitive);
        break;

      case DRAW_INLINE_ELEMENTS:
        host_.DrawInlineElements16(index_buffer_, attributes, primitive);
        break;

      case DRAW_INLINE_ARRAYS:
        host_.DrawInlineArray(attributes, primitive);
        break;
    }
  };

  // TODO: Set the attribute under test.
  auto vertex = bleed_buffer_->Lock();
  switch (primitive) {
    case TestHost::PRIMITIVE_LINES:
      // Set the second vertex in the line.
      ++vertex;
      break;

    case TestHost::PRIMITIVE_TRIANGLES:
      // Set the last vertex in the triangle.
      vertex += 2;
      break;

    default:
      ASSERT(!"TODO: Implement additional primitives.");
      break;
  }

  switch (test_attribute) {
    case ATTR_WEIGHT:
      vertex->SetWeight(attribute_value);
      break;
    case ATTR_NORMAL:
      vertex->SetNormal(attribute_value);
      break;
    case ATTR_DIFFUSE:
      vertex->SetDiffuse(attribute_value);
      break;
    case ATTR_SPECULAR:
      vertex->SetSpecular(attribute_value);
      break;
    case ATTR_FOG_COORD:
    case ATTR_POINT_SIZE:
    case ATTR_BACK_DIFFUSE:
    case ATTR_BACK_SPECULAR:
      ASSERT(!"Attribute not supported.");
      break;
    case ATTR_TEX0:
      vertex->SetTexCoord0(attribute_value);
      break;
    case ATTR_TEX1:
      vertex->SetTexCoord1(attribute_value);
      break;
    case ATTR_TEX2:
      vertex->SetTexCoord2(attribute_value);
      break;
    case ATTR_TEX3:
      vertex->SetTexCoord3(attribute_value);
      break;
  }
  bleed_buffer_->Unlock();

  // The bleed buffer should always render its diffuse color, regardless of the attribute under test.
  shader->SetUniformF(0, ATTR_DIFFUSE);
  host_.SetVertexBuffer(bleed_buffer_);
  draw(host_.DIFFUSE | TestAttributeToVertexAttribute(test_attribute));

  // The test buffer should render the attribute under test as its diffuse color.
  shader->SetUniformF(0, test_attribute);
  host_.SetVertexBuffer(test_buffer_);
  draw(0);

  std::string name = MakeTestName(primitive, test_attribute, attribute_value, draw_mode);
  pb_print("%s", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, name);
}

std::string AttributeCarryoverTests::MakeTestName(TestHost::DrawPrimitive primitive,
                                                  AttributeCarryoverTests::Attribute test_attribute,
                                                  const float* attribute_value,
                                                  AttributeCarryoverTests::DrawMode draw_mode) {
  std::string ret;

  switch (primitive) {
    case TestHost::PRIMITIVE_TRIANGLES:
      ret = "T";
      break;

    case TestHost::PRIMITIVE_LINES:
      ret = "L";
      break;

    default:
      ASSERT(!"Unsupported primitive");
  }

  switch (test_attribute) {
    case ATTR_WEIGHT:
      ret += "-w";
      break;
    case ATTR_NORMAL:
      ret += "-n";
      break;
    case ATTR_DIFFUSE:
      ret += "-d";
      break;
    case ATTR_SPECULAR:
      ret += "-s";
      break;
    case ATTR_FOG_COORD:
      ret += "-fc";
      break;
    case ATTR_POINT_SIZE:
      ret += "-ps";
      break;
    case ATTR_BACK_DIFFUSE:
      ret += "-bd";
      break;
    case ATTR_BACK_SPECULAR:
      ret += "-bs";
      break;
    case ATTR_TEX0:
      ret += "-t0";
      break;
    case ATTR_TEX1:
      ret += "-t1";
      break;
    case ATTR_TEX2:
      ret += "-t2";
      break;
    case ATTR_TEX3:
      ret += "-t3";
      break;
  }

  char buf[32] = {0};
  snprintf(buf, 31, "%.1f_%.1f_%.1f_%.1f", attribute_value[0], attribute_value[1], attribute_value[2],
           attribute_value[3]);
  ret += buf;

  switch (draw_mode) {
    case DRAW_ARRAYS:
      ret += "-da";
      break;
    case DRAW_INLINE_BUFFERS:
      ret += "-ib";
      break;
    case DRAW_INLINE_ARRAYS:
      ret += "-ia";
      break;
    case DRAW_INLINE_ELEMENTS:
      ret += "-ie";
      break;
  }

  return ret;
}

static TestHost::VertexAttribute TestAttributeToVertexAttribute(AttributeCarryoverTests::Attribute attribute) {
  switch (attribute) {
    case AttributeCarryoverTests::ATTR_WEIGHT:
      return TestHost::WEIGHT;
    case AttributeCarryoverTests::ATTR_NORMAL:
      return TestHost::NORMAL;
    case AttributeCarryoverTests::ATTR_DIFFUSE:
      return TestHost::DIFFUSE;
    case AttributeCarryoverTests::ATTR_SPECULAR:
      return TestHost::SPECULAR;
    case AttributeCarryoverTests::ATTR_FOG_COORD:
      return TestHost::FOG_COORD;
    case AttributeCarryoverTests::ATTR_POINT_SIZE:
      return TestHost::POINT_SIZE;
    case AttributeCarryoverTests::ATTR_BACK_DIFFUSE:
      return TestHost::BACK_DIFFUSE;
    case AttributeCarryoverTests::ATTR_BACK_SPECULAR:
      return TestHost::BACK_SPECULAR;
    case AttributeCarryoverTests::ATTR_TEX0:
      return TestHost::TEXCOORD0;
    case AttributeCarryoverTests::ATTR_TEX1:
      return TestHost::TEXCOORD1;
    case AttributeCarryoverTests::ATTR_TEX2:
      return TestHost::TEXCOORD2;
    case AttributeCarryoverTests::ATTR_TEX3:
      return TestHost::TEXCOORD3;
  }
}
