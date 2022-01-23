#ifndef NXDK_PGRAPH_TESTS_ATTRIBUTE_CARRYOVER_TESTS_H
#define NXDK_PGRAPH_TESTS_ATTRIBUTE_CARRYOVER_TESTS_H

#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"

class VertexBuffer;

// Tests behavior when vertex attributes are not provided but are used by shaders.
class AttributeCarryoverTests : public TestSuite {
 public:
  enum DrawMode {
    DRAW_ARRAYS,
    DRAW_INLINE_BUFFERS,
    DRAW_INLINE_ARRAYS,
    DRAW_INLINE_ELEMENTS,
  };

  // Keep in sync with attribute_carryover_test.vs.cg
  enum Attribute {
    ATTR_WEIGHT = 0,
    ATTR_NORMAL = 1,
    ATTR_DIFFUSE = 2,
    ATTR_SPECULAR = 3,
    ATTR_FOG_COORD = 4,
    ATTR_POINT_SIZE = 5,
    ATTR_BACK_DIFFUSE = 6,
    ATTR_BACK_SPECULAR = 7,
    ATTR_TEX0 = 8,
    ATTR_TEX1 = 9,
    ATTR_TEX2 = 10,
    ATTR_TEX3 = 11,
  };

 public:
  AttributeCarryoverTests(TestHost& host, std::string output_dir);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void CreateGeometry(TestHost::DrawPrimitive primitive);
  void Test(TestHost::DrawPrimitive primitive, Attribute test_attribute, const float* attribute_value,
            DrawMode draw_mode);

  static std::string MakeTestName(TestHost::DrawPrimitive primitive, Attribute test_attribute,
                                  const float* attribute_value, DrawMode draw_mode);

 private:
  // Buffer containing vertices that set the attribute(s) under test.
  std::shared_ptr<VertexBuffer> bleed_buffer_;
  // Buffer omitting the attributes under test.
  std::shared_ptr<VertexBuffer> test_buffer_;

  std::vector<uint32_t> index_buffer_;
};

#endif  // NXDK_PGRAPH_TESTS_ATTRIBUTE_CARRYOVER_TESTS_H
