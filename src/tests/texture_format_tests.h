#ifndef NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_TESTS_H

#include <string>

#include "test_suite.h"

class TestHost;
struct TextureFormatInfo;

class TextureFormatTests : public TestSuite {
 public:
  TextureFormatTests(TestHost &host, std::string output_dir);

  void Initialize() override;

 private:
  void CreateGeometry();
  void Test(const TextureFormatInfo &texture_format);
  static std::string MakeTestName(const TextureFormatInfo &texture_format);
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_TESTS_H
