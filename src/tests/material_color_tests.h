#ifndef NXDK_PGRAPH_TESTS_MATERIAL_COLOR_TESTS_H
#define NXDK_PGRAPH_TESTS_MATERIAL_COLOR_TESTS_H

#include <memory>
#include <vector>

#include "test_suite.h"
#include "vertex_buffer.h"

class TestHost;
namespace PBKitPlusPlus {
class VertexBuffer;
}

using namespace PBKitPlusPlus;

// Tests behavior when lighting is enabled and color components are requested from various sources.
class MaterialColorTests : public TestSuite {
 public:
  struct TestConfig {
    char name[32]{0};

    Color scene_ambient;

    Color material_diffuse;
    Color material_specular;
    Color material_ambient;
    Color material_emissive;
    float material_specular_power{0.0f};

    Color light_diffuse;
    Color light_specular;
    Color light_ambient;
  };

 public:
  MaterialColorTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;

 private:
  void CreateGeometry();
  void Test(TestConfig config);
};

#endif  // NXDK_PGRAPH_TESTS_MATERIAL_COLOR_TESTS_H
