#ifndef NXDK_PGRAPH_TESTS_TEXTURE_CUBEMAP_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_CUBEMAP_TESTS_H

#include <string>

#include "test_suite.h"
#include "texture_format.h"
#include "xbox_math_vector.h"

class TestHost;

/**
 * Tests cubemap texture addressing, projective coordinates, and dot product reflection modes.
 */
class TextureCubemapTests : public TestSuite {
 public:
  TextureCubemapTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  enum class ReflectTest {
    kSpecular,
    kDiffuse,
    kSpecularConst,
  };

 private:
  //! Tests basic cubemap sampling with varying projective Q coordinates.
  void TestCubemap(float q_coord);

  //! Tests DOT_STR_3D dot product lookups with specified RGB mapping mode.
  void TestDotSTR3D(const std::string &name, uint32_t dot_rgb_mapping);

  //! Tests DOT_STR_CUBE dot product lookups on cubemap textures.
  void TestDotSTRCubemap(const std::string &name, uint32_t dot_rgb_mapping);

  //! Tests DOT_REFLECT diffuse and specular reflection vector calculations.
  void TestDotReflect(const std::string &name, uint32_t dot_rgb_mapping, ReflectTest mode);

  //! Tests DOT_REFLECT_SPECULAR reflection lookups with varying eye vectors.
  void TestDotReflectSpec(const std::string &name, uint32_t dot_rgb_mapping, const vector_t &eye_vec, bool const_eye);
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_CUBEMAP_TESTS_H
