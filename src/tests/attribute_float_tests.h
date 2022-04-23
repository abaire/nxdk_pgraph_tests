#ifndef NXDK_PGRAPH_TESTS_ATTRIBUTE_NAN_TESTS_H
#define NXDK_PGRAPH_TESTS_ATTRIBUTE_NAN_TESTS_H

#include "test_host.h"
#include "test_suite.h"

struct TestConfig;

// Tests behavior of various float inputs to vertex shaders
// including NaN, INF, etc.
// This was created due to a mismatch between Xbox and PC hardware
// when encountering NaN values
class AttributeFloatTests : public TestSuite {
 public:
  AttributeFloatTests(TestHost &host, std::string output_dir);

 private:
  void Test(const TestConfig &testConfig);
};

#endif  // NXDK_PGRAPH_TESTS_ATTRIBUTE_NAN_TESTS_H
