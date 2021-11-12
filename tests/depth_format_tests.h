#ifndef NXDK_PGRAPH_TESTS_DEPTH_FORMAT_TESTS_H
#define NXDK_PGRAPH_TESTS_DEPTH_FORMAT_TESTS_H

#include <cstdint>
#include <memory>
#include <string>

#include "test_base.h"

class TestHost;
class VertexBuffer;

class DepthFormatTests : TestBase {
 public:
  struct DepthFormat {
    uint32_t float_to_fixed(float val) const;
    float fixed_to_float(uint32_t val) const;

    uint32_t format;
    uint32_t max_depth;
  };

 public:
  DepthFormatTests(TestHost &host, std::string output_dir);

  void Run() override;

 private:
  void CreateGeometry(const DepthFormat &format, bool z_format_float);
  void Test(const DepthFormat &format, bool compress_z, bool z_format_float, uint32_t depth_cutoff);
};

#endif  // NXDK_PGRAPH_TESTS_DEPTH_FORMAT_TESTS_H
