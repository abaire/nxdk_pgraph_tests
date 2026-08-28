#ifndef NXDK_PGRAPH_TESTS_FRONT_FACE_TESTS_H
#define NXDK_PGRAPH_TESTS_FRONT_FACE_TESTS_H

#include "test_suite.h"

class TestHost;

/**
 * Tests handling of NV097_SET_FRONT_FACE and NV097_SET_CULL_FACE, including
 * degenerate winding values.
 */
class FrontFaceTests : public TestSuite {
 public:
  FrontFaceTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;

 private:
  void CreateGeometry();

  //! Tests face culling behavior with the given front-face winding order and cull face mode in solid or line
  //! rasterization.
  void Test(uint32_t front_face, uint32_t cull_face, bool line_mode);

  static std::string MakeTestName(uint32_t front_face, uint32_t cull_face, bool line_mode);
};

#endif  // NXDK_PGRAPH_TESTS_FRONT_FACE_TESTS_H
