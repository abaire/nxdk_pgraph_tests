#ifndef POINTPARAMSTESTS_H
#define POINTPARAMSTESTS_H

#include <string>

#include "test_host.h"
#include "test_suite.h"

/**
 * Tests the behavior of NV097_SET_POINT_PARAMS_ENABLE (0x00000318) and
 * NV097_SET_POINT_PARAMS (0x00000A30).
 *
 * See https://registry.khronos.org/OpenGL/extensions/EXT/EXT_point_parameters.txt
 *
 * Each test renders a grid of points. Point size is set to a test value for all
 * points.
 *
 * As points go from top to bottom:
 *   Z starts at 0 and increases by 0.5 per row.
 */
class PointParamsTests : public TestSuite {
 public:
  PointParamsTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void Test(const std::string &name, bool point_params_enabled, bool point_smooth_enabled, int point_size,
            bool use_shader);
};

#endif  // POINTPARAMSTESTS_H
