#ifndef NXDK_PGRAPH_TESTS_TEXTURE_LOD_BIAS_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_LOD_BIAS_TESTS_H

#include "test_suite.h"

/**
 * Tests the behavior of `NV097_SET_TEXTURE_FILTER`'s `NV097_SET_TEXTURE_FILTER_MIPMAP_LOD_BIAS` component.
 */
class TextureLodBiasTests : public TestSuite {
 public:
  TextureLodBiasTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void Test();
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_LOD_BIAS_TESTS_H
