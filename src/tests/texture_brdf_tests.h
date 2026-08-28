#ifndef NXDK_PGRAPH_TESTS_TEXTURE_BRDF_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_BRDF_TESTS_H

#include <string>

#include "test_suite.h"
#include "texture_format.h"

class TestHost;

/**
 * Tests BRDF pixel shader texture modes (NV097_SET_SHADER_STAGE_PROGRAM BRDF).
 */
class TextureBRDFTests : public TestSuite {
 public:
  TextureBRDFTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  //! Tests BRDF shader stage program lookups with combinations of blank/active stages.
  void Test(bool stage0_blank, bool stage1_blank);
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_BRDF_TESTS_H
