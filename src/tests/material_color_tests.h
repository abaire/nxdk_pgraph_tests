#ifndef NXDK_PGRAPH_TESTS_MATERIAL_COLOR_TESTS_H
#define NXDK_PGRAPH_TESTS_MATERIAL_COLOR_TESTS_H

#include <memory>
#include <vector>

#include "test_suite.h"

class TestHost;
class VertexBuffer;

// Tests behavior when lighting is enabled and color components are requested from various sources.
class MaterialColorTests : public TestSuite {
 public:
  enum SourceMode {
    SOURCE_MATERIAL,
    SOURCE_DIFFUSE,
    SOURCE_SPECULAR,
  };

 public:
  MaterialColorTests(TestHost& host, std::string output_dir);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void CreateGeometry();
  void Test(uint32_t diffuse_source, uint32_t specular_source, uint32_t emissive_source, uint32_t ambient_source);
  static std::string MakeTestName(uint32_t diffuse_source, uint32_t specular_source, uint32_t emissive_source,
                                  uint32_t ambient_source);
};

#endif  // NXDK_PGRAPH_TESTS_MATERIAL_COLOR_TESTS_H
