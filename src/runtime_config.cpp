#include "runtime_config.h"

#include <fstream>
#ifdef NXDK
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmacro-redefined"
#pragma clang diagnostic ignored "-Wignored-attributes"
#include <lwip/inet.h>
#pragma clang diagnostic pop
#else
#include <arpa/inet.h>
#endif

#include <tiny-json/tiny-json.h>

#define MAX_CONFIG_FILE_SIZE (1024 * 128)

static bool ParseTestSuites(
    json_t const* test_suites, std::vector<std::string>& errors,
    std::map<std::string, RuntimeConfig::SkipConfiguration>& skipped_test_suites,
    std::map<std::string, std::map<std::string, RuntimeConfig::SkipConfiguration>>& skipped_test_cases);

//! C++ wrapper around Tiny-JSON jsonPool_t
class JSONParser : jsonPool_t {
 public:
  JSONParser() : jsonPool_t{&Alloc, &Alloc} {}
  explicit JSONParser(const char* str) : jsonPool_t{&Alloc, &Alloc}, json_string_{str} {
    root_node_ = json_createWithPool(json_string_.data(), this);
  }

  JSONParser(const JSONParser&) = delete;
  JSONParser(JSONParser&&) = delete;
  JSONParser& operator=(const JSONParser&) = delete;
  JSONParser& operator=(JSONParser&&) = delete;

  [[nodiscard]] json_t const* root() const { return root_node_; }

 private:
  static json_t* Alloc(jsonPool_t* pool) {
    const auto list_pool = static_cast<JSONParser*>(pool);
    list_pool->object_list_.emplace_back();
    return &list_pool->object_list_.back();
  }

  std::list<json_t> object_list_{};
  std::string json_string_{};
  json_t const* root_node_{};
};

bool RuntimeConfig::LoadConfig(const char* config_file_path, std::vector<std::string>& errors) {
  std::string dos_style_path = config_file_path;
  std::replace(dos_style_path.begin(), dos_style_path.end(), '/', '\\');

  std::string json_string;

  {
    std::ifstream config_file(dos_style_path.c_str());
    if (!config_file) {
      errors.push_back(std::string("Missing config file at ") + config_file_path);
      return false;
    }

    config_file.seekg(0, std::ios::end);
    auto end_pos = config_file.tellg();
    config_file.seekg(0, std::ios::beg);
    std::streamsize file_size = end_pos - config_file.tellg();

    if (file_size > MAX_CONFIG_FILE_SIZE || file_size == -1) {
      errors.push_back(std::string("Config file at ") + config_file_path + " is too large.");
      return false;
    }

    json_string.resize(file_size);
    config_file.read(&json_string[0], file_size);
  }

  return LoadConfigBuffer(json_string, errors);
}

static bool LoadBool(json_t const* object, const char* key, bool& out) {
  auto property = json_getProperty(object, key);
  if (!property) {
    return true;
  }

  if (json_getType(property) != JSON_BOOLEAN) {
    return false;
  }

  out = json_getBoolean(property);
  return true;
};

static bool LoadString(json_t const* object, const char* key, std::string& out) {
  auto property = json_getProperty(object, key);
  if (!property) {
    return true;
  }

  if (json_getType(property) != JSON_TEXT) {
    return false;
  }

  out = json_getValue(property);
  return true;
};

static bool LoadUint32(json_t const* object, const char* key, uint32_t& out) {
  auto property = json_getProperty(object, key);
  if (!property) {
    return true;
  }

  if (json_getType(property) != JSON_INTEGER) {
    return false;
  }

  auto value = json_getInteger(property);
  if (value < 0) {
    return false;
  }

  out = static_cast<uint32_t>(value & 0xFFFFFFFF);
  return true;
};

bool RuntimeConfig::LoadConfigBuffer(const std::string& config_content, std::vector<std::string>& errors) {
  std::map<std::string, std::vector<std::string>> test_config;

  const JSONParser parser{config_content.c_str()};

  auto root = parser.root();
  if (!root) {
    errors.emplace_back("Failed to parse config file.");
    return false;
  }

  auto settings = json_getProperty(root, "settings");
  if (!settings) {
    errors.emplace_back("'settings' not found");
    return false;
  }
  if (json_getType(settings) != JSON_OBJ) {
    errors.emplace_back("'settings' not an object");
    return false;
  }

  if (!LoadBool(settings, "enable_progress_log", enable_progress_log_)) {
    errors.emplace_back("settings[enable_progress_log] must be a boolean");
    return false;
  }

  if (!LoadBool(settings, "disable_autorun", disable_autorun_)) {
    errors.emplace_back("settings[disable_autorun] must be a boolean");
    return false;
  }

  if (!LoadBool(settings, "enable_autorun_immediately", enable_autorun_immediately_)) {
    errors.emplace_back("settings[enable_autorun_immediately] must be a boolean");
    return false;
  }

  if (!LoadBool(settings, "enable_shutdown_on_completion", enable_shutdown_on_completion_)) {
    errors.emplace_back("settings[enable_shutdown_on_completion] must be a boolean");
    return false;
  }

  if (!LoadBool(settings, "enable_pgraph_region_diff", enable_pgraph_region_diff_)) {
    errors.emplace_back("settings[enable_pgraph_region_diff] must be a boolean");
    return false;
  }

  if (!LoadBool(settings, "skip_tests_by_default", skip_tests_by_default_)) {
    errors.emplace_back("settings[skip_tests_by_default] must be a boolean");
    return false;
  }

  if (!LoadUint32(settings, "delay_milliseconds_between_tests", delay_milliseconds_between_tests_)) {
    errors.emplace_back("settings[delay_milliseconds_between_tests] must be a positive integer");
    return false;
  }

  if (!LoadString(settings, "output_directory_path", output_directory_path_)) {
    errors.emplace_back("settings[output_directory_path] must be a string");
    return false;
  }
  output_directory_path_ = SanitizePath(output_directory_path_);

  if (!ProcessNetworkSettings(settings, errors)) {
    return false;
  }

  auto test_suites = json_getProperty(root, "test_suites");
  if (!test_suites) {
    return true;
  }

  if (json_getType(test_suites) != JSON_OBJ) {
    errors.emplace_back("'test_suites' not an object");
    return false;
  }

  return ParseTestSuites(test_suites, errors, configured_test_suites_, configured_test_cases_);
}

bool RuntimeConfig::ProcessNetworkSettings(const void* parent, std::vector<std::string>& errors) {
  auto settings = static_cast<json_t const*>(parent);
  auto network = json_getProperty(settings, "network");
  if (!network) {
    return true;
  }

  bool network_enable = false;
  if (!LoadBool(network, "enable", network_enable)) {
    errors.emplace_back("settings[network][config_automatic] must be a boolean");
    return false;
  }

  bool config_automatic = false;
  if (!LoadBool(network, "config_automatic", config_automatic)) {
    errors.emplace_back("settings[network][config_automatic] must be a boolean");
    return false;
  }

  bool config_dhcp = false;
  if (!LoadBool(network, "config_dhcp", config_dhcp)) {
    errors.emplace_back("settings[network][config_dhcp] must be a boolean");
    return false;
  }

  if (!network_enable) {
    network_config_mode_ = NetworkConfigMode::OFF;
  } else if (config_automatic) {
    network_config_mode_ = NetworkConfigMode::AUTOMATIC;
  } else if (config_dhcp) {
    network_config_mode_ = NetworkConfigMode::DHCP;
  } else {
    network_config_mode_ = NetworkConfigMode::STATIC;
  }

  auto load_ip = [&errors](auto object, const char* key, uint32_t& output, const std::string& error_prefix) {
    std::string value;
    if (!LoadString(object, key, value)) {
      errors.emplace_back(error_prefix + " must be a string containing an IPv4 address (e.g., \"1.2.3.4\")");
      return false;
    }
    if (!value.empty()) {
      output = ntohl(inet_addr(value.c_str()));
      if (output == 0xFFFFFFFF) {
        errors.emplace_back(error_prefix + " must be a string containing an IPv4 address (e.g., \"1.2.3.4\")");
        return false;
      }
    }
    return true;
  };

  if (!load_ip(network, "static_ip", static_ip_, "settings[network][static_ip]")) {
    return false;
  }
  if (!load_ip(network, "static_netmask", static_netmask_, "settings[network][static_netmask]")) {
    return false;
  }
  if (!load_ip(network, "static_gateway", static_gateway_, "settings[network][static_gateway]")) {
    return false;
  }
  if (!load_ip(network, "static_dns_1", static_dns_1_, "settings[network][static_dns_1]")) {
    return false;
  }
  if (!load_ip(network, "static_dns_2", static_dns_2_, "settings[network][static_dns_2]")) {
    return false;
  }

  if (auto ftp = json_getProperty(network, "ftp")) {
    if (!load_ip(ftp, "ftp_ip", ftp_server_ip_, "settings[network][ftp][ftp_ip]")) {
      return false;
    }

    uint32_t port = 0;
    if (!LoadUint32(ftp, "ftp_port", port) || port > 0xFFFF) {
      errors.emplace_back("settings[network][ftp][ftp_port] must be a positive 16-bit integer");
      return false;
    }
    ftp_server_port_ = port & 0xFFFF;

    if (!LoadString(ftp, "ftp_user", ftp_user_)) {
      errors.emplace_back("settings[network][ftp][ftp_user] must be a string");
      return false;
    }

    if (!LoadString(ftp, "ftp_password", ftp_password_)) {
      errors.emplace_back("settings[network][ftp][ftp_password] must be a string");
      return false;
    }

    if (!LoadUint32(ftp, "ftp_timeout_milliseconds", ftp_timeout_milliseconds_)) {
      errors.emplace_back("settings[network][ftp][ftp_timeout_milliseconds] must be a positive integer");
      return false;
    }
  }

  return true;
}

static RuntimeConfig::SkipConfiguration MakeSkipConfiguration(bool is_skipped) {
  if (is_skipped) {
    return RuntimeConfig::SkipConfiguration::SKIPPED;
  }
  return RuntimeConfig::SkipConfiguration::UNSKIPPED;
}

static bool ParseTestCase(json_t const* test_case, const std::string& suite_name, const std::string& test_name,
                          std::vector<std::string>& errors, RuntimeConfig::SkipConfiguration& config_value,
                          const std::string& test_case_error_message_prefix) {
  for (auto element = json_getChild(test_case); element; element = json_getSibling(element)) {
    std::string name = json_getName(element);
    auto type = json_getType(element);

    if (name != "skipped") {
      errors.emplace_back(test_case_error_message_prefix + "[" + name + "] unsupported. Ignoring");
      continue;
    }

    if (type == JSON_BOOLEAN) {
      config_value = MakeSkipConfiguration(json_getBoolean(element));
    } else {
      errors.emplace_back(test_case_error_message_prefix + "[skipped] must be a boolean");
      return false;
    }
  }

  return true;
}

static bool ParseTestCases(
    json_t const* test_suite, const std::string& suite_name, std::vector<std::string>& errors,
    std::map<std::string, RuntimeConfig::SkipConfiguration>& configured_suites,
    std::map<std::string, std::map<std::string, RuntimeConfig::SkipConfiguration>>& configured_cases,
    const std::string& suite_error_message_prefix) {
  std::map<std::string, RuntimeConfig::SkipConfiguration> case_settings;

  for (auto test_or_skipped = json_getChild(test_suite); test_or_skipped;
       test_or_skipped = json_getSibling(test_or_skipped)) {
    std::string test_name = json_getName(test_or_skipped);
    auto type = json_getType(test_or_skipped);

    if (test_name == "skipped") {
      if (type == JSON_BOOLEAN) {
        configured_suites[suite_name] = MakeSkipConfiguration(json_getBoolean(test_or_skipped));
        continue;
      }

      errors.emplace_back(suite_error_message_prefix + "[skipped] must be a boolean.");
      return false;
    }

    if (type == JSON_OBJ) {
      auto config_value = RuntimeConfig::SkipConfiguration::DEFAULT;
      if (!ParseTestCase(test_or_skipped, suite_name, test_name, errors, config_value,
                         suite_error_message_prefix + "[" + test_name + "]")) {
        return false;
      }
      if (config_value != RuntimeConfig::SkipConfiguration::DEFAULT) {
        case_settings[test_name] = config_value;
      }
    } else {
      errors.emplace_back(suite_error_message_prefix + "[" + test_name + "] must be an object. Ignoring");
      continue;
    }
  }

  if (!case_settings.empty()) {
    configured_cases[suite_name] = case_settings;
  }

  return true;
}

static bool ParseTestSuites(
    json_t const* test_suites, std::vector<std::string>& errors,
    std::map<std::string, RuntimeConfig::SkipConfiguration>& skipped_test_suites,
    std::map<std::string, std::map<std::string, RuntimeConfig::SkipConfiguration>>& skipped_test_cases) {
  std::string test_suites_error_message_prefix("test_suites[");
  for (auto suite = json_getChild(test_suites); suite; suite = json_getSibling(suite)) {
    std::string suite_name = json_getName(suite);
    auto suite_error_message_prefix = test_suites_error_message_prefix + suite_name + "]";

    if (json_getType(suite) != JSON_OBJ) {
      errors.emplace_back(suite_error_message_prefix + " must be an object. Ignoring");
      continue;
    }

    if (!ParseTestCases(suite, suite_name, errors, skipped_test_suites, skipped_test_cases,
                        suite_error_message_prefix)) {
      return false;
    }
  }

  return true;
}

bool RuntimeConfig::ApplyConfig(std::vector<std::shared_ptr<TestSuite>>& test_suites,
                                std::vector<std::string>& errors) {
  std::vector<std::shared_ptr<TestSuite>> filtered_test_suites;

  for (auto& suite : test_suites) {
    auto default_skip_test_case = skip_tests_by_default_;

    auto entry = configured_test_suites_.find(suite->Name());
    if (entry != configured_test_suites_.end()) {
      switch (entry->second) {
        case SkipConfiguration::SKIPPED:
          default_skip_test_case = true;
          break;
        case SkipConfiguration::UNSKIPPED:
          default_skip_test_case = false;
          break;
        default:
          break;
      }
    }

    auto test_case_config = configured_test_cases_.find(suite->Name());
    std::set<std::string> skipped_test_cases;

    for (auto& test_case : suite->TestNames()) {
      bool skip_test_case = default_skip_test_case;

      if (test_case_config != configured_test_cases_.end()) {
        auto explicit_config = test_case_config->second.find(test_case);
        if (explicit_config != test_case_config->second.end()) {
          skip_test_case = explicit_config->second == SkipConfiguration::SKIPPED;
        }
      }

      if (skip_test_case) {
        skipped_test_cases.insert(test_case);
      }
    }

    if (!skipped_test_cases.empty()) {
      suite->DisableTests(skipped_test_cases);
    }

    if (suite->HasEnabledTests()) {
      filtered_test_suites.push_back(suite);
    }
  }

  test_suites = filtered_test_suites;

  return true;
}

#ifdef DUMP_CONFIG_FILE
static std::string bool_str(bool value) { return value ? "true" : "false"; }

bool RuntimeConfig::DumpConfig(const std::string& config_file_path,
                               const std::vector<std::shared_ptr<TestSuite>>& test_suites,
                               std::vector<std::string>& errors) {
  std::ofstream config_file(config_file_path);
  if (!config_file) {
    errors.emplace_back("Failed to open config file for output");
    return false;
  }

  return DumpConfigToStream(config_file, test_suites, errors);
}

bool RuntimeConfig::DumpConfigToStream(std::ostream& config_file,
                                       const std::vector<std::shared_ptr<TestSuite>>& test_suites,
                                       std::vector<std::string>& errors) {
  config_file << "{" << std::endl;

  // General settings.

  WriteSettings(config_file);
  config_file << "," << std::endl;

  // Test suite enable/disable status.

  config_file << "  \"test_suites\": {" << std::endl;

  bool add_test_suite_comma = false;
  for (auto& suite : test_suites) {
    if (add_test_suite_comma) {
      config_file << "," << std::endl;
    }

    WriteTestSuite(config_file, suite);
    add_test_suite_comma = true;
  }

  config_file << std::endl;
  config_file << "  }" << std::endl;  // test_suites

  config_file << "}" << std::endl;

  return true;
}

void RuntimeConfig::WriteSettings(std::ostream& output) const {
  output << "  \"settings\": {" << std::endl;

#define BOOL_OPT(VALUE) output << "    \"" #VALUE "\": " << bool_str(VALUE())

  BOOL_OPT(enable_progress_log) << "," << std::endl;
  BOOL_OPT(disable_autorun) << "," << std::endl;
  BOOL_OPT(enable_autorun_immediately) << "," << std::endl;
  BOOL_OPT(enable_shutdown_on_completion) << "," << std::endl;
  BOOL_OPT(enable_pgraph_region_diff) << "," << std::endl;
  BOOL_OPT(skip_tests_by_default) << "," << std::endl;
#undef BOOL_OPT

  output << R"(    "delay_milliseconds_between_tests": )" << delay_milliseconds_between_tests_ << "," << std::endl;

  output << R"(    "network": {)" << std::endl;
  output << R"(      "enable": )" << bool_str(network_config_mode_ != NetworkConfigMode::OFF) << "," << std::endl;
  output << R"(      "config_automatic": )" << bool_str(network_config_mode_ == NetworkConfigMode::AUTOMATIC) << ","
         << std::endl;
  output << R"(      "config_dhcp": )" << bool_str(network_config_mode_ == NetworkConfigMode::DHCP) << "," << std::endl;

#define IP_VALUE(MEMBER)                                                                   \
  if ((MEMBER)) {                                                                          \
    auto network_ordered = htonl((MEMBER));                                                \
    output << (network_ordered & 0xFF) << "." << ((network_ordered >> 8) & 0xFF) << "."    \
           << ((network_ordered >> 16) & 0xFF) << "." << ((network_ordered >> 24) & 0xFF); \
  }

  output << R"(      "static_ip": ")";
  IP_VALUE(static_ip_)
  output << R"(",)" << std::endl;
  output << R"(      "static_netmask": ")";
  IP_VALUE(static_netmask_)
  output << R"(",)" << std::endl;
  output << R"(      "static_gateway": ")";
  IP_VALUE(static_gateway_)
  output << R"(",)" << std::endl;
  output << R"(      "static_dns_1": ")";
  IP_VALUE(static_dns_1_)
  output << R"(",)" << std::endl;
  output << R"(      "static_dns_2": ")";
  IP_VALUE(static_dns_2_)
  output << R"(",)" << std::endl;

  output << R"(      "ftp": {)" << std::endl;
  output << R"(        "ftp_ip": ")";
  IP_VALUE(ftp_server_ip_)
  output << R"(",)" << std::endl;

#undef IP_VALUE

  output << R"(        "ftp_port": )" << ftp_server_port_ << "," << std::endl;
  output << R"(        "ftp_user": ")" << ftp_user_ << "\"," << std::endl;
  output << R"(        "ftp_password": ")" << ftp_password_ << "\"," << std::endl;
  output << R"(        "ftp_timeout_milliseconds": )" << ftp_timeout_milliseconds_ << std::endl;
  output << R"(      })" << std::endl;
  output << R"(    },)" << std::endl;

  output << R"(    "output_directory_path": ")" << EscapePath(output_directory_path_) << "\"" << std::endl;

  output << "  }";
}

void RuntimeConfig::WriteTestSuite(std::ostream& output, const std::shared_ptr<TestSuite>& suite) const {
  auto& suite_name = suite->Name();
  output << "    \"" << suite_name << "\": {" << std::endl;
  bool add_test_comma = false;
  auto suite_config = configured_test_suites_.find(suite_name);
  if (suite_config != configured_test_suites_.end()) {
    output << "      \"skipped\": " << (suite_config->second == SkipConfiguration::SKIPPED ? "true" : "false");
    add_test_comma = true;
  }

  for (auto& test_name : suite->TestNames()) {
    if (add_test_comma) {
      output << "," << std::endl;
    }

    WriteTestCase(output, suite_name, test_name);

    add_test_comma = true;
  }
  output << std::endl;

  output << "    }";
}

void RuntimeConfig::WriteTestCase(std::ostream& output, const std::string& suite_name,
                                  const std::string& test_name) const {
  output << "      \"" << test_name << "\": {" << std::endl;
  auto suite_cases_config = configured_test_cases_.find(suite_name);
  if (suite_cases_config != configured_test_cases_.end()) {
    auto case_config = suite_cases_config->second.find(test_name);
    if (case_config != suite_cases_config->second.end()) {
      output << "        \"skipped\": " << (case_config->second == SkipConfiguration::SKIPPED ? "true" : "false")
             << std::endl;
    }
  }

  output << "      }";
}

std::string RuntimeConfig::EscapePath(const std::string& path) {
  std::string escaped(path);
  std::replace(escaped.begin(), escaped.end(), '\\', '/');
  return std::move(escaped);
}

#endif  // DUMP_CONFIG_FILE

std::string RuntimeConfig::SanitizePath(const std::string& path) {
  std::string sanitized(path);
  std::replace(sanitized.begin(), sanitized.end(), '/', '\\');
  return std::move(sanitized);
}
