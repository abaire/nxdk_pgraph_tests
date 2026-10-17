#ifndef NXDK_PGRAPH_TESTS_FOG_CARRYOVER_TESTS_H
#define NXDK_PGRAPH_TESTS_FOG_CARRYOVER_TESTS_H

#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"

class TestHost;

/**
 * Tests the behavior when fog is enabled but the fog coordinate is not specified by the shader.
 *
 * This test reproduces behavior observed in "Blade 2" https://github.com/xemu-project/xemu/issues/1852
 *
 * Observations from HW:
 * - A draw that cannot write anything (color, depth, and stencil all disabled) still updates the carried fog values.
 * - Carried fog is tracked per vertex rather than as a single value. For vertices submitted inline (immediate mode
 *   SET_VERTEX, inline buffers, and inline arrays) a vertex that does not write fog inherits the fog of the vertex six
 *   positions earlier in submission order. The association continues across draws and does not depend on whether the
 *   primitives are points or triangles. This is consistent with six vertex output buffers used round robin.
 * - For vertices fetched from memory (draw arrays and inline elements) the inherited values have no stable pattern
 *   and differ between runs, presumably depending on the timing of vertex fetch relative to shader execution.
 * - Waiting for the pushbuffer to become idle between draws changes the association; the six vertex pattern above does
 *   not predict the results of CarryoverNopDraw, which waits between its draws.
 */
class FogCarryoverTests : public TestSuite {
 public:
  FogCarryoverTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;

 private:
  void Test();
  void TestPrimitive(const std::string& name, TestHost::DrawPrimitive primitive);
  void TestNopDraw();
  void TestVertexSlots();
};

#endif  // NXDK_PGRAPH_TESTS_FOG_CARRYOVER_TESTS_H
