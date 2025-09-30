#ifndef NXDK_PGRAPH_TESTS_PIXEL_SHADER_TESTS_H
#define NXDK_PGRAPH_TESTS_PIXEL_SHADER_TESTS_H

#include "image_resource.h"
#include "test_suite.h"

/**
 * Tests various D3D PixelShader / NV097_SET_SHADER_STAGE_PROGRAM operations.
 *
 * See https://xboxdevwiki.net/NV2A/Pixel_Combiner
 * See
 * https://web.archive.org/web/20240607015835/https://developer.download.nvidia.com/assets/gamedev/docs/GDC2K1_DX8_Pixel_Shaders.pdf
 */
class PixelShaderTests : public TestSuite {
 public:
  PixelShaderTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  //! Tests PS_TEXTUREMODES_PASSTHRU
  //! Demonstrates that the 4d texture coordinate may be used directly as a color.
  void TestPassthrough();

  //! Tests PS_TEXTUREMODES_CLIPPLANE
  //! Demonstrates the use of clip planes specified via texcoords in combination with
  //! `NV097_SET_SHADER_CLIP_PLANE_MODE`.
  void TestClipPlane();

  //! Tests PS_TEXTUREMODES_BUMPENVMAP and PS_TEXTUREMODES_BUMPENVMAP_LUM
  //! Demonstrates the use of NV097_SET_TEXTURE_SET_BUMP_ENV_MAT to modify texture UV coordinates with a normal map.
  //!
  //! In luminance mode, demonstrates use of NV097_SET_TEXTURE_SET_BUMP_ENV_SCALE and
  //! NV097_SET_TEXTURE_SET_BUMP_ENV_OFFSET to calculate final RGB color from the normal map's blue channel via
  //! `out_color = out_color * (lum_scale * nm.b + lum_bias)`.
  //!
  //! u' = u + BUMPENVMAT00 * normalmap.r + BUMPENVMAT10 * normalmap.g
  //! v' = v + BUMPENVMAT01 * normalmap.r + BUMPENVMAT11 * normalmap.g
  void TestBumpEnvMap(bool luminance = false);

  //  //! Tests PS_TEXTUREMODES_BRDF
  //  void TestBRDF();

  //! Tests PS_TEXTUREMODES_DOT_ST.
  void TestDotST();

  //! Tests PS_TEXTUREMODES_DOT_ZW.
  void TestDotZW();

  //! Tests PS_TEXTUREMODES_DPNDNT_AR and PS_TEXTUREMODES_DPNDNT_GB.
  void TestDependentColorChannel(bool use_green_blue = false);

  //! Draws a basic 2d sampled quad at the given coordinates.
  //!
  //! Note: This modifies texture stage settings, shader stage programs, and combiner settings.
  void DrawPlainImage(float x, float y, const ImageResource &image, float quad_size, bool border = true) const;
  void DrawZBuffer(float left, float top, uint32_t src_x, uint32_t src_y, uint32_t quad_size) const;

 private:
  ImageResource water_bump_map_;
  ImageResource bump_map_test_image_;
};

#endif  // NXDK_PGRAPH_TESTS_PIXEL_SHADER_TESTS_H
