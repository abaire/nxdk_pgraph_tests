#ifndef NXDK_PGRAPH_TESTS_LIGHTING_TWO_SIDED_TESTS_H
#define NXDK_PGRAPH_TESTS_LIGHTING_TWO_SIDED_TESTS_H

#include <cstdint>
#include <memory>
#include <vector>

#include "test_suite.h"

class TestHost;
class VertexBuffer;

// Tests two-sided lighting.
class LightingTwoSidedTests : public TestSuite {
 public:
  LightingTwoSidedTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void Test();

 private:
  std::shared_ptr<VertexBuffer> vertex_buffer_;
};

#endif  // NXDK_PGRAPH_TESTS_LIGHTING_TWO_SIDED_TESTS_H
