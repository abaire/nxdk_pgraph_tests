#ifndef NXDK_PGRAPH_TESTS_COLOR_KEY_TESTS_H
#define NXDK_PGRAPH_TESTS_COLOR_KEY_TESTS_H

#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"

class TestHost;
class VertexBuffer;

// Tests 0x0AE0 color key functions.
class ColorKeyTests : public TestSuite {
 public:
  ColorKeyTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;
  void TearDownTest() override;

 private:
  void TestFixedFunction(const std::string& name, uint32_t mode, bool alpha_from_texture);
  void Test(const std::string& name, uint32_t mode, bool alpha_from_texture);

  //! Demonstrate the fact that unsampled texels will kill pixels entirely.
  void TestUnsampled(const std::string& name);

  void SetupTextureStage(uint32_t stage, uint32_t mode) const;
  void SetupAllTextureStages(uint32_t mode) const;
  void DisableAllTextureStages() const;
};

#endif  // NXDK_PGRAPH_TESTS_COLOR_KEY_TESTS_H
