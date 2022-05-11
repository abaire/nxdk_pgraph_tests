#include "logger.h"

#include "debug_output.h"

Logger* Logger::singleton_ = nullptr;

Logger::Logger(const std::string& log_path, bool truncate_log) {
  const char* p = log_path.c_str();
  PrintMsg("Opening log file at %s\n", p);
  auto open_mode = truncate_log ? std::ios_base::trunc : std::ios_base::app;
  log_file_ = std::ofstream(log_path, open_mode);

  ASSERT(log_file_ && "Failed to open log file for output");
}

void Logger::Initialize(const std::string& log_path, bool truncate_log) {
  ASSERT(!singleton_ && "Invalid attempt to initialize logger twice.");

  singleton_ = new Logger(log_path, truncate_log);
}

std::ofstream& Logger::Log() {
  ASSERT(singleton_ && "Attempt to use Logger before Initialize");
  return singleton_->log_file_;
}
