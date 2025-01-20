#ifndef NXDK_PGRAPH_TESTS_DEPTH_CLAMP_TESTS_H
#define NXDK_PGRAPH_TESTS_DEPTH_CLAMP_TESTS_H

#include "test_suite.h"

class TestHost;

namespace PBKitPlusPlus {
class VertexBuffer;
}

class DepthClampTests : public TestSuite {
 public:
  DepthClampTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void Test(bool w_buffered, bool clamp, bool zbias, bool full_range, bool vsh);
  void TestEqualDepth(bool w_buffered, float ofs);
};

#endif  // NXDK_PGRAPH_TESTS_DEPTH_CLAMP_TESTS_H
