#ifndef NXDK_PGRAPH_TESTS_CLEAR_TESTS_H
#define NXDK_PGRAPH_TESTS_CLEAR_TESTS_H

#include "test_suite.h"

class TestHost;

/**
 * Tests the behavior of various write masks on NV097_CLEAR_SURFACE.
 *
 * Each test draws some geometry, then modifies the NV097_SET_COLOR_MASK,
 * NV097_SET_DEPTH_MASK, and NV097_SET_STENCIL_MASK settings, then invokes
 * NV097_CLEAR_SURFACE.
 *
 * In all cases, the clear color is set to 0x7F7F7F7F and the depth value is
 * set to 0xFFFFFFFF.
 *
 */
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
