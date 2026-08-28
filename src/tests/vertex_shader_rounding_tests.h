#ifndef NXDK_PGRAPH_TESTS_VERTEX_SHADER_ROUNDING_TESTS_H
#define NXDK_PGRAPH_TESTS_VERTEX_SHADER_ROUNDING_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

namespace PBKitPlusPlus {
class VertexBuffer;
}

using namespace PBKitPlusPlus;

/**
 * Tests vertex position rounding, sub-pixel snapping, and rasterization boundary alignment.
 */
class VertexShaderRoundingTests : public TestSuite {
 public:
  VertexShaderRoundingTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void CreateGeometry();

  //! Tests coordinate rounding when rendering to an offscreen render target.
  void TestRenderTarget();

  //! Tests geometry vertex coordinate rounding with sub-pixel bias offsets.
  void TestGeometry(float bias);

  //! Tests subscreen quad geometry rounding with sub-pixel offsets.
  void TestGeometrySubscreen(float bias);

  //! Tests geometry vertices extending beyond the viewport boundary.
  void TestGeometrySuperscreen(float draw_width);

  //! Tests compositing multiple render target layers at specific depth planes.
  void TestCompositingRenderTarget(int z);

  //! Tests seam rasterization and rounding between adjacent primitives with coordinate bias.
  void TestAdjacentGeometry(float bias);

  //! Tests projected coordinate interpolation and rounding along adjacent polygon edges.
  void TestProjectedAdjacentGeometry(float bias);

  //! Tests top-left rasterization rule compliance in fixed-function vs programmable modes.
  void TestTopLeftRasterization(bool fixed);

 private:
  uint8_t *render_target_{nullptr};

  std::shared_ptr<VertexBuffer> framebuffer_vertex_buffer_;
};

#endif  // NXDK_PGRAPH_TESTS_VERTEX_SHADER_ROUNDING_TESTS_H
