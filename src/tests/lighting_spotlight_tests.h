#ifndef NXDK_PGRAPH_TESTS_LIGHTING_SPOTLIGHT_TESTS_H
#define NXDK_PGRAPH_TESTS_LIGHTING_SPOTLIGHT_TESTS_H

#include <cstdint>
#include <memory>
#include <vector>

#include "test_suite.h"
#include "xbox_math_types.h"

class TestHost;
class VertexBuffer;

// Tests spotlights.
class LightingSpotlightTests : public TestSuite {
 public:
  //! Describes a spotlight.
  //! See https://learn.microsoft.com/en-us/windows/uwp/graphics-concepts/attenuation-and-spotlight-factor
  typedef struct Spotlight {
    void Commit(uint32_t light_index, const XboxMath::matrix4_t& view_matrix) const;

    XboxMath::vector_t position;

    XboxMath::vector_t direction;

    //! penumbra (outer cone) angle in radians.
    float phi;

    //! umbra (inner cone) angle in radians.
    float theta;

    //! Raw nv2a falloff coefficiens.
    float falloff[3];
  } Spotlight;

 public:
  LightingSpotlightTests(TestHost& host, std::string output_dir);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void Test(const std::string& name, const Spotlight& light);
  void TestFixed(const std::string& name, const Spotlight& light);

  void CreateGeometry();

 private:
  std::shared_ptr<VertexBuffer> vertex_buffer_;
};

#endif  // NXDK_PGRAPH_TESTS_LIGHTING_SPOTLIGHT_TESTS_H
