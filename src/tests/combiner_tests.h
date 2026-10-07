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
  void TestMux();
  void TestCombinerIndependence();
  void TestCombinerColorAlphaIndependence();
  void TestSpecularR0SumFlags();
  void TestInputMappings();
  void TestUnboundTextureSamplers();

  void TestAlphaFromBlue();
  void TestCombinerOps();
  void TestFinalCombinerSpecialInputs();
  void TestSignedCombinerOps();
  void TestSignedToUnsignedMapping();
  void TestTextureDestination();
  void TestShiftClamping();
  void TestSpecularR0Sum();

 private:
  std::shared_ptr<VertexBuffer> vertex_buffers_[6];
};

#endif  // NXDK_PGRAPH_TESTS_COMBINER_TESTS_H
