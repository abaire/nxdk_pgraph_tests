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
  void Test(uint32_t front_face, uint32_t cull_face);

  static std::string MakeTestName(uint32_t front_face, uint32_t cull_face);
};

#endif  // NXDK_PGRAPH_TESTS_FRONT_FACE_TESTS_H
