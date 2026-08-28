#ifndef NXDK_PGRAPH_TESTS_TEXTURE_BORDER_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_BORDER_TESTS_H

#include <string>

#include "test_host.h"
#include "test_suite.h"

namespace PBKitPlusPlus {
class VertexBuffer;
}

using namespace PBKitPlusPlus;

/**
 * Tests textures with border dimensions (2D, 3D, and cubemap textures with 1-pixel borders).
 */
class TextureBorderTests : public TestSuite {
 public:
  TextureBorderTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void CreateGeometry();

  //! Tests sampling 2D textures configured with borders.
  void Test2D();

  //! Tests border coordinate clamping and address generation edge cases.
  void TestXemu1034();

  //! Tests 2D swizzled textures with border pixels.
  void Test2DBorderedSwizzled();
  //  void Test2DPalettized();

  //! Tests 3D volumetric swizzled textures with border voxels.
  void Test3DBorderedSwizzled(const std::string &name, uint32_t width, uint32_t height);

  //! Tests cubemap swizzled textures with border texels.
  void TestCubemapBorderedSwizzled(const std::string &name, uint32_t width, uint32_t height);

  void GenerateBordered3DSurface(uint8_t *texture_memory, uint32_t width, uint32_t height, uint32_t depth,
                                 bool swizzle) const;
  void GenerateBorderedCubemapSurface(uint8_t *texture_memory, uint32_t width, uint32_t height, bool swizzle) const;

 private:
  std::shared_ptr<VertexBuffer> vertex_buffers_[6];
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_BORDER_TESTS_H
