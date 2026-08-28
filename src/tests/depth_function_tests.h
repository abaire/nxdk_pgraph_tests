#ifndef NXDK_PGRAPH_TESTS_DEPTH_FUNCTION_TESTS_H
#define NXDK_PGRAPH_TESTS_DEPTH_FUNCTION_TESTS_H

#include <cstdint>
#include <memory>
#include <string>

#include "test_suite.h"

class TestHost;

/**
 * Tests depth comparison functions (NV097_SET_DEPTH_FUNC) including standard and invalid values.
 */
class DepthFunctionTests : public TestSuite {
 public:
  DepthFunctionTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  //! Tests behavior of all NV097_SET_DEPTH_FUNC comparison functions and invalid values.
  void Test();
};

#endif  // NXDK_PGRAPH_TESTS_DEPTH_FUNCTION_TESTS_H
