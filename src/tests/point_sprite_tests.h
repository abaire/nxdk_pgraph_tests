#ifndef NXDK_PGRAPH_TESTS_POINT_SPRITE_TESTS_H
#define NXDK_PGRAPH_TESTS_POINT_SPRITE_TESTS_H

#include "test_suite.h"

/**
 * Tests behavior of points with NV097_SET_POINT_SMOOTH_ENABLE enabled.
 */
class PointSpriteTests : public TestSuite {
 public:
  PointSpriteTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void TestAlphaTest();
};

#endif  // NXDK_PGRAPH_TESTS_POINT_SPRITE_TESTS_H
