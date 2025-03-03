#ifndef NXDK_PGRAPH_TESTS_LIGHTING_CONTROL_TESTS_H
#define NXDK_PGRAPH_TESTS_LIGHTING_CONTROL_TESTS_H

#include <memory>
#include <vector>

#include "test_suite.h"

class TestHost;
class VertexBuffer;

/**
 * Tests the effects of NV097_SET_LIGHT_CONTROL (0x294) in conjunction with
 * NV097_SET_SPECULAR_ENABLE.
 *
 * A number of test meshes are rendered and lit with two lights:
 *
 * 1) A directional light coming from the left and pointing into the screen
 *    (direction is {1, 0, 1}).
 *
 * 2) An attenuated point light on the right at {1.5f, 1.f, -2.5f} with a max
 *    rannge of 4. and attenuation of {0.025f, 0.15f, 0.02f}.
 *
 *  Both lights have ambient {0.f, 0.f, 0.05f}, diffuse {0.25f, 0.f, 0.f},
 *  specular {0.f, 0.2f, 0.4f}.
 *
 *  Meshes have the specular color of each vertex at {0.f, 0.4, 0.f, 0.25f}.
 *
 *  All colors are taken from the material (i.e., contributed by the lights).
 *  NV097_SET_MATERIAL_ALPHA is set to 0.4 and the scene ambient is a very dark
 *  grey at 0.031373.
 *
 *  See also: the OpenGL 2.1 specification lighting model
 *    https://registry.khronos.org/OpenGL/specs/gl/glspec21.pdf
 *
 * Notes on use with the programmable pipeline:
 *
 * While it is possible to enable lights when using a vertex shader, it appears
 * that parts of the pipeline are not fully initialized, leading to
 * non-deterministic color effects that change with each draw and often differ
 * across cold boots of the hardware.
 *
 * Leaving lighting enabled without enabling any lights via
 * NV097_SET_LIGHT_ENABLE_MASK will cause all vertices to be black in color, but
 * alpha will still come from the material when allowed by the light control and
 * specular enable settings.
 *
 * Disabling specular with lighting enabled will still disable the alpha
 * application.
 *
 * Enabling specular with lighting disabled will apply the material alpha and
 * use the vertex shader output colors.
 */
class LightingControlTests : public TestSuite {
 public:
  LightingControlTests(TestHost& host, std::string output_dir, const Config& config);

  void Deinitialize() override;
  void Initialize() override;

 private:
  //! Tests the behavior of SET_LIGHT_CONTROL.
  void Test(const std::string& name, uint32_t light_control, bool specular_enabled, bool is_fixed_function);

  void CreateGeometry();

 private:
  std::shared_ptr<VertexBuffer> vertex_buffer_cone_;
  std::shared_ptr<VertexBuffer> vertex_buffer_cylinder_;
  std::shared_ptr<VertexBuffer> vertex_buffer_sphere_;
  std::shared_ptr<VertexBuffer> vertex_buffer_suzanne_;
  std::shared_ptr<VertexBuffer> vertex_buffer_torus_;
};

#endif  // NXDK_PGRAPH_TESTS_LIGHTING_CONTROL_TESTS_H
