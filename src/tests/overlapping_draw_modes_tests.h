#ifndef NXDK_PGRAPH_TESTS_OVERLAPPING_DRAW_MODES_TESTS_H
#define NXDK_PGRAPH_TESTS_OVERLAPPING_DRAW_MODES_TESTS_H

#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"

/**
 * Tests interleaved and consecutive draw calls using different primitive draw modes (e.g. DrawArrays and
 * ArrayElements).
 */
class OverlappingDrawModesTests : public TestSuite {
 public:
  OverlappingDrawModesTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;

 private:
  void CreateTriangles();
  void CreateTriangleStrip();

  //! Tests interleaving ArrayElement and DrawArrays calls within the same draw sequence.
  void TestArrayElementDrawArrayArrayElement();

  //! Tests consecutive DrawArrays calls modifying vertex attributes.
  void TestDrawArrayDrawArray();

  //! Tests consecutive draw call squashing optimizations across distinct primitive types.
  void TestXemuSquashOptimization();

  //! Tests draw call optimization handling for single DrawArrays invocations.
  void TestXemuSquashOptimizationSingleDrawArrays();

 private:
  std::vector<uint32_t> index_buffer_;
};

#endif  // NXDK_PGRAPH_TESTS_OVERLAPPING_DRAW_MODES_TESTS_H
