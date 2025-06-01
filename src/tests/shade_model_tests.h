#ifndef NXDK_PGRAPH_TESTS_SHADE_MODEL_TESTS_H
#define NXDK_PGRAPH_TESTS_SHADE_MODEL_TESTS_H

#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"

class TestHost;

// Tests behavior when lighting is enabled but a normal is not provided in the vertex data.
// The observed behavior on hardware is that the last set normal is reused for the unspecified vertices.
class ShadeModelTests : public TestSuite {
 public:
  ShadeModelTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;

 private:
  void TestShadeModelFixed(uint32_t model, uint32_t provoking_vertex, TestHost::DrawPrimitive primitive, bool texture);
  void TestShadeModel(uint32_t model, uint32_t provoking_vertex, TestHost::DrawPrimitive primitive, bool texture,
                      bool line_mode = false);

  void TestShadeModelFixed_W(uint32_t model, uint32_t provoking_vertex, TestHost::DrawPrimitive primitive, bool texture,
                             float w, float w_inc);
};

#endif  // NXDK_PGRAPH_TESTS_SHADE_MODEL_TESTS_H
