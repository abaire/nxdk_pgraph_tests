#ifndef NXDK_PGRAPH_TESTS_TEXTURE_CUBEMAP_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_CUBEMAP_TESTS_H

#include <string>

#include "test_suite.h"
#include "texture_format.h"
#include "xbox_math_vector.h"

using namespace XboxMath;

class TestHost;

// Tests cubemap texture behavior.
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
  void TestCubemap(float q_coord);
  void TestDotSTR3D(const std::string &name, uint32_t dot_rgb_mapping);
  void TestDotSTRCubemap(const std::string &name, uint32_t dot_rgb_mapping);
  void TestDotReflect(const std::string &name, uint32_t dot_rgb_mapping, ReflectTest mode);
  void TestDotReflectSpec(const std::string &name, uint32_t dot_rgb_mapping, const vector_t &eye_vec, bool const_eye);
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_CUBEMAP_TESTS_H
