#ifndef NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_TESTS_H

#include <string>

class TestHost;
struct TextureFormatInfo;

class TextureFormatTests {
 public:
  TextureFormatTests(TestHost &host, std::string output_dir);

  void Run();

 private:
  void Test(const TextureFormatInfo &texture_format);

 private:
  TestHost &host_;
  std::string output_dir_;
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_TESTS_H
