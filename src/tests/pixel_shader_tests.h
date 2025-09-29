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

  //! Tests PS_TEXTUREMODES_BUMPENVMAP
  //! Demonstrates the use of NV097_SET_TEXTURE_SET_BUMP_ENV_MAT to modify texture UV coordinates with a normal map.
  //!
  //! u' = u + BUMPENVMAT00 * normalmap.r + BUMPENVMAT10 * normalmap.g
  //! v' = v + BUMPENVMAT01 * normalmap.r + BUMPENVMAT11 * normalmap.g
  void TestBumpEnvMap();

  //  //! Tests PS_TEXTUREMODES_BUMPENVMAP_LUM
  //  //! Demonstrates
  //  void Test();
  //
  //  //! Tests PS_TEXTUREMODES_BRDF
  //  //! Demonstrates
  //  void Test();
  //
  //  //! Tests PS_TEXTUREMODES_DOT_ST
  //  //! Demonstrates
  //  void Test();
  //
  //  //! Tests PS_TEXTUREMODES_DOT_ZW
  //  //! Demonstrates
  //  void Test();
  //
  //  //! Tests PS_TEXTUREMODES_DOT_RFLCT_DIFF
  //  //! Demonstrates
  //  void Test();
  //
  //  //! Tests PS_TEXTUREMODES_DOT_RFLCT_SPEC
  //  //! Demonstrates
  //  void Test();
  //
  //  //! Tests PS_TEXTUREMODES_DOT_STR_3D
  //  //! Demonstrates
  //  void Test();
  //
  //  //! Tests PS_TEXTUREMODES_DOT_STR_CUBE
  //  //! Demonstrates
  //  void Test();
  //
  //  //! Tests PS_TEXTUREMODES_DPNDNT_AR
  //  //! Demonstrates
  //  void Test();
  //
  //  //! Tests PS_TEXTUREMODES_DPNDNT_GB
  //  //! Demonstrates
  //  void Test();
  //
  //  //! Tests PS_TEXTUREMODES_DOT_RFLCT_SPEC_CONST
  //  //! Demonstrates
  //  void Test();

  //! Draws a basic 2d sampled quad at the given coordinates.
  //!
  //! Note: This modifies texture stage settings, shader stage programs, and combiner settings.
  void DrawPlainImage(float x, float y, const ImageResource &image, float quad_size);

 private:
  ImageResource water_bump_map_;
  ImageResource bump_map_test_image_;
};

#endif  // NXDK_PGRAPH_TESTS_PIXEL_SHADER_TESTS_H
