#ifndef NXDK_PGRAPH_TESTS_Z_MIN_MAX_CONTROL_TESTS_H
#define NXDK_PGRAPH_TESTS_Z_MIN_MAX_CONTROL_TESTS_H

#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"

class TestHost;
class VertexBuffer;

// Tests 0x1D78 NV097_SET_ZMIN_MAX_CONTROL functions.
class ZMinMaxControlTests : public TestSuite {
 public:
  typedef enum ZMinMaxDrawMode {
    M_Z_INC_W_ONE,
    M_Z_INC_W_INC,
    M_Z_NF_W_INC,
    M_Z_NF_W_NF,
    M_Z_INC_W_TEN,
    M_Z_INC_W_FRAC,
    M_Z_INC_W_INV_Z,
  } ZMinMaxDrawMode;

 public:
  ZMinMaxControlTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;
  void TearDownTest() override;

 private:
  void Test(const std::string& name, uint32_t mode, bool w_buffered);
  void TestFixed(const std::string& name, uint32_t mode, bool w_buffered);

  void DrawBlock(float x_offset, float y_offset, ZMinMaxDrawMode zw_mode, bool projected = false) const;

 private:
  float kLeft{0.f};
  float kRight{0.f};
  float kTop{0.f};
  float kBottom{0.f};
  float kRegionWidth{0.f};
  float kRegionHeight{0.f};

  uint32_t quads_per_row_{0};
  uint32_t quads_per_col_{0};
  float col_size_{0.f};
  uint32_t num_quads_{0};
};

#endif  // NXDK_PGRAPH_TESTS_Z_MIN_MAX_CONTROL_TESTS_H
