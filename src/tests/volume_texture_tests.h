#ifndef NXDK_PGRAPH_TESTS_VOLUME_TEXTURE_TESTS_H
#define NXDK_PGRAPH_TESTS_VOLUME_TEXTURE_TESTS_H

#include <string>

#include "test_suite.h"
#include "texture_format.h"

class TestHost;

// Tests 3d texture behavior.
class VolumeTextureTests : public TestSuite {
 public:
  VolumeTextureTests(TestHost &host, std::string output_dir);

  void Initialize() override;

 private:
  void CreateGeometry();

  void Test(const TextureFormatInfo &texture_format);
  void TestPalettized();
};

#endif  // NXDK_PGRAPH_TESTS_VOLUME_TEXTURE_TESTS_H
