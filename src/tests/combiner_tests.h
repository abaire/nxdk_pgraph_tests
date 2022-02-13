#ifndef NXDK_PGRAPH_TESTS_COMBINER_TESTS_H
#define NXDK_PGRAPH_TESTS_COMBINER_TESTS_H

#include <memory>

#include "test_host.h"
#include "test_suite.h"

class TestHost;
class VertexBuffer;

// Tests behavior when vertex attributes have a 0 stride.
class CombinerTests : public TestSuite {
 public:
  CombinerTests(TestHost& host, std::string output_dir);
  void Initialize() override;
  void Deinitialize() override;

 private:
  void CreateGeometry();
  void TestMux();
  void TestCombinerIndependence();

 private:
  std::shared_ptr<VertexBuffer> vertex_buffers_[6];
};

#endif  // NXDK_PGRAPH_TESTS_COMBINER_TESTS_H
