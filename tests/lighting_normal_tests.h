#ifndef NXDK_PGRAPH_TESTS_LIGHTING_NORMAL_TESTS_H
#define NXDK_PGRAPH_TESTS_LIGHTING_NORMAL_TESTS_H

#include "test_suite.h"

class TestHost;
class VertexBuffer;

// Tests behavior when lighting is enabled but a normal is not provided in the vertex data.
// The observed behavior on hardware is that the last set normal is reused for the unspecified vertices.
class LightingNormalTests : public TestSuite {
 public:
  LightingNormalTests(TestHost& host, std::string output_dir);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void CreateGeometry();
  void Test(bool set_normal, const float* normal, bool use_inline_buffer);

  static std::string MakeTestName(bool set_normal, const float* normal, bool inline_buffer);

 private:
  std::shared_ptr<VertexBuffer> normal_bleed_buffer_;
  std::shared_ptr<VertexBuffer> lit_buffer_;
};

#endif  // NXDK_PGRAPH_TESTS_LIGHTING_NORMAL_TESTS_H
