#ifndef NXDK_PGRAPH_TESTS_TEST_SUITE_H
#define NXDK_PGRAPH_TESTS_TEST_SUITE_H

#include <chrono>
#include <fstream>
#include <map>
#include <string>
#include <vector>

class TestHost;

class TestSuite {
 public:
  TestSuite(TestHost &host, std::string output_dir, std::string suite_name);

  const std::string &Name() const { return suite_name_; };

  virtual void Initialize();
  virtual void Deinitialize() {}

  void DisableTests(const std::vector<std::string> &tests_to_skip);

  std::vector<std::string> TestNames() const;
  void Run(const std::string &test_name);

  void RunAll();

  void SetSavingAllowed(bool enable = true) { allow_saving_ = enable; }

 protected:
  void SetDefaultTextureFormat() const;

  std::chrono::steady_clock::time_point LogTestStart(const std::string &test_name);
  void LogTestEnd(const std::string &test_name, std::chrono::steady_clock::time_point start_time);

 protected:
  TestHost &host_;
  std::string output_dir_;
  std::string suite_name_;

  // Flag to forcibly disallow saving of output (e.g., when in multiframe test mode for debugging).
  bool allow_saving_{true};

  // Map of `test_name` to `void test()`
  std::map<std::string, std::function<void()>> tests_{};
};

#endif  // NXDK_PGRAPH_TESTS_TEST_SUITE_H
