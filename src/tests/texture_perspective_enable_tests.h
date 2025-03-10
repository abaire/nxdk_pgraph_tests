#ifndef NXDK_PGRAPH_TESTS_TEXTURE_PERSPECTIVE_ENABLE_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_PERSPECTIVE_ENABLE_TESTS_H

#include "test_suite.h"

class TestHost;

/**
 * Tests handling of bit 20 in NV097_SET_CONTROL0.
 */
class TexturePerspectiveEnableTests : public TestSuite {
 public:
  TexturePerspectiveEnableTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;

 private:
  void Test(const std::string& name, bool texture_enabled);
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_PERSPECTIVE_ENABLE_TESTS_H
