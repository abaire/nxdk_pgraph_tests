#ifndef NXDK_PGRAPH_TESTS_DMA_TOP_OF_MEMORY_TESTS_H
#define NXDK_PGRAPH_TESTS_DMA_TOP_OF_MEMORY_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

struct TextureFormatInfo;
class VertexBuffer;

class DMATopOfMemoryTests : public TestSuite {
 public:
  DMATopOfMemoryTests(TestHost& host, std::string output_dir, const Config& config);
  void Initialize() override;

 private:
  void TestReadTextureBeyondTopOfMemory();
  void TestClearSurfaceBeyondTopOfMemory();

 private:
  struct s_CtxDma all_memory_ctx_{};
};

#endif  // NXDK_PGRAPH_TESTS_DMA_TOP_OF_MEMORY_TESTS_H
