#ifndef NXDK_PGRAPH_TESTS_BUMP_MAP_TESTS_H
#define NXDK_PGRAPH_TESTS_BUMP_MAP_TESTS_H

#include <string>

#include "test_host.h"
#include "test_suite.h"

namespace PBKitPlusPlus {
struct TextureFormatInfo;
}

/**
 * @brief Tests NV2A bump mapping using various texture formats, orientations, and signed/unsigned component filtering.
 *
 * Exercises bump mapping via projective 2D and dot product / bumpenvmap shader stages. Tests standard
 * 8-bit du/dv bump mapping with G8B8, R6G5B5, and other formats across combinations of signed and unsigned
 * coordinates, 90-degree rotations, and cross-channel mappings. Also exercises 16-bit HILO bump mapping
 * using NV097_SET_TEXTURE_FORMAT_COLOR_SZ_YB_16_YA_16 and NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_YB16YA16.
 */
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
