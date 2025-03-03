#ifndef NXDK_PGRAPH_TESTS_ATTRIBUTE_CARRYOVER_TESTS_H
#define NXDK_PGRAPH_TESTS_ATTRIBUTE_CARRYOVER_TESTS_H

#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"

class VertexBuffer;

/**
 * Tests behavior when vertex attributes are not provided but are used by
 * shaders.
 *
 * For various types of geometry, a fully specified primitive is rendered,
 * followed by an identical (but position-transformed) primitive where only the
 * vertex positions are provided and all other attributes bleed over from the
 * fully specified primitive.
 *
 * Each test picks a separate vertex attribute (e.g., weight, normal, ...) and
 * sets it to a test value. The shader code will convert the attribute under
 * test into a color.
 *
 * Attributes with three parameters (iNormal) will have their alpha forced to 1.f.
 *
 * Attributes with one parameter (e.g., iWeight) will be repeated into the red
 * and green channels with their blue and alpha forced to 1.f.
 */
class AttributeCarryoverTests : public TestSuite {
 public:
  enum DrawMode {
    DRAW_ARRAYS,
    DRAW_INLINE_BUFFERS,
    DRAW_INLINE_ARRAYS,
    DRAW_INLINE_ELEMENTS,
  };

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

  struct TestConfig {
    DrawMode draw_mode;
    float attribute_value[4];
  };

 public:
  AttributeCarryoverTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void CreateGeometry(TestHost::DrawPrimitive primitive);
  void Test(TestHost::DrawPrimitive primitive, Attribute test_attribute, const TestConfig &config);

  static std::string MakeTestName(TestHost::DrawPrimitive primitive, Attribute test_attribute,
                                  const TestConfig &config);

 private:
  // Buffer containing vertices that set the attribute(s) under test.
  std::shared_ptr<VertexBuffer> bleed_buffer_;
  // Buffer omitting the attributes under test.
  std::shared_ptr<VertexBuffer> test_buffer_;

  std::vector<uint32_t> index_buffer_;
};

#endif  // NXDK_PGRAPH_TESTS_ATTRIBUTE_CARRYOVER_TESTS_H
