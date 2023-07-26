#ifndef NXDK_PGRAPH_TESTS_LIGHTING_SPOTLIGHT_TESTS_H
#define NXDK_PGRAPH_TESTS_LIGHTING_SPOTLIGHT_TESTS_H

#include <cstdint>
#include <memory>
#include <vector>

#include "test_suite.h"
#include "xbox_math_types.h"

class TestHost;
class VertexBuffer;

/**
 * Tests the behavior of spotlight falloff and attenuation settings.
 *
 * The spotlight is placed at {0, 0, -7} (the camera position) pointing directly
 * into the screen, with a maximum range of 15.f. A plane is rendered at
 * {0, 0, 0}, and a cylinder at ~ {1, 1, 0}.
 *
 * For the "At" prefixed tests, the attenuation factor of the light is adjusted.
 * This factor should decrease the light intensity relative to the distance
 * between the vertex and the light.
 *
 * For the "Fo" prefixed tests, the falloff factor of the light is adjusted.
 * This factor modifies how the intensity falls off in the zone between the
 * penumbra (the outermost illuminated region) and the umbra (the region within
 * which the light's full intensity is applied). Note that all pixels are still
 * modified by attenuation, which is set to a constant value.
 *
 * For the "PT" prefixed tests, the phi (penumbra) and theta (umbra) angles are
 * set to various values, including some that would be prevented by DirectX.
 *
 * See https://learn.microsoft.com/en-us/windows/uwp/graphics-concepts/attenuation-and-spotlight-factor
 */
class LightingSpotlightTests : public TestSuite {
 public:
  //! Describes a spotlight.
  //! See https://learn.microsoft.com/en-us/windows/uwp/graphics-concepts/attenuation-and-spotlight-factor
  typedef struct Spotlight {
    Spotlight(const XboxMath::vector_t& position, const XboxMath::vector_t& direction, float phi, float theta,
              float attenuation_1, float attenuation_2, float attenuation_3, float falloff_1, float falloff_2,
              float falloff_3);
    void Commit(uint32_t light_index, const XboxMath::matrix4_t& view_matrix) const;

    XboxMath::vector_t position;

    XboxMath::vector_t direction;

    //! penumbra (outer cone) angle in degrees.
    float phi;

    //! umbra (inner cone) angle in degrees.
    float theta;

    //! Attenuation: {constant, linear (distance), quadratic (distance^2)}
    float attenuation[3];

    //! Raw nv2a falloff coefficients.
    float falloff[3];
  } Spotlight;

 public:
  LightingSpotlightTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void Test(const std::string& name, const Spotlight& light);
  void TestFixed(const std::string& name, const Spotlight& light);

  void CreateGeometry();

 private:
  std::shared_ptr<VertexBuffer> vertex_buffer_plane_;
  std::shared_ptr<VertexBuffer> vertex_buffer_cylinder_;
};

#endif  // NXDK_PGRAPH_TESTS_LIGHTING_SPOTLIGHT_TESTS_H
