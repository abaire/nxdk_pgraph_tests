#ifndef NXDK_PGRAPH_TESTS_LIGHTING_SPECULAR_TESTS_H
#define NXDK_PGRAPH_TESTS_LIGHTING_SPECULAR_TESTS_H

#include <cstdint>
#include <memory>
#include <vector>

#include "test_suite.h"

class TestHost;
class VertexBuffer;

// Tests specular lighting commands.
class LightingSpecularTests : public TestSuite {
 public:
  enum DrawMode {
    DRAW_ARRAYS,
    DRAW_INLINE_BUFFERS,
    DRAW_INLINE_ARRAYS,
    DRAW_INLINE_ELEMENTS,
  };

 public:
  LightingSpecularTests(TestHost& host, std::string output_dir);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void CreateGeometry();
  void Test(bool specular_enabled);

  static std::string MakeTestName(bool specular_enabled);

 private:
  std::shared_ptr<VertexBuffer> vertex_buffer_;
};

#endif  // NXDK_PGRAPH_TESTS_LIGHTING_SPECULAR_TESTS_H
