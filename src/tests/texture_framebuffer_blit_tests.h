#ifndef NXDK_PGRAPH_TESTS_TEXTURE_FRAMEBUFFER_BLIT_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_FRAMEBUFFER_BLIT_TESTS_H

#include <pbkit/pbkit.h>

#include "test_host.h"
#include "test_suite.h"

class TestHost;

class TextureFramebufferBlitTests : public TestSuite {
 public:
  TextureFramebufferBlitTests(TestHost& host, std::string output_dir);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void CreateGeometry();
  void TestRenderTarget(const char* test_name);
  void Test(uint32_t texture_destination, const char* test_name);
  void ImageBlit(uint32_t operation, uint32_t beta, uint32_t source_channel, uint32_t destination_channel,
                 uint32_t surface_format, uint32_t source_pitch, uint32_t destination_pitch, uint32_t source_offset,
                 uint32_t source_x, uint32_t source_y, uint32_t destination_offset, uint32_t destination_x,
                 uint32_t destination_y, uint32_t width, uint32_t height, uint32_t clip_x, uint32_t clip_y,
                 uint32_t clip_width, uint32_t clip_height) const;

  // The geometry used to create the texture.
  std::shared_ptr<VertexBuffer> texture_source_vertex_buffer_;

  // The bi-triangle used to render the texture to the final framebuffer.
  std::shared_ptr<VertexBuffer> target_vertex_buffer_;

  struct s_CtxDma texture_target_ctx_ {};
  struct s_CtxDma null_ctx_ {};
  struct s_CtxDma clip_rect_ctx_ {};
  struct s_CtxDma beta_ctx_ {};
  struct s_CtxDma beta4_ctx_ {};
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_FRAMEBUFFER_BLIT_TESTS_H
