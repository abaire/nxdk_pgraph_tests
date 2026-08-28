#ifndef NXDK_PGRAPH_TESTS_INF_TESTS_H
#define NXDK_PGRAPH_TESTS_INF_TESTS_H

#include "test_suite.h"

class TestHost;
namespace PBKitPlusPlus {
class VertexBuffer;
}

/**
 * Tests vertex projective W-coordinate exceptional values (zero, negative, infinity) and perspective interpolation.
 */
class WParamTests : public TestSuite {
 public:
  WParamTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void CreateGeometryWGaps();

  //! Tests geometry rendering with step discontinuities (gaps) in vertex W coordinates.
  void TestWGaps(bool texture_perspective_enable);

  void CreateGeometryPositiveWTriangleStrip();

  //! Tests triangle strips with strictly positive vertex W coordinates.
  void TestPositiveWTriangleStrip(bool texture_perspective_enable);

  void CreateGeometryNegativeWTriangleStrip();

  //! Tests triangle strips with negative vertex W coordinates.
  void TestNegativeWTriangleStrip(bool texture_perspective_enable);

  //! Tests fixed-function handling of zero W coordinates.
  void TestFixedFunctionZeroW(bool draw_quad, bool texture_perspective_enable);

  //! Tests fixed-function handling of infinite and zero W coordinates with scaling multipliers.
  void TestFixedFunctionZeroInfW(bool draw_quad, float w_multiplier);

  //! Tests programmable shader handling of infinite and zero W coordinates.
  void TestProgZeroInfW(bool draw_quad, float w_multiplier);

  //! Tests reciprocal clamp (RCC) vertex shader instruction handling of infinite and zero W coordinates.
  void TestRccZeroInfW(float w_multiplier);

 private:
  std::shared_ptr<PBKitPlusPlus::VertexBuffer> triangle_strip_;
  std::shared_ptr<PBKitPlusPlus::VertexBuffer> triangles_;
};

#endif  // NXDK_PGRAPH_TESTS_INF_TESTS_H
