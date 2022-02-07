#ifndef NXDK_PGRAPH_TESTS_STENCIL_TESTS_H
#define NXDK_PGRAPH_TESTS_STENCIL_TESTS_H

#include <cstdint>
#include <memory>
#include <string>

#include "test_suite.h"

class TestHost;

class StencilTests : public TestSuite {
 public:
  struct StencilParams {
    uint32_t stencil_op_zpass;
    const char *stencil_op_zpass_str;
    bool stencil_test_enabled;
    bool depth_test_enabled;
    uint32_t stencil_clear_value;
    uint32_t stencil_ref_value;
  };

 public:
  StencilTests(TestHost &host, std::string output_dir);

  void Initialize() override;

 private:
  void CreateGeometry(const float sideLength, const float r, const float g, const float b);
  void Test(const StencilParams &params);

  void AddTestEntry(const StencilParams &params);
  static std::string MakeTestName(const StencilParams &params);
};

#endif  // NXDK_PGRAPH_TESTS_STENCIL_TESTS_H
