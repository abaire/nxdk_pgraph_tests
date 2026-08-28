#ifndef NXDK_PGRAPH_TESTS_ZERO_STRIDE_TESTS_H
#define NXDK_PGRAPH_TESTS_ZERO_STRIDE_TESTS_H

#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"
#include "vertex_buffer.h"

class TestHost;
class VertexBuffer;

/**
 * Tests vertex buffer streams configured with zero stride (attribute constant across vertices).
 */
class ZeroStrideTests : public TestSuite {
 public:
  enum DrawMode {
    DRAW_ARRAYS,
    DRAW_INLINE_ARRAYS,
    DRAW_INLINE_ELEMENTS,
  };

 public:
  ZeroStrideTests(TestHost& host, std::string output_dir, const Config& config);
  void Initialize() override;

 private:
  void CreateGeometry();

  //! Tests drawing geometry with zero-stride vertex attribute arrays across draw modes.
  void Test(DrawMode draw_mode);
  static std::string MakeTestName(DrawMode draw_mode);

 private:
  std::vector<uint32_t> index_buffer_;
};

#endif  // NXDK_PGRAPH_TESTS_ZERO_STRIDE_TESTS_H
