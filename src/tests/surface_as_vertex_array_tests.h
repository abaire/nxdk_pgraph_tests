#ifndef NXDK_PGRAPH_TESTS_SURFACE_AS_VERTEX_ARRAY_TESTS_H
#define NXDK_PGRAPH_TESTS_SURFACE_AS_VERTEX_ARRAY_TESTS_H

#include <memory>
#include <string>

#include "test_suite.h"

namespace PBKitPlusPlus {
class VertexBuffer;
}

class TestHost;

/**
 * Tests rendering to GPU surfaces (render targets) and subsequently binding the
 * rendered surface memory as a vertex attribute array (vertex stream).
 *
 * This pattern takes advantage of Xbox UMA (Unified Memory Architecture) to
 * perform GPU-driven vertex generation or displacement mapping without hardware
 * vertex texture fetch.
 */
class SurfaceAsVertexArrayTests : public TestSuite {
 public:
  SurfaceAsVertexArrayTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;
  void Deinitialize() override;
  void TearDownTest() override;

 private:
  void AllocateTestSurfaces(bool need_surface_b = false);
  void FreeTestSurfaces();

  void DrawQuads(const void *diffuse_surface);

  //! Renders color strips into a linear render target, then binds the surface as a diffuse vertex array to draw 4
  //! quads.
  void TestLinearDiffuseArray();

  //! Renders color blocks into a swizzled render target, then binds the surface as a diffuse vertex array to draw 4
  //! quads.
  void TestSwizzledDiffuseArray();

  //! Renders separate colors to two surfaces, binding one as diffuse and the other as specular across multiple streams.
  void TestMultiStream();

  //! Alternates rendering into a surface and drawing individual quads within a single frame update loop.
  void TestDynamicUpdateLoop();

  //! Renders carrier triangle vertex indices and barycentric weights into swizzled surfaces A and B, then binds them
  //! as Diffuse and Specular vertex streams with a vertex shader performing palette lookups to reconstruct a 3D mesh.
  void TestRenderScalePattern();

 private:
  uint8_t *surface_a_{nullptr};
  uint8_t *surface_b_{nullptr};
  std::shared_ptr<VertexBuffer> vertex_buffer_;
};

#endif  // NXDK_PGRAPH_TESTS_SURFACE_AS_VERTEX_ARRAY_TESTS_H
