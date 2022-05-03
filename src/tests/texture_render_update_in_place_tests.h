#ifndef NXDK_PGRAPH_TESTS_TEXTURE_RENDER_UPDATE_IN_PLACE_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_RENDER_UPDATE_IN_PLACE_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

struct TextureFormatInfo;
class VertexBuffer;

class TextureRenderUpdateInPlaceTests : public TestSuite {
 public:
  TextureRenderUpdateInPlaceTests(TestHost &host, std::string output_dir);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void CreateGeometry();

  void Test();

 private:
  struct s_CtxDma texture_target_ctx_ {};
  uint8_t *render_target_{nullptr};

  std::shared_ptr<VertexBuffer> render_target_vertex_buffer_;
  std::shared_ptr<VertexBuffer> framebuffer_vertex_buffer_;
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_RENDER_UPDATE_IN_PLACE_TESTS_H
