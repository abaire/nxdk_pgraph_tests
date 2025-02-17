#ifndef POINTPARAMSTESTS_H
#define POINTPARAMSTESTS_H

#include <string>

#include "test_host.h"
#include "test_suite.h"

/**
 * Tests the behavior of NV097_SET_POINT_PARAMS_ENABLE (0x00000318) and
 * NV097_SET_POINT_PARAMS (0x00000A30).
 *
 * See https://learn.microsoft.com/en-us/windows/win32/direct3d9/point-sprites
 *
 * PointParamsX_SmoothX_PtSize_(FF | VS)
 * Each test renders a grid of points. Point size is set to a test value for all
 * points.
 *
 * As points go from top to bottom:
 *   Z starts at -5 (eye at -7) and increases by 25 per row (max depth = 193).
 *
 * In each row, as points go from left to right, point params are set from the
 *   following:
 *
 * 1. No scale factors, min size 0, size range 0. Should be 1 pixel.
 * 2. No scale factors, min size 16, size range 0. Should be 16 pixels square.
 * 3. Negligible constant scale, size range 16, min size 0. Should be 16 pixels.
 * 4. Negligible constant scale, size range 16, min size 8. Should be 24 pixels.
 * 5. 4 constant scale, size range 14, min size 16. Should be ~23 pixels.
 * 6. 4 constant scale, size range 36, min size 4. Should be ~21 pixels.
 * 7. 256 constant scale, size range 36, min size 4. Should be ~7 pixels.
 * 8. 0.25 linear scale, size range 39, min 1.
 * 9. 0.05 quadratic scale, size range 39, min 1.
 * 10. 4 constant scale, size range 14, min size 16, bias -0.5.
 * 11. 4 constant scale, size range 14, min size 16, bias 0.5.
 * 12. No scale factors, size range 15, min size 1, bias -100.
 * 13. No scale factors, size range 15, min size 1, bias 100.
 *
 * * Detailed_(FF | VS)
 * Each test renders a grid of points. Point size is set to 128 (16 pixels) for
 * all points.
 *
 * As points go from top to bottom:
 *   Z starts at -5 (eye at -7) and increases by 25 per row (max depth = 193).
 *
 * In each row, as points go from left to right, point params are set from the
 *   following:
 *
 * 1. 0.25 linear scale, size range 39, min size 1, scale bias -1.0
 * 2. 0.25 linear scale, size range 39, min size 1, scale bias -0.5
 * 3. 0.25 linear scale, size range 39, min size 1, scale bias -0.25
 * 4. 0.25 linear scale, size range 39, min size 1, scale bias -0.01
 * 5. 0.25 linear scale, size range 39, min size 1, scale bias 0
 * 6. 0.25 linear scale, size range 39, min size 1, scale bias -0.01
 * 7. 0.25 linear scale, size range 39, min size 1, scale bias -0.25
 * 8. 0.25 linear scale, size range 39, min size 1, scale bias -0.5
 * 9. 0.25 linear scale, size range 39, min size 1, scale bias -1.0
 * 10. 4 constant scale, size range 36, min size 4, scale bias -0.4
 * 11. 4 constant scale, size range 36, min size 4, scale bias -0.25
 * 12. 4 constant scale, size range 36, min size 4, scale bias 0.25
 * 13. 4 constant scale, size range 36, min size 4, scale bias 0.4
 */
class PointParamsTests : public TestSuite {
 public:
  PointParamsTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void Test(const std::string &name, bool point_params_enabled, bool point_smooth_enabled, int point_size,
            bool use_shader);

  void TestDetailed(const std::string &name, bool use_shader);
};
#endif  // POINTPARAMSTESTS_H
