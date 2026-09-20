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
 * Tests sampling of NV2A texture formats, verifying color conversion, swizzling, and alpha channel handling.
 */
class TextureFormatTests : public TestSuite {
 public:
  TextureFormatTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void CreateGeometry();

  //! Tests sampling of the given texture format using a generated RGBA color gradient.
  void Test(const TextureFormatInfo &texture_format);

  //! Tests sampling of 8-bit palettized textures (SZ_I8_A8R8G8B8) with the given palette size.
  void TestPalettized(TestHost::PaletteSize size);

  //! Tests alpha channel sampling and blending behavior for texture formats with dummy/unused "X" alpha components.
  void TestXAlpha(const TextureFormatInfo &texture_format);

  static std::string MakeTestName(const TextureFormatInfo &texture_format, bool mipmap = false);
  static std::string MakePalettizedTestName(TestHost::PaletteSize size);
  static std::string MakeXAlphaTestName(const TextureFormatInfo &texture_format);
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_TESTS_H
