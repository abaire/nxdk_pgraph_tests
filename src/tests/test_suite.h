#ifndef NXDK_PGRAPH_TESTS_TEST_SUITE_H
#define NXDK_PGRAPH_TESTS_TEST_SUITE_H

#include <chrono>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "pgraph_diff_token.h"

class TestHost;

class TestSuite {
 public:
  enum class SuspectedCrashHandling {
    SKIP_ALL,
    RUN_ALL,
    ASK,
  };

 public:
  TestSuite(TestHost &host, std::string output_dir, std::string suite_name);

  const std::string &Name() const { return suite_name_; };

  virtual void Initialize();
  virtual void Deinitialize();

  void DisableTests(const std::vector<std::string> &tests_to_skip);
  void SetSuspectedCrashes(const std::set<std::string> &test_names) { suspected_crashes_ = test_names; }

  std::vector<std::string> TestNames() const;
  void Run(const std::string &test_name);

  void RunAll();

  void SetSavingAllowed(bool enable = true) { allow_saving_ = enable; }
  void SetSuspectedCrashHandlingMode(SuspectedCrashHandling mode) { suspected_crash_handling_mode_ = mode; }

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

  // Tests that have crashed historically.
  std::set<std::string> suspected_crashes_{};
  SuspectedCrashHandling suspected_crash_handling_mode_{SuspectedCrashHandling::RUN_ALL};

  PGRAPHDiffToken pgraph_diff_;
};

#endif  // NXDK_PGRAPH_TESTS_TEST_SUITE_H
