#ifndef NXDK_PGRAPH_TESTS_TEXTURE_RENDER_TARGET_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_RENDER_TARGET_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

namespace PBKitPlusPlus {
struct TextureFormatInfo;
class VertexBuffer;
}  // namespace PBKitPlusPlus

using namespace PBKitPlusPlus;

class TextureRenderTargetTests : public TestSuite {
 public:
  TextureRenderTargetTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void CreateGeometry();

  void Test(const TextureFormatInfo &texture_format);
  void TestPalettized(TestHost::PaletteSize size);

  //! Tests behavior when rendering to a surface, using it to texture geometry, then updating the same surface and
  //! texturing different geometry.
  void TestRenderTextureLoop();

  //! Tests emulator texture cache invalidation and coherency when a texture surface is rendered via GPU, then cleared
  //! back to an earlier color state using a linear clear, or updated via CPU.
  void TestXemu2036RenderTextureClearLoop();

  //! Tests rendering an SZ_A8R8G8B8 texture into a swizzled LE_R5G6B5 render target surface, then sampling that surface
  //! as an SZ_R5G6B5 texture onto an LE_A8R8G8B8 framebuffer (xemu #2387).
  void TestA8R8G8B8ToLE_R5G6B5Loop();

  void ResetAndDrawFromRenderTarget() const;

  static std::string MakeTestName(const TextureFormatInfo &texture_format);
  static std::string MakePalettizedTestName(TestHost::PaletteSize size);

 private:
  struct s_CtxDma texture_target_ctx_{};
  uint8_t *render_target_{nullptr};

  std::shared_ptr<VertexBuffer> render_target_vertex_buffer_;
  std::shared_ptr<VertexBuffer> framebuffer_vertex_buffer_;
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_RENDER_TARGET_TESTS_H
