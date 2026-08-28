#ifndef NXDK_PGRAPH_TESTS_VOLUME_TEXTURE_TESTS_H
#define NXDK_PGRAPH_TESTS_VOLUME_TEXTURE_TESTS_H

#include <string>

#include "test_suite.h"
#include "texture_format.h"

class TestHost;

namespace PBKitPlusPlus {
struct TextureFormatInfo;
}

using namespace PBKitPlusPlus;

/**
 * Tests 3D volumetric texture formats, filtering, and slice interpolation.
 */
class VolumeTextureTests : public TestSuite {
 public:
  VolumeTextureTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void CreateGeometry();

  //! Tests sampling a 3D volume texture of the specified format across depth slices.
  void Test(const TextureFormatInfo &texture_format);

  //! Tests 3D volume texture sampling with palettized color formats.
  void TestPalettized();
};

#endif  // NXDK_PGRAPH_TESTS_VOLUME_TEXTURE_TESTS_H
