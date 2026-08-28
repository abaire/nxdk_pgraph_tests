#ifndef NXDK_PGRAPH_TESTS_VERTEX_SHADER_INDEPENDENCE_TESTS_H
#define NXDK_PGRAPH_TESTS_VERTEX_SHADER_INDEPENDENCE_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

/**
 * Tests vertex shader execution independence between MAC and ILU calculation pipelines.
 */
class VertexShaderIndependenceTests : public TestSuite {
 public:
  VertexShaderIndependenceTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;

 private:
  void CreateGeometry();

  //! Tests parallel execution and data hazard independence between MAC and ILU pipelines.
  void TestMACILUIndependence();

  //! Tests vertex shader instructions generating multiple simultaneous output register writes.
  void TestMultiOutput();
};

#endif  // NXDK_PGRAPH_TESTS_VERTEX_SHADER_INDEPENDENCE_TESTS_H
