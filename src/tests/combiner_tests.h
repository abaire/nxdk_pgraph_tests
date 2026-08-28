#ifndef NXDK_PGRAPH_TESTS_COMBINER_TESTS_H
#define NXDK_PGRAPH_TESTS_COMBINER_TESTS_H

#include <memory>

#include "test_host.h"
#include "test_suite.h"

class TestHost;
namespace PBKitPlusPlus {
class VertexBuffer;
}
using namespace PBKitPlusPlus;

//! Tests behavior of NV097_SET_COMBINER_* and NV097_SET_SPECULAR_* final
//! combiner operations.
class CombinerTests : public TestSuite {
 public:
  CombinerTests(TestHost& host, std::string output_dir, const Config& config);
  void Initialize() override;
  void Deinitialize() override;

 private:
  void CreateGeometry();

  //! Tests multiplexer operations in register combiners.
  void TestMux();

  //! Tests independence between combiner stages.
  void TestCombinerIndependence();

  //! Tests independence between RGB and alpha combiner computations.
  void TestCombinerColorAlphaIndependence();

  //! Tests condition flags and mapping modes in register combiners.
  void TestFlags();

  //! Tests combiner behavior when sampling an unbound texture stage.
  void TestUnboundTexture();

  //! Tests combiner outputs when referencing texture samplers that have no texture bound.
  void TestUnboundTextureSamplers();

  //! Tests extracting alpha values from the blue color channel in combiners.
  void TestAlphaFromBlue();

  //! Tests various arithmetic and mapping operations available in general combiners.
  void TestCombinerOps();

  //! Tests special input sources (such as zero, diffuse, specular, and fog) to the final combiner.
  void TestFinalCombinerSpecialInputs();

 private:
  std::shared_ptr<VertexBuffer> vertex_buffers_[6];
};

#endif  // NXDK_PGRAPH_TESTS_COMBINER_TESTS_H
