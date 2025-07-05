#ifndef NXDK_PGRAPH_TESTS_ATTRIBUTE_EXPLICIT_SETTER_TESTS_H
#define NXDK_PGRAPH_TESTS_ATTRIBUTE_EXPLICIT_SETTER_TESTS_H

#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"

// Tests behavior when vertex attributes are not provided but are used by shaders.
class AttributeExplicitSetterTests : public TestSuite {
 public:
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

  struct TestConfig {
    const char* test_name;
    bool force_blend_alpha;
  };

 public:
  AttributeExplicitSetterTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;

 private:
  void CreateGeometry();
  void Test(const TestConfig& config);
  void Draw(float x, float y, const std::function<void(int)>& attribute_setter, Attribute test_attribute,
            int mask = 0xF, float bias = 0.0f, float multiplier = 1.0f);

 private:
  float test_triangle_width{0.0f};
  float test_triangle_height{0.0f};
};

#endif  // NXDK_PGRAPH_TESTS_ATTRIBUTE_EXPLICIT_SETTER_TESTS_H
