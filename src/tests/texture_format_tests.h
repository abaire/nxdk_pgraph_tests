#ifndef NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_TESTS_H

#include <string>

#include "test_host.h"
#include "test_suite.h"

namespace PBKitPlusPlus {
struct TextureFormatInfo;
}
using namespace PBKitPlusPlus;

class TextureFormatTests : public TestSuite {
 public:
  TextureFormatTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void CreateGeometry();

  void Test(const TextureFormatInfo &texture_format);
  //  void TestMipMap(const TextureFormatInfo &texture_format);
  void TestPalettized(TestHost::PaletteSize size);

  static std::string MakeTestName(const TextureFormatInfo &texture_format, bool mipmap = false);
  static std::string MakePalettizedTestName(TestHost::PaletteSize size);
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_TESTS_H
