#ifndef NXDK_PGRAPH_TESTS_ATTRIBUTE_NAN_TESTS_H
#define NXDK_PGRAPH_TESTS_ATTRIBUTE_NAN_TESTS_H

#include "test_host.h"
#include "test_suite.h"

struct TestConfig;

//! Tests behavior of various float inputs to vertex shaders
//! including NaN, INF, etc.
//!
//! The test renders a number of quads. The leftmost quad passes through the raw
//! value as `diffuse.rgb` and subsequent columns multiply each component by the
//! value listed in the column header.
//!
//! The values vary from the top to the bottom of the quad with the top and
//! bottom values displayed above the "Multiplier" header.
class AttributeFloatTests : public TestSuite {
 public:
  AttributeFloatTests(TestHost &host, std::string output_dir, const Config &config);

 private:
  void Test(const TestConfig &testConfig);
};

#endif  // NXDK_PGRAPH_TESTS_ATTRIBUTE_NAN_TESTS_H
