#ifndef NXDK_PGRAPH_TESTS_STIPPLE_TESTS_H
#define NXDK_PGRAPH_TESTS_STIPPLE_TESTS_H

#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"

// Tests behavior of 0x147C - 3D_POLYGON_STIPPLE_ENABLE
class StippleTests : public TestSuite {
 public:
  StippleTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;
  void TearDownTest() override;

 private:
  void Test(const std::string& name, bool stipple_enable, const std::vector<DWORD>& stipple_pattern);
};

#endif  // NXDK_PGRAPH_TESTS_STIPPLE_TESTS_H
