#ifndef NXDK_PGRAPH_TESTS_DEPTH_FORMAT_TESTS_H
#define NXDK_PGRAPH_TESTS_DEPTH_FORMAT_TESTS_H

#include <memory>
#include <string>

#include "test_base.h"

class TestHost;
class VertexBuffer;

class DepthFormatTests : TestBase {
 public:
  DepthFormatTests(TestHost &host, std::string output_dir);

  void Run() override;

 private:
  void CreateGeometry(uint32_t max_depth);
  void Test(uint32_t depth_format, bool compress_z, uint32_t depth_cutoff);
};

#endif  // NXDK_PGRAPH_TESTS_DEPTH_FORMAT_TESTS_H
