#ifndef NXDK_PGRAPH_TESTS_BUMP_MAP_TESTS_H
#define NXDK_PGRAPH_TESTS_BUMP_MAP_TESTS_H

#include <string>

#include "test_host.h"
#include "test_suite.h"

namespace PBKitPlusPlus {
struct TextureFormatInfo;
}

class BumpMapTests : public TestSuite {
 public:
  BumpMapTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void DrawRectangles(const TextureFormatInfo &texture_format, char x_channel, char y_channel, bool cross_on_blue,
                      bool rotate90, const std::function<void(float, float, float, float)> &draw);
  void Test(const TextureFormatInfo &texture_format, bool cross_on_blue, bool rotate90);
  void Test16bit(const TextureFormatInfo &texture_format, bool cross_on_blue);
  static std::string MakeTestName(const TextureFormatInfo &texture_format, bool cross_on_blue, bool rotate90);
};

#endif  // NXDK_PGRAPH_TESTS_BUMP_MAP_TESTS_H
