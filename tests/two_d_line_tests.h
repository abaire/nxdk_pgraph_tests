#ifndef NXDK_PGRAPH_TESTS_2D_LINE_TESTS_H
#define NXDK_PGRAPH_TESTS_2D_LINE_TESTS_H

#include <pbkit/pbkit.h>

#include "test_suite.h"

class TestHost;

class TwoDLineTests : public TestSuite {
 public:
  struct TestCase {
    uint32_t object_color;
    uint32_t color_format;
    uint32_t start_x;
    uint32_t start_y;
    uint32_t end_x;
    uint32_t end_y;
  };

 public:
  TwoDLineTests(TestHost& host, std::string output_dir);

  void Initialize() override;

 private:
  void Test(const TestCase& test);

  static std::string MakeTestName(const TestCase& test, bool ReturnShortName);

  struct s_CtxDma solid_lin_ctx_ {};
  struct s_CtxDma surface_destination_ctx_ {};
};

#endif  // NXDK_PGRAPH_TESTS_2D_LINE_TESTS_H
