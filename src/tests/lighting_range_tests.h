#ifndef NXDK_PGRAPH_TESTS_LIGHTING_RANGE_TESTS_H
#define NXDK_PGRAPH_TESTS_LIGHTING_RANGE_TESTS_H

#include <memory>
#include <vector>

#include "test_suite.h"

class Light;
class TestHost;
class VertexBuffer;

/**
 * Tests the effects of NV097_SET_LIGHT_LOCAL_RANGE (0x1024).
 *
 * Each test renders a grid mesh in the center, angled such that the left side is closer to the camera than the right.
 * Two quads are also rendered to the left and the right of the main mesh, the one on the left at a depth close to the
 * camera and the one on the right farther away.
 *
 * A light is then projected onto the geometry with a range cutoff set such that a portion of the vertices should not be
 * lit.
 */
class LightingRangeTests : public TestSuite {
 public:
  LightingRangeTests(TestHost& host, std::string output_dir, const Config& config);

  void Deinitialize() override;
  void Initialize() override;

 private:
  //! Tests the behavior of NV097_SET_LIGHT_LOCAL_RANGE.
  void Test(const std::string& name, std::shared_ptr<Light> light);

  void CreateGeometry();

 private:
  std::shared_ptr<VertexBuffer> vertex_buffer_mesh_;
};

#endif  // NXDK_PGRAPH_TESTS_LIGHTING_RANGE_TESTS_H
