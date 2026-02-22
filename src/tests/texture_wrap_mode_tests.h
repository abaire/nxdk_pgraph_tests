#ifndef NXDK_PGRAPH_TESTS_TEXTURE_WRAP_MODE_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_WRAP_MODE_TESTS_H

#include "test_suite.h"

//! Various tests of NV097_SET_TEXTURE_ADDRESS
class TextureWrapModeTests : public TestSuite {
 public:
  TextureWrapModeTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void TestCylinderWrapping();
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_WRAP_MODE_TESTS_H
