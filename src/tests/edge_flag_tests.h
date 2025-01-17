#ifndef NXDK_PGRAPH_TESTS_EDGE_FLAG_TESTS_H
#define NXDK_PGRAPH_TESTS_EDGE_FLAG_TESTS_H

#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"

class TestHost;
class VertexBuffer;

// Tests behavior of 0x16BC - glEdgeFlag
class EdgeFlagTests : public TestSuite {
 public:
  EdgeFlagTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void Test(const std::string& name, bool edge_flag);
};

#endif  // NXDK_PGRAPH_TESTS_EDGE_FLAG_TESTS_H
