#ifndef NXDK_PGRAPH_TESTS_VERTEX_SHADER_EDGE_CASE_TESTS_H
#define NXDK_PGRAPH_TESTS_VERTEX_SHADER_EDGE_CASE_TESTS_H

#include "test_suite.h"

class VertexShaderEdgeCaseTests : public TestSuite {
 public:
  VertexShaderEdgeCaseTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void TestDP4WithThreeComponentInputs();
};

#endif  // NXDK_PGRAPH_TESTS_VERTEX_SHADER_EDGE_CASE_TESTS_H
