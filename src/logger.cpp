#include "logger.h"

#include "debug_output.h"

Logger* Logger::singleton_ = nullptr;

Logger::Logger(const std::string& log_path, bool truncate_log) : log_path_(log_path) {
  const char* p = log_path.c_str();
  PrintMsg("Opening log file at %s\n", p);

  if (truncate_log) {
    auto log_file = std::ofstream(log_path, std::ios_base::trunc);
    ASSERT(log_file && "Failed to open log file for output");
  }
}

void Logger::Initialize(const std::string& log_path, bool truncate_log) {
  ASSERT(!singleton_ && "Invalid attempt to initialize logger twice.");

  singleton_ = new Logger(log_path, truncate_log);
}

std::ofstream Logger::Log() {
  ASSERT(singleton_ && "Attempt to use Logger before Initialize");
  auto log_file = std::ofstream(singleton_->log_path_, std::ios_base::app);
  ASSERT(log_file && "Failed to open log file for output");
  return log_file;
}
