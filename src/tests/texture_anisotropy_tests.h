#ifndef NXDK_PGRAPH_TESTS_TEXTURE_ANISOTROPY_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_ANISOTROPY_TESTS_H

#include "test_suite.h"

/**
 * Tests behavior of the anisotropic filtering components of
 * NV097_SET_TEXTURE_CONTROL0.
 */
class TextureAnisotropyTests : public TestSuite {
 public:
  TextureAnisotropyTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void Test(uint32_t anisotropy_shift);
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_ANISOTROPY_TESTS_H
