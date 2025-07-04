#ifndef NXDK_PGRAPH_TESTS_MATERIAL_COLOR_SOURCE_TESTS_H
#define NXDK_PGRAPH_TESTS_MATERIAL_COLOR_SOURCE_TESTS_H

#include <xbox_math_types.h>

#include <memory>
#include <vector>

#include "test_suite.h"

class TestHost;

/**
 * Tests behavior when lighting is enabled and color components are requested from various sources. Also tests the
 * behavior of non-zero NV097_SET_MATERIAL_EMISSION values.
 *
 * A color legend is rendered along the left hand side: vD = vertex diffuse. vS = vertex specular. sA = scene ambient.
 * mD = material diffuse. mS = material specular. mE = material emissive. mA = material ambient. A single directional
 * light is used with ambient, diffuse, and specular multipliers all set to 1.0.
 *
 * NV097_SET_SCENE_AMBIENT_COLOR is set to scene ambient + material emissive.
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
  void Test(const std::string& name, SourceMode source_mode, const XboxMath::vector_t& material_emission,
            uint32_t num_lights = 1);

  void TestEmissive(const std::string& name, const XboxMath::vector_t& material_emission);

  static std::string MakeTestName(SourceMode source_mode);
};

#endif  // NXDK_PGRAPH_TESTS_MATERIAL_COLOR_SOURCE_TESTS_H
