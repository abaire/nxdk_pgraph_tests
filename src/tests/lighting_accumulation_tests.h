#ifndef NXDK_PGRAPH_TESTS_LIGHTING_ACCUMULATION_TESTS_H
#define NXDK_PGRAPH_TESTS_LIGHTING_ACCUMULATION_TESTS_H

#include <memory>
#include <vector>

#include "test_suite.h"

class Light;
class TestHost;
class VertexBuffer;

/**
 * Tests the effects of multiple lights.
 *
 * Each test renders a grid mesh in the center and renders multiple lights with low color multipliers in order to
 * evaluate how they are accumulated into final pixel colors.
 */
class LightingAccumulationTests : public TestSuite {
 public:
  LightingAccumulationTests(TestHost& host, std::string output_dir, const Config& config);

  void Deinitialize() override;
  void Initialize() override;

 private:
  //! Tests the behavior of multiple lights on a mesh.
  void Test(const std::string& name, std::vector<std::shared_ptr<Light>> light);

  void CreateGeometry();

 private:
  std::shared_ptr<VertexBuffer> vertex_buffer_mesh_;
};

#endif  // NXDK_PGRAPH_TESTS_LIGHTING_ACCUMULATION_TESTS_H
