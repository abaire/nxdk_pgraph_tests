#ifndef NXDK_PGRAPH_TESTS_ATTRIBUTE_CLAMPING_TESTS_H
#define NXDK_PGRAPH_TESTS_ATTRIBUTE_CLAMPING_TESTS_H

#include "test_host.h"
#include "test_suite.h"

struct TestConfig;

// Tests behavior of returning negative values from the vertex shader
// This is intended to verify whether clamping should be performed on
// values.
class AttributeClampingTests : public TestSuite {
 public:
  enum Attribute {
    ATTR_DIFFUSE = 2,
    ATTR_SPECULAR = 3,
    ATTR_FOG_COORD = 4,
  };

 public:
  AttributeClampingTests(TestHost &host, std::string output_dir);
  void Initialize() override;

 private:
  void Test(Attribute attribute, const float values[4], const std::string &test_name);
};

#endif  // NXDK_PGRAPH_TESTS_ATTRIBUTE_CLAMPING_TESTS_H
