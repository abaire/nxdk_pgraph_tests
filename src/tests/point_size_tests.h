#ifndef POINTSIZETESTS_H
#define POINTSIZETESTS_H

#include <string>

#include "test_host.h"
#include "test_suite.h"

/**
 * Tests the behavior of NV097_SET_POINT_SIZE and NV097_SET_POINT_SMOOTH_ENABLE
 *
 * See https://registry.khronos.org/OpenGL-Refpages/gl2.1/xhtml/glPointSize.xml
 *
 * Each test renders a grid of points.
 *
 * As points go from left to right:
 *   point size starts at 0 and increases by point_size_increment per point.
 */
class PointSizeTests : public TestSuite {
 public:
  PointSizeTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void Test(const std::string &name, bool point_smooth_enabled, int point_size_increment, bool use_shader);
};

#endif  // POINTSIZETESTS_H
