#ifndef NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_TESTS_H

#include <string>

#include "test_base.h"

class TestHost;
struct TextureFormatInfo;

class TextureFormatTests : TestBase {
 public:
  TextureFormatTests(TestHost &host, std::string output_dir, uint32_t framebuffer_width, uint32_t framebuffer_height);

  void Run() override;

 private:
  void Test(const TextureFormatInfo &texture_format);

  uint32_t framebuffer_width_;
  uint32_t framebuffer_height_;
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_TESTS_H
