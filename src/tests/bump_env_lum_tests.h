#ifndef NXDK_PGRAPH_TESTS_BUMP_ENV_LUM_TESTS_H
#define NXDK_PGRAPH_TESTS_BUMP_ENV_LUM_TESTS_H

#include <string>

#include "test_host.h"
#include "test_suite.h"

namespace PBKitPlusPlus {
struct TextureFormatInfo;
}

/**
 * Tests bump environment mapping with luminance calculation (NV097_SET_SHADER_STAGE_PROGRAM BUMPENVMAP_LUMINANCE).
 */
class BumpEnvLumTests : public TestSuite {
 public:
  BumpEnvLumTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  //! Tests bump environment mapping with luminance across signed and unsigned component configurations for the given
  //! texture format.
  void Test(const TextureFormatInfo &texture_format, bool cross_on_blue, bool rotate90);
  static std::string MakeTestName(const TextureFormatInfo &texture_format, bool cross_on_blue, bool rotate90);
};

#endif  // NXDK_PGRAPH_TESTS_BUMP_ENV_LUM_TESTS_H
