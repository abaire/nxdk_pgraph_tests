#ifndef NXDK_PGRAPH_TESTS_COLOR_MASK_BLEND_TESTS_H
#define NXDK_PGRAPH_TESTS_COLOR_MASK_BLEND_TESTS_H

#include "test_suite.h"

class TestHost;

class ColorMaskBlendTests : public TestSuite {
 public:
  ColorMaskBlendTests(TestHost &host, std::string output_dir);

  void Initialize() override;

 private:
  void CreateGeometry();
  void Test(uint32_t color_mask, uint32_t blend_op, uint32_t sfactor, uint32_t dfactor, const std::string &test_name);
};
#endif  // NXDK_PGRAPH_TESTS_COLOR_MASK_BLEND_TESTS_H
