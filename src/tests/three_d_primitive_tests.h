#ifndef NXDK_PGRAPH_TESTS_THREE_D_PRIMITIVE_TESTS_H
#define NXDK_PGRAPH_TESTS_THREE_D_PRIMITIVE_TESTS_H

#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"
#include "vertex_buffer.h"

class VertexBuffer;

/**
 * Tests behavior when rendering various 3D primitives.
 */
class ThreeDPrimitiveTests : public TestSuite {
 public:
  enum DrawMode {
    DRAW_ARRAYS,
    DRAW_INLINE_BUFFERS,
    DRAW_INLINE_ARRAYS,
    DRAW_INLINE_ELEMENTS,
  };

 public:
  ThreeDPrimitiveTests(TestHost& host, std::string output_dir, const Config& config);
  void Initialize() override;

 private:
  void CreateLines();

  void CreateTriangles();
  void CreateTriangleStrip();
  void CreateTriangleFan();

  void CreateQuads();
  void CreateQuadStrip();

  void CreatePolygon();

  void Test(TestHost::DrawPrimitive primitive, DrawMode draw_mode, bool line_smooth, bool poly_smooth);

  static std::string MakeTestName(TestHost::DrawPrimitive primitive, DrawMode draw_mode, bool line_smooth,
                                  bool poly_smooth);

 private:
  std::vector<uint32_t> index_buffer_;

  static constexpr const char* kSuiteName = "3D primitive";
};

#endif  // NXDK_PGRAPH_TESTS_THREE_D_PRIMITIVE_TESTS_H
