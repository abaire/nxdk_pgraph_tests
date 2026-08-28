#ifndef NXDK_PGRAPH_TESTS_DEPTH_CLAMP_TESTS_H
#define NXDK_PGRAPH_TESTS_DEPTH_CLAMP_TESTS_H

#include "test_suite.h"

class TestHost;

namespace PBKitPlusPlus {
class VertexBuffer;
}

/**
 * Tests the behavior of depth clamping and depth range settings.
 */
class DepthClampTests : public TestSuite {
 public:
  DepthClampTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  //! Tests depth clamping and bias interactions across fixed function and programmable shaders.
  void Test(bool w_buffered, bool clamp, bool zbias, bool full_range, bool vsh);

  //! Tests depth buffer behavior when rendering coplanar geometry at equal depth with small offsets.
  void TestEqualDepth(bool w_buffered, float ofs);
};

#endif  // NXDK_PGRAPH_TESTS_DEPTH_CLAMP_TESTS_H
