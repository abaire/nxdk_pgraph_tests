#ifndef NXDK_PGRAPH_TESTS_MATERIAL_COLOR_SOURCE_TESTS_H
#define NXDK_PGRAPH_TESTS_MATERIAL_COLOR_SOURCE_TESTS_H

#include <xbox_math_types.h>

#include <memory>
#include <vector>

#include "test_suite.h"

class TestHost;
class VertexBuffer;

/**
 * Tests behavior when lighting is enabled and color components are requested from various sources. Also tests the
 * behavior of non-zero NV097_SET_MATERIAL_EMISSION values.
 */
class MaterialColorSourceTests : public TestSuite {
 public:
  enum SourceMode {
    SOURCE_MATERIAL,
    SOURCE_DIFFUSE,
    SOURCE_SPECULAR,
  };

 public:
  MaterialColorSourceTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;

 private:
  void Test(const std::string& name, SourceMode source_mode, const XboxMath::vector_t& material_emission);

  static std::string MakeTestName(SourceMode source_mode);
};

#endif  // NXDK_PGRAPH_TESTS_MATERIAL_COLOR_SOURCE_TESTS_H
