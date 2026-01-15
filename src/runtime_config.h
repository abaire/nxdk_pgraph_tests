#ifndef NXDK_PGRAPH_TESTS_RUNTIME_CONFIG_H
#define NXDK_PGRAPH_TESTS_RUNTIME_CONFIG_H

#include <memory>
#include <vector>

#include "configure.h"
#include "tests/test_suite.h"

class RuntimeConfig {
 public:
  enum class SkipConfiguration {
    DEFAULT,
    SKIPPED,
    UNSKIPPED,
  };

  enum class NetworkConfigMode {
    OFF,
    AUTOMATIC,
    DHCP,
    STATIC,
  };

 public:
  RuntimeConfig() = default;
  explicit RuntimeConfig(const RuntimeConfig&) = delete;

  /**
   * Loads the JSON config file from the given path.
   * @param config_file_path - The path to the file containing the JSON config.
   * @param errors - Vector of strings into which any error messages will be placed.
   * @return true on success, false on failure
   */
  bool LoadConfig(const char* config_file_path, std::vector<std::string>& errors);

  /**
   * Loads the JSON config file from the given string buffer.
   * @param config_content - String containing the JSON config.
   * @param errors - Vector of strings into which any error messages will be placed.
   * @return true on success, false on failure
   */
  bool LoadConfigBuffer(const std::string& config_content, std::vector<std::string>& errors);

  /**
   * Processes the JSON config file at the given path and adjusts the given set of test suites. Returns false if parsing
   * fails for any reason.
   */
  bool ApplyConfig(std::vector<std::shared_ptr<TestSuite>>& test_suites, std::vector<std::string>& errors);

#ifdef DUMP_CONFIG_FILE
  /**
   * Generates a default config file at the given path.
   */
  bool DumpConfig(const std::string& config_file_path, const std::vector<std::shared_ptr<TestSuite>>& test_suites,
                  std::vector<std::string>& errors);

  bool DumpConfigToStream(std::ostream& config_file, const std::vector<std::shared_ptr<TestSuite>>& test_suites,
                          std::vector<std::string>& errors);
#endif  // DUMP_CONFIG_FILE

  [[nodiscard]] bool enable_progress_log() const { return enable_progress_log_; }
  [[nodiscard]] bool disable_autorun() const { return disable_autorun_; }
  [[nodiscard]] bool enable_autorun_immediately() const { return enable_autorun_immediately_; }
  [[nodiscard]] bool enable_shutdown_on_completion() const { return enable_shutdown_on_completion_; }
  [[nodiscard]] bool enable_pgraph_region_diff() const { return enable_pgraph_region_diff_; }
  [[nodiscard]] bool skip_tests_by_default() const { return skip_tests_by_default_; }
  [[nodiscard]] uint32_t delay_milliseconds_between_tests() const { return delay_milliseconds_between_tests_; }
  [[nodiscard]] uint32_t delay_milliseconds_before_exit() const { return delay_milliseconds_before_exit_; }

  [[nodiscard]] uint32_t ftp_server_ip() const { return ftp_server_ip_; }
  [[nodiscard]] uint16_t ftp_server_port() const { return ftp_server_port_; }
  [[nodiscard]] const std::string& ftp_user() const { return ftp_user_; }
  [[nodiscard]] const std::string& ftp_password() const { return ftp_password_; }
  [[nodiscard]] uint32_t ftp_timeout_milliseconds() const { return ftp_timeout_milliseconds_; }

  [[nodiscard]] NetworkConfigMode network_config_mode() const { return network_config_mode_; }
  [[nodiscard]] uint32_t static_ip() const { return static_ip_; }
  [[nodiscard]] uint32_t static_gateway() const { return static_gateway_; }
  [[nodiscard]] uint32_t static_netmask() const { return static_netmask_; }
  [[nodiscard]] uint32_t static_dns_1() const { return static_dns_1_; }
  [[nodiscard]] uint32_t static_dns_2() const { return static_dns_2_; }

  [[nodiscard]] const std::string& output_directory_path() const { return output_directory_path_; }

  static std::string SanitizePath(const std::string& path);

 private:
#ifdef DUMP_CONFIG_FILE
  static std::string EscapePath(const std::string& path);

  void WriteSettings(std::ostream& output) const;
  void WriteTestSuite(std::ostream& output, const std::shared_ptr<TestSuite>& suite) const;
  void WriteTestCase(std::ostream& output, const std::string& suite_name, const std::string& test_name) const;
#endif  // DUMP_CONFIG_FILE

  bool ProcessNetworkSettings(const void* parent, std::vector<std::string>& errors);

 private:
  bool enable_progress_log_ = DEFAULT_ENABLE_PROGRESS_LOG;
  //! Add a delay before running each test. This may be useful in conjunction with the progress log if specific tests
  //! crash an emulator; giving more time for the log to be flushed to the filesystem.
  uint32_t delay_milliseconds_between_tests_{0};
  uint32_t delay_milliseconds_before_exit_{DEFAULT_DELAY_MILLISECONDS_BEFORE_EXIT};
  bool disable_autorun_ = DEFAULT_DISABLE_AUTORUN;
  bool enable_autorun_immediately_ = DEFAULT_AUTORUN_IMMEDIATELY;
  bool enable_shutdown_on_completion_ = DEFAULT_ENABLE_SHUTDOWN;
  bool enable_pgraph_region_diff_ = DEFAULT_ENABLE_PGRAPH_REGION_DIFF;
  bool skip_tests_by_default_ = DEFAULT_SKIP_TESTS_BY_DEFAULT;

  uint32_t ftp_server_ip_{0};
  uint16_t ftp_server_port_{0};
  std::string ftp_user_;
  std::string ftp_password_;
  uint32_t ftp_timeout_milliseconds_{0};

  NetworkConfigMode network_config_mode_ = NetworkConfigMode::OFF;
  uint32_t static_ip_{0};
  uint32_t static_gateway_{0};
  uint32_t static_netmask_{0};
  uint32_t static_dns_1_{0};
  uint32_t static_dns_2_{0};

  std::string output_directory_path_ = SanitizePath(DEFAULT_OUTPUT_DIRECTORY_PATH);

  //! Map of test suite name to skip config.
  std::map<std::string, SkipConfiguration> configured_test_suites_;
  //! Map of test suite name to a map of test case to skip config.
  std::map<std::string, std::map<std::string, SkipConfiguration>> configured_test_cases_;
};

#endif  // NXDK_PGRAPH_TESTS_RUNTIME_CONFIG_H
