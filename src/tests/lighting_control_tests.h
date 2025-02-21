#ifndef NXDK_PGRAPH_TESTS_LIGHTING_CONTROL_TESTS_H
#define NXDK_PGRAPH_TESTS_LIGHTING_CONTROL_TESTS_H

#include <memory>
#include <vector>

#include "test_suite.h"

class TestHost;
class VertexBuffer;

/**
 * Tests the effects of NV097_SET_LIGHT_CONTROL (0x294).
 */
class LightingControlTests : public TestSuite {
 public:
  LightingControlTests(TestHost& host, std::string output_dir, const Config& config);

  void Deinitialize() override;
  void Initialize() override;

 private:
  //! Tests the behavior of LIGHT_CONTROL using the fixed function pipeline.
  void TestFixed(const std::string& name, uint32_t light_control);

  void CreateGeometry();

 private:
  std::shared_ptr<VertexBuffer> vertex_buffer_cone_;
  std::shared_ptr<VertexBuffer> vertex_buffer_cylinder_;
  std::shared_ptr<VertexBuffer> vertex_buffer_sphere_;
  std::shared_ptr<VertexBuffer> vertex_buffer_suzanne_;
  std::shared_ptr<VertexBuffer> vertex_buffer_torus_;
};

#endif  // NXDK_PGRAPH_TESTS_LIGHTING_CONTROL_TESTS_H
