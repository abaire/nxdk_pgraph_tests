#ifndef NXDK_PGRAPH_TESTS_TEXTURE_SIGNED_COMPONENT_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_SIGNED_COMPONENT_TESTS_H

#include <string>

#include "test_host.h"
#include "test_suite.h"

struct TextureFormatInfo;

class TextureSignedComponentTests : public TestSuite {
 public:
  TextureSignedComponentTests(TestHost &host, std::string output_dir);

  void Initialize() override;

 private:
  void CreateGeometry();

  void Test(const TextureFormatInfo &texture_format, uint32_t signed_flags, const std::string &test_name);
  void TestGradients(const TextureFormatInfo &texture_format);

  static std::string MakeTestName(const TextureFormatInfo &texture_format, uint32_t signed_flags);
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_SIGNED_COMPONENT_TESTS_H
