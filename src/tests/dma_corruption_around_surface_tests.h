#ifndef NXDK_PGRAPH_TESTS_DMA_CORRUPTION_AROUND_SURFACE_TESTS_H
#define NXDK_PGRAPH_TESTS_DMA_CORRUPTION_AROUND_SURFACE_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

struct TextureFormatInfo;
class VertexBuffer;

class DMACorruptionAroundSurfaceTests : public TestSuite {
 public:
  DMACorruptionAroundSurfaceTests(TestHost& host, std::string output_dir, const Config& config);
  void Initialize() override;

 private:
  void Test();

  void Draw() const;
  void NoOpDraw() const;
  void WaitForGPU() const;
};

#endif  // NXDK_PGRAPH_TESTS_DMA_CORRUPTION_AROUND_SURFACE_TESTS_H
