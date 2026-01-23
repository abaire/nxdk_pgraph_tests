#ifndef NXDK_PGRAPH_TESTS_DEPTH_FORMAT_FIXED_FUNCTION_TESTS_H
#define NXDK_PGRAPH_TESTS_DEPTH_FORMAT_FIXED_FUNCTION_TESTS_H

#include <cstdint>
#include <memory>
#include <string>

#include "test_suite.h"

class TestHost;

class DepthFormatFixedFunctionTests : public TestSuite {
 public:
  struct DepthFormat {
    uint32_t format{0};
    uint32_t max_depth{0};
    bool floating_point{false};
  };

 public:
  DepthFormatFixedFunctionTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void CreateGeometry();
  void Test(const DepthFormat &format, bool compress_z, uint32_t depth_cutoff);

  static std::string MakeTestName(const DepthFormat &format, bool compress_z, uint32_t depth_cutoff);
};

#endif  // NXDK_PGRAPH_TESTS_DEPTH_FORMAT_FIXED_FUNCTION_TESTS_H
