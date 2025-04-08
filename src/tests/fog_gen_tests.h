#ifndef NXDK_PGRAPH_TESTS_FOG_GEN_TESTS_H
#define NXDK_PGRAPH_TESTS_FOG_GEN_TESTS_H

#include <memory>
#include <vector>

#include "test_suite.h"

class TestHost;

/**
 * Tests the effects of NV097_SET_FOG_GEN_MODE.
 *
 * Each test renders a series of blueish quads placed progressively further from the eye position. Each quad's left side
 * is 1.0f closer to the eye than its right.
 *
 * Each quad's specular alpha components are hardcoded to 0.0, 1.0, 0.75, 0.25 as viewed clockwise from the upper left.
 *
 * The fog coordinate starts at 50.0 and is decremented by 0.2 for each quad rendered from upper left to lower right.
 *
 */
class FogGenTests : public TestSuite {
 public:
  FogGenTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;

 private:
  void Test(const std::string& name, uint32_t fog_mode, uint32_t fog_gen_mode, bool use_fixed_function);
};

#endif  // NXDK_PGRAPH_TESTS_FOG_GEN_TESTS_H
