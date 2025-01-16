#ifndef NXDK_PGRAPH_TESTS_VERTEX_SHADER_ROUNDING_TESTS_H
#define NXDK_PGRAPH_TESTS_VERTEX_SHADER_ROUNDING_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

struct TextureFormatInfo;
class VertexBuffer;

class VertexShaderRoundingTests : public TestSuite {
 public:
  VertexShaderRoundingTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void CreateGeometry();

  void TestRenderTarget();
  void TestGeometry(float bias);
  void TestGeometrySubscreen(float bias);
  void TestGeometrySuperscreen(float draw_width);
  void TestCompositingRenderTarget(int z);
  void TestAdjacentGeometry(float bias);
  void TestProjectedAdjacentGeometry(float bias);

 private:
  uint8_t *render_target_{nullptr};

  std::shared_ptr<VertexBuffer> framebuffer_vertex_buffer_;
};

#endif  // NXDK_PGRAPH_TESTS_VERTEX_SHADER_ROUNDING_TESTS_H
