#ifndef NXDK_PGRAPH_TESTS_LIGHTING_NORMAL_TESTS_H
#define NXDK_PGRAPH_TESTS_LIGHTING_NORMAL_TESTS_H

#include <memory>
#include <vector>

#include "test_suite.h"

class TestHost;
class VertexBuffer;

/**
 * Tests behavior when lighting is enabled but a normal is not provided in the vertex data.
 *
 * The observed behavior on hardware is that the last set normal is reused for the unspecified vertices.
 */
class LightingNormalTests : public TestSuite {
 public:
  enum DrawMode {
    DRAW_ARRAYS,
    DRAW_INLINE_BUFFERS,
    DRAW_INLINE_ARRAYS,
    DRAW_INLINE_ELEMENTS,
  };

 public:
  LightingNormalTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void CreateGeometry();
  void Test(bool set_normal, const float* normal, DrawMode draw_mode);

  static std::string MakeTestName(bool set_normal, const float* normal, DrawMode draw_mode);

 private:
  std::shared_ptr<VertexBuffer> normal_bleed_buffer_;
  std::shared_ptr<VertexBuffer> lit_buffer_;

  std::vector<uint32_t> lit_index_buffer_;
};

#endif  // NXDK_PGRAPH_TESTS_LIGHTING_NORMAL_TESTS_H
