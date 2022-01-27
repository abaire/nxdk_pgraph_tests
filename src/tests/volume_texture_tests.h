#ifndef NXDK_PGRAPH_TESTS_VOLUME_TEXTURE_TESTS_H
#define NXDK_PGRAPH_TESTS_VOLUME_TEXTURE_TESTS_H

#include <string>

#include "test_suite.h"
#include "texture_format.h"

class TestHost;

class VolumeTextureTests : public TestSuite {
 public:
  struct TestConfig {
    char name[30]{0};
    TextureFormatInfo format;
    bool is_palettized{false};
  };

 public:
  VolumeTextureTests(TestHost &host, std::string output_dir);

  void Initialize() override;

 private:
  void CreateGeometry();
  void Test(const TestConfig &config);
};

#endif  // NXDK_PGRAPH_TESTS_VOLUME_TEXTURE_TESTS_H
