#ifndef NXDK_PGRAPH_TESTS_LIGHTING_TWO_SIDED_TESTS_H
#define NXDK_PGRAPH_TESTS_LIGHTING_TWO_SIDED_TESTS_H

#include <cstdint>
#include <memory>
#include <vector>

#include "test_suite.h"

class TestHost;
namespace PBKitPlusPlus {
class VertexBuffer;
}

using namespace PBKitPlusPlus;

/**
 * Tests two-sided lighting behavior and back-facing polygon material color lighting calculations.
 */
class LightingTwoSidedTests : public TestSuite {
 public:
  LightingTwoSidedTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  //! Tests two-sided lighting evaluation on front-facing and back-facing polygon geometry.
  void Test();

 private:
  std::shared_ptr<VertexBuffer> vertex_buffer_;
};

#endif  // NXDK_PGRAPH_TESTS_LIGHTING_TWO_SIDED_TESTS_H
