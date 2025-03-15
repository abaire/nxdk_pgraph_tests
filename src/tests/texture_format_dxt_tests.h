#ifndef NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_DXT_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_DXT_TESTS_H

#include <string>

#include "test_host.h"
#include "test_suite.h"

struct TextureFormatInfo;

/**
 * Tests processing of ST3-compressed textures in various formats.
 */
class TextureFormatDXTTests : public TestSuite {
 public:
  enum class CompressedTextureFormat {
    DXT1 = NV097_SET_TEXTURE_FORMAT_COLOR_L_DXT1_A1R5G5B5,
    DXT3 = NV097_SET_TEXTURE_FORMAT_COLOR_L_DXT23_A8R8G8B8,
    DXT5 = NV097_SET_TEXTURE_FORMAT_COLOR_L_DXT45_A8R8G8B8,
  };

 public:
  TextureFormatDXTTests(TestHost &host, std::string output_dir, const Config &config);
  void Initialize() override;

 private:
  void Test(const char *filename, CompressedTextureFormat texture_format);
  void TestMipmap(const char *filename, CompressedTextureFormat texture_format);
  static std::string MakeTestName(const std::string &filename, CompressedTextureFormat texture_format,
                                  bool mipmap = false);
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_DXT_TESTS_H
