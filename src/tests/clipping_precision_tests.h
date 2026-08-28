#ifndef NXDK_PGRAPH_TESTS_CLIPPING_PRECISION_TESTS_H
#define NXDK_PGRAPH_TESTS_CLIPPING_PRECISION_TESTS_H

#include "test_suite.h"

class TestHost;

/**
 * Tests clipping precision for guard band and screen-edge polygon clipping.
 */
class ClippingPrecisionTests : public TestSuite {
 public:
  ClippingPrecisionTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  //! Renders a single test frame evaluating triangle clipping precision against the viewport boundary.
  void TestClippingPrecisionFrame(float ofs, bool perspective_corrected, bool flat, float rotate_angle,
                                  int vertex_cycle, bool done);

  //! Tests clipping precision across offset ranges with specified interpolation, shading, and rotation parameters.
  void TestClippingPrecision(bool perspective_corrected, bool flat, float rotate_angle, int vertex_cycle);
};

#endif  // NXDK_PGRAPH_TESTS_CLIPPING_PRECISION_TESTS_H
