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
  VertexShaderRoundingTests(TestHost &host, std::string output_dir);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void CreateGeometry();

  void TestRenderTarget();
  void TestGeometry(float bias);
  void TestCompositingRenderTarget(int z);

  static std::string MakeGeometryTestName(float bias);

 private:
  struct s_CtxDma texture_target_ctx_ {};
  uint8_t *render_target_{nullptr};

  std::shared_ptr<VertexBuffer> framebuffer_vertex_buffer_;
};

#endif  // NXDK_PGRAPH_TESTS_VERTEX_SHADER_ROUNDING_TESTS_H
