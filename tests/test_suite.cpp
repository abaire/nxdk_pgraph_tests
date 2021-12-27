#include "test_suite.h"

#include <cassert>

#include "../test_host.h"
#include "texture_format.h"

TestSuite::TestSuite(TestHost& host, std::string output_dir, std::string test_name)
    : host_(host), output_dir_(std::move(output_dir)), test_name_(std::move(test_name)) {
  output_dir_ += "\\";
  output_dir_ += test_name_;
  std::replace(output_dir_.begin(), output_dir_.end(), ' ', '_');
}

std::vector<std::string> TestSuite::TestNames() const {
  std::vector<std::string> ret;
  ret.reserve(tests_.size());

  for (auto& kv : tests_) {
    ret.push_back(kv.first);
  }
  return std::move(ret);
}

void TestSuite::Run(const std::string& test_name) {
  auto it = tests_.find(test_name);
  if (it == tests_.end()) {
    assert(!"Invalid test name");
  }

  it->second();
}

void TestSuite::RunAll() {
  auto names = TestNames();
  for (const auto& test_name : names) {
    Run(test_name);
  }
}

void TestSuite::SetDefaultTextureFormat() const {
  // NV097_SET_TEXTURE_FORMAT_COLOR_SZ_X8R8G8B8
  const TextureFormatInfo& texture_format = kTextureFormats[3];
  host_.SetTextureFormat(texture_format);
}
