#ifndef NXDK_PGRAPH_TESTS_TEXTURE_CPU_UPDATE_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_CPU_UPDATE_TESTS_H

#include <string>

#include "test_host.h"
#include "test_suite.h"

struct TextureFormatInfo;

class TextureCPUUpdateTests : public TestSuite {
 public:
  TextureCPUUpdateTests(TestHost &host, std::string output_dir);

  void Initialize() override;

 private:
  void CreateGeometry();

  void TestRGBA();
  void TestPalettized();
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_CPU_UPDATE_TESTS_H
