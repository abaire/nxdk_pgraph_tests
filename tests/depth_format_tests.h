#ifndef NXDK_PGRAPH_TESTS_DEPTH_FORMAT_TESTS_H
#define NXDK_PGRAPH_TESTS_DEPTH_FORMAT_TESTS_H

#include <string>

#include "test_base.h"

extern const uint32_t kDepthFormats[];
extern const uint32_t kNumDepthFormats;

class TestHost;

class DepthFormatTests : TestBase {
 public:
  DepthFormatTests(TestHost &host, std::string output_dir);

  void Run() override;

 private:
  void Test(uint32_t depth_format, bool compress_z, uint32_t depth_cutoff);
};

#endif  // NXDK_PGRAPH_TESTS_DEPTH_FORMAT_TESTS_H
