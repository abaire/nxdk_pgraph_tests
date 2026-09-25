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
 * @brief Tests sampling of NV2A texture formats, verifying color conversion, swizzling, and alpha channel handling.
 *
 * Exercises rendering a textured quad using various NV2A hardware texture formats. A color gradient is
 * generated and converted to the target format (applying swizzling if swizzled), then sampled via
 * the projective 2D shader stage with the final combiner set to output the sampled texture color.
 * Also tests 8-bit palettized textures across various palette sizes and verifies alpha channel behavior
 * for formats containing dummy "X" alpha components.
 */
class TextureFormatTests : public TestSuite {
 public:
  TextureFormatTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void CreateGeometry();

  //! Tests sampling of the given texture format using a generated RGBA color gradient.
  void Test(const TextureFormatInfo &texture_format);

  //! Tests sampling of 8-bit palettized textures (SZ_I8_A8R8G8B8) with the given palette size.
  void TestPalettized(TestHost::PaletteSize size);

  //! Tests alpha channel sampling and blending behavior for texture formats with dummy/unused "X" alpha components.
  void TestXAlpha(const TextureFormatInfo &texture_format);

  static std::string MakeTestName(const TextureFormatInfo &texture_format);
  static std::string MakePalettizedTestName(TestHost::PaletteSize size);
  static std::string MakeXAlphaTestName(const TextureFormatInfo &texture_format);
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_TESTS_H
