#ifndef NXDK_PGRAPH_TESTS_SPECULAR_TESTS_H
#define NXDK_PGRAPH_TESTS_SPECULAR_TESTS_H

#include "test_suite.h"

class VertexBuffer;
class TestHost;

/**
 * Tests handling of specular color with interesting combinations of lighting
 * settings and specular params.
 */
class SpecularTests : public TestSuite {
 public:
  SpecularTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  //! Tests handling of LIGHTING_ENABLE, SPECULAR_ENABLE, and SEPARATE_SPECULAR.
  void TestControlFlags(const std::string& name, bool use_fixed_function, bool enable_lighting);

  //! Tests the behavior of NV097_SET_SPECULAR_PARAMS.
  void TestSpecularParams(const std::string& name, const float* specular_params);

  //! Loads test model meshes.
  void CreateGeometry();

  std::shared_ptr<VertexBuffer> vertex_buffer_cone_;
  std::shared_ptr<VertexBuffer> vertex_buffer_cylinder_;
  std::shared_ptr<VertexBuffer> vertex_buffer_sphere_;
  std::shared_ptr<VertexBuffer> vertex_buffer_suzanne_;
  std::shared_ptr<VertexBuffer> vertex_buffer_torus_;
};

#endif  // NXDK_PGRAPH_TESTS_SPECULAR_TESTS_H
