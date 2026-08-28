#ifndef NXDK_PGRAPH_TESTS_SHADE_MODEL_TESTS_H
#define NXDK_PGRAPH_TESTS_SHADE_MODEL_TESTS_H

#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"

class TestHost;

/**
 * Tests flat vs smooth shading modes (NV097_SET_SHADE_MODEL) and provoking vertex conventions.
 */
class ShadeModelTests : public TestSuite {
 public:
  ShadeModelTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;

 private:
  //! Tests flat and smooth shade modeling in the fixed-function pipeline.
  void TestShadeModelFixed(uint32_t model, uint32_t provoking_vertex, TestHost::DrawPrimitive primitive, bool texture);

  //! Tests flat and smooth shade modeling in programmable shaders with optional wireframe mode.
  void TestShadeModel(uint32_t model, uint32_t provoking_vertex, TestHost::DrawPrimitive primitive, bool texture,
                      bool line_mode = false);

  //! Tests shade modeling interactions with varying projective W-coordinates.
  void TestShadeModelFixed_W(uint32_t model, uint32_t provoking_vertex, TestHost::DrawPrimitive primitive, bool texture,
                             float w, float w_inc);
};

#endif  // NXDK_PGRAPH_TESTS_SHADE_MODEL_TESTS_H
