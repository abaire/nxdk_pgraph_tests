#ifndef NXDK_PGRAPH_TESTS_SURFACE_AS_VERTEX_ARRAY_TESTS_H
#define NXDK_PGRAPH_TESTS_SURFACE_AS_VERTEX_ARRAY_TESTS_H

#include <pbkit/pbkit.h>

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

  //! Draws two quads using surface_a_ as the diffuse stream, starting at the given vertex index.
  void DrawSurfaceDiffuseQuads(uint32_t start_index);

  //! Draws the top quads from a CPU-written red diffuse stream, then renders green over the same memory.
  void RenderOverConsumedVertexArray();

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

  //! Renders into memory previously consumed as a vertex stream, reads it back with the CPU, then draws from it again.
  void TestDownloadByCPURead();

  //! Renders into memory previously consumed as a vertex stream, samples it as an incompatible texture, then draws from
  //! it again.
  void TestDownloadByTextureRead();

  //! Image blits over memory previously consumed as a vertex stream, then draws from it again.
  void TestImageBlitOverVertexArray();

 private:
  uint8_t *surface_a_{nullptr};
  uint8_t *surface_b_{nullptr};
  std::shared_ptr<VertexBuffer> vertex_buffer_;

  struct s_CtxDma blit_dma_ctx_{};
  struct s_CtxDma null_ctx_{};
};

#endif  // NXDK_PGRAPH_TESTS_SURFACE_AS_VERTEX_ARRAY_TESTS_H
