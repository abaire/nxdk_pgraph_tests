#ifndef NXDK_PGRAPH_TESTS_TEXTURE_3D_AS_2D_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_3D_AS_2D_TESTS_H

#include <string>

#include "test_suite.h"

/**
 * Tests the behavior of various texture stage modes that require 2D textures but are given cubemap/volumetric textures.
 *
 * NOTE: This suite tests behavior of incorrectly configured textures. Please see TextureFormatTests for examples of how
 * 2D textures should generally be used.
 */
class Texture3DAs2DTests : public TestSuite {
 public:
  Texture3DAs2DTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void TestCubemap();
  void TestVolumetric();
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_3D_AS_2D_TESTS_H
