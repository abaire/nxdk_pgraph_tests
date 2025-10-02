#ifndef NXDK_PGRAPH_TESTS_TEXTURE_2D_AS_CUBEMAP_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_2D_AS_CUBEMAP_TESTS_H

#include <string>

#include "test_suite.h"

/**
 * Tests the behavior of various texture stage modes that require cubemap/volumetric textures but are given textures
 * registered as 2d.
 *
 * NOTE: This suite tests behavior of incorrectly configured textures. Please see TextureCubemapTests for examples of
 * how these modes should actually be used.
 */
class Texture2DAsCubemapTests : public TestSuite {
 public:
  Texture2DAsCubemapTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  enum class ReflectTest {
    kSpecular,
    kDiffuse,
    kSpecularConst,
  };

 private:
  void TestCubemap();
  void TestDotSTR3D(const std::string &name);
  void TestDotSTRCubemap(const std::string &name);
  void TestDotReflect(const std::string &name, ReflectTest mode);
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_2D_AS_CUBEMAP_TESTS_H
