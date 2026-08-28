#ifndef NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_TESTS_H

#include <string>

#include "test_host.h"
#include "test_suite.h"

namespace PBKitPlusPlus {
struct TextureFormatInfo;
}
using namespace PBKitPlusPlus;

/**
 * Tests hardware texture format decoding across linear, swizzled, and palettized formats.
 */
class TextureFormatTests : public TestSuite {
 public:
  TextureFormatTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void CreateGeometry();

  //! Tests texture mapping with the specified texture format.
  void Test(const TextureFormatInfo &texture_format);

  //! Tests palettized texture formats with the specified palette entry size.
  void TestPalettized(TestHost::PaletteSize size);

  static std::string MakeTestName(const TextureFormatInfo &texture_format, bool mipmap = false);
  static std::string MakePalettizedTestName(TestHost::PaletteSize size);
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_TESTS_H
