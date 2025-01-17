#ifndef NXDK_PGRAPH_TESTS_CLEAR_TESTS_H
#define NXDK_PGRAPH_TESTS_CLEAR_TESTS_H

#include "test_suite.h"

class TestHost;

class ClearTests : public TestSuite {
 public:
  ClearTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;

 private:
  void CreateGeometry();
  void Test(uint32_t color_mask, bool depth_write_enable);

  static std::string MakeTestName(uint32_t color_mask, bool depth_write_enable);
};
#endif  // NXDK_PGRAPH_TESTS_CLEAR_TESTS_H
