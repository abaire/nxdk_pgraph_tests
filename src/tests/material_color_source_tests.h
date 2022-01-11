#ifndef NXDK_PGRAPH_TESTS_MATERIAL_COLOR_SOURCE_TESTS_H
#define NXDK_PGRAPH_TESTS_MATERIAL_COLOR_SOURCE_TESTS_H

#include <memory>
#include <vector>

#include "test_suite.h"

class TestHost;
class VertexBuffer;

// Tests behavior when lighting is enabled and color components are requested from various sources.
class MaterialColorSourceTests : public TestSuite {
 public:
  enum SourceMode {
    SOURCE_MATERIAL,
    SOURCE_DIFFUSE,
    SOURCE_SPECULAR,
  };

 public:
  MaterialColorSourceTests(TestHost& host, std::string output_dir);

  void Initialize() override;

 private:
  void CreateGeometry();
  void Test(SourceMode source_mode);
  static std::string MakeTestName(SourceMode source_mode);

 private:
  std::shared_ptr<VertexBuffer> diffuse_buffer_;
  std::shared_ptr<VertexBuffer> specular_buffer_;
  std::shared_ptr<VertexBuffer> emissive_buffer_;
  std::shared_ptr<VertexBuffer> ambient_buffer_;
};

#endif  // NXDK_PGRAPH_TESTS_MATERIAL_COLOR_SOURCE_TESTS_H
