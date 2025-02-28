#ifndef NXDK_PGRAPH_TESTS_CLIPPING_PRECISION_TESTS_H
#define NXDK_PGRAPH_TESTS_CLIPPING_PRECISION_TESTS_H

#include "test_suite.h"

class TestHost;

class ClippingPrecisionTests : public TestSuite {
 public:
  ClippingPrecisionTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void TestClippingPrecisionFrame(float ofs, bool perspective_corrected, bool flat, float rotate_angle,
                                  int vertex_cycle, bool done);
  void TestClippingPrecision(bool perspective_corrected, bool flat, float rotate_angle, int vertex_cycle);
};

#endif  // NXDK_PGRAPH_TESTS_CLIPPING_PRECISION_TESTS_H
