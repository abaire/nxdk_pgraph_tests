#ifndef OCCLUDE_ZSTENCIL_TESTS_H
#define OCCLUDE_ZSTENCIL_TESTS_H

#include <string>

#include "light.h"
#include "test_host.h"
#include "test_suite.h"

/**
 * Tests the behavior of NV097_SET_OCCLUDE_ZSTENCIL_EN (0x1d84).
 */
class OccludeZStencilTests : public TestSuite {
 public:
  OccludeZStencilTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void Test(const std::string &name, uint32_t zstencil_value);

  void CreateGeometry();
  void SetupLights();

 private:
  std::shared_ptr<Spotlight> spotlight_;
  std::shared_ptr<PointLight> point_light_;
  std::shared_ptr<VertexBuffer> vertex_buffer_plane_;
};

#endif  // OCCLUDE_ZSTENCIL_TESTS_H
