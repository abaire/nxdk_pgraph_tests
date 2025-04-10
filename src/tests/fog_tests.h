#ifndef NXDK_PGRAPH_TESTS_FOG_TESTS_H
#define NXDK_PGRAPH_TESTS_FOG_TESTS_H

#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"
#include "vertex_buffer.h"

class TestHost;
class VertexBuffer;

// Tests behavior of the Fog code.
class FogTests : public TestSuite {
 public:
  enum FogMode {
    FOG_LINEAR = NV097_SET_FOG_MODE_V_LINEAR,
    FOG_EXP = NV097_SET_FOG_MODE_V_EXP,
    FOG_EXP2 = NV097_SET_FOG_MODE_V_EXP2,
    FOG_EXP_ABS = NV097_SET_FOG_MODE_V_EXP_ABS,
    FOG_EXP2_ABS = NV097_SET_FOG_MODE_V_EXP2_ABS,
    FOG_LINEAR_ABS = NV097_SET_FOG_MODE_V_LINEAR_ABS,
  };

  enum FogGenMode {
    FOG_GEN_SPEC_ALPHA = NV097_SET_FOG_GEN_MODE_V_SPEC_ALPHA,
    FOG_GEN_RADIAL = NV097_SET_FOG_GEN_MODE_V_RADIAL,
    FOG_GEN_PLANAR = NV097_SET_FOG_GEN_MODE_V_PLANAR,
    FOG_GEN_ABS_PLANAR = NV097_SET_FOG_GEN_MODE_V_ABS_PLANAR,
    FOG_GEN_FOG_X = NV097_SET_FOG_GEN_MODE_V_FOG_X,
  };

 public:
  FogTests(TestHost& host, std::string output_dir, const Config& config, std::string suite_name = "Fog");
  void Initialize() override;
  void Deinitialize() override;

 protected:
  virtual void CreateGeometry();
  void Test(FogMode fog_mode, FogGenMode gen_mode, uint32_t fog_alpha);

  static std::string MakeTestName(FogMode fog_mode, FogGenMode gen_mode, uint32_t fog_alpha);

 protected:
  std::shared_ptr<VertexBuffer> vertex_buffer_;
};

class FogCustomShaderTests : public FogTests {
 public:
  FogCustomShaderTests(TestHost& host, std::string output_dir, const Config& config,
                       std::string suite_name = "Fog vsh");
  void Initialize() override;
};

class FogInfiniteFogCoordinateTests : public FogCustomShaderTests {
 public:
  FogInfiniteFogCoordinateTests(TestHost& host, std::string output_dir, const Config& config);
  void Initialize() override;
};

class FogVec4CoordTests : public FogCustomShaderTests {
 public:
  struct TestConfig {
    const char* prefix;
    const uint32_t* shader;
    const uint32_t shader_size;
    float fog[4];
  };

 public:
  FogVec4CoordTests(TestHost& host, std::string output_dir, const Config& config);
  void Initialize() override;

 private:
  // Verifies the behavior of setting individual components of the oFog register, showing that the last set value is
  // taken as the value regardless of the destination mask.
  // E.g., `mov oFog.w, 0.5` will effectively set oFog.x to 0.5, obliterating any previous value.
  void Test(const TestConfig& config);

  // Tests behavior when the fog register is used without being set.
  void TestUnset();

  void SetShader(const TestConfig& config) const;
  static std::string MakeTestName(const TestConfig& config);
};

#endif  // NXDK_PGRAPH_TESTS_FOG_TESTS_H
