#include "test_suite.h"

#include <cassert>

TestSuite::TestSuite(TestHost& host, std::string output_dir) : host_(host), output_dir_(std::move(output_dir)) {}

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
