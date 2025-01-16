#ifndef NXDK_PGRAPH_TESTS_SURFACE_PITCH_TESTS_H
#define NXDK_PGRAPH_TESTS_SURFACE_PITCH_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

class SurfacePitchTests : public TestSuite {
 public:
  SurfacePitchTests(TestHost &host, std::string output_dir, const Config &config);
  void Initialize() override;

 private:
  void TestSwizzle();

  void DrawResults(const uint32_t *result_textures, const uint32_t demo_memory) const;
};

#endif  // NXDK_PGRAPH_TESTS_SURFACE_PITCH_TESTS_H
