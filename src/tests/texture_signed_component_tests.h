#ifndef NXDK_PGRAPH_TESTS_TEXTURE_SIGNED_COMPONENT_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_SIGNED_COMPONENT_TESTS_H

#include <string>

#include "test_host.h"
#include "test_suite.h"

namespace PBKitPlusPlus {
struct TextureFormatInfo;
}

using namespace PBKitPlusPlus;

/**
 * Tests texture formats with signed components and sign-extension/bias decoding modes.
 */
class TextureSignedComponentTests : public TestSuite {
 public:
  TextureSignedComponentTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void CreateGeometry();

  //! Tests texture sampling with specified signed component flags for the given texture format.
  void Test(const TextureFormatInfo &texture_format, uint32_t signed_flags, const std::string &test_name);

  //! Tests gradient rendering and blending with signed texture formats.
  void TestGradients(const std::string &name, const TextureFormatInfo &texture_format, uint32_t blend_op);

  static std::string MakeTestName(const TextureFormatInfo &texture_format, uint32_t signed_flags);
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_SIGNED_COMPONENT_TESTS_H
