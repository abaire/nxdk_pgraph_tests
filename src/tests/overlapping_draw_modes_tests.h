#ifndef NXDK_PGRAPH_TESTS_OVERLAPPING_DRAW_MODES_TESTS_H
#define NXDK_PGRAPH_TESTS_OVERLAPPING_DRAW_MODES_TESTS_H

#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"

class VertexBuffer;

// Tests behavior when vertex attributes are not provided but are used by shaders.
class OverlappingDrawModesTests : public TestSuite {
 public:
  OverlappingDrawModesTests(TestHost &host, std::string output_dir);

  void Initialize() override;

 private:
  void CreateTriangles();
  void CreateTriangleStrip();

  void TestArrayElementDrawArrayArrayElement();
  void TestDrawArrayDrawArray();

 private:
  std::vector<uint32_t> index_buffer_;
};

#endif  // NXDK_PGRAPH_TESTS_OVERLAPPING_DRAW_MODES_TESTS_H
