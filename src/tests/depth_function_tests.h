#ifndef NXDK_PGRAPH_TESTS_DEPTH_FUNCTION_TESTS_H
#define NXDK_PGRAPH_TESTS_DEPTH_FUNCTION_TESTS_H

#include <cstdint>
#include <memory>
#include <string>

#include "test_suite.h"

class TestHost;

class DepthFunctionTests : public TestSuite {
 public:
  DepthFunctionTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void Test();
};

#endif  // NXDK_PGRAPH_TESTS_DEPTH_FUNCTION_TESTS_H
