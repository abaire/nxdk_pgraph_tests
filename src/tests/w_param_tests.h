#ifndef NXDK_PGRAPH_TESTS_INF_TESTS_H
#define NXDK_PGRAPH_TESTS_INF_TESTS_H

#include "test_suite.h"

class TestHost;

class WParamTests : public TestSuite {
 public:
  WParamTests(TestHost &host, std::string output_dir);

  void Initialize() override;

 private:
  void CreateGeometry();
  void Test();
};

#endif  // NXDK_PGRAPH_TESTS_INF_TESTS_H
