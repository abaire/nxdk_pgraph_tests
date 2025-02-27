#ifndef FTPLOGGER_H
#define FTPLOGGER_H

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>
#include <vector>

extern "C" struct FTPClient;

//! Handles sending log artifacts to an FTP server.
class FTPLogger {
  //! The number of times the logger will try to reconnect to the FTP server
  // before bailing.
  static constexpr uint32_t kDefaultReconnectRetries = 4;
  static constexpr uint32_t kDefaultTimeoutMilliseconds = 250;

 public:
  FTPLogger() = delete;
  FTPLogger(uint32_t server_ip_host_ordered, uint16_t server_port_host_ordered, const std::string& username,
            const std::string& password, uint32_t timeout_milliseconds)
      : FTPLogger(server_ip_host_ordered, server_port_host_ordered, username, password, timeout_milliseconds,
                  kDefaultReconnectRetries) {}
  FTPLogger(uint32_t server_ip_host_ordered, uint16_t ftp_server_port_host_ordered, std::string username,
            std::string password, uint32_t timeout_milliseconds, uint32_t reconnect_retries)
      : ftp_server_ip_{server_ip_host_ordered},
        ftp_server_port_{ftp_server_port_host_ordered},
        ftp_user_{std::move(username)},
        ftp_password_{std::move(password)},
        ftp_timeout_milliseconds_{timeout_milliseconds},
        reconnect_retries_{reconnect_retries} {};

  ~FTPLogger() { Disconnect(); }

  bool Connect();
  bool Disconnect();

  bool IsConnected() const;

  //! Truncate the remote file with the given name and set its content.
  bool WriteFile(const std::string& filename, const std::string& content);
  //! Append the given content to the remote file with the given name.
  bool AppendFile(const std::string& filename, const std::string& content);

  bool PutFile(const std::string& local_filename, const std::string& remote_filename = "");

  void QueuePutFile(const std::string& local_filename, const std::string& remote_filename = "") {
    send_file_queue_.emplace_back(local_filename, remote_filename);
  }

  //! (Local path, Remote path) pairs that have been queued for sending.
  const std::vector<std::pair<std::string, std::string>> send_file_queue() const { return send_file_queue_; }

  void ClearSendQueue() { send_file_queue_.clear(); }

  const std::vector<std::string>& error_log() const { return error_log_; }

 private:
  void ClearLog();
  void LogError(std::string message);

 private:
  uint32_t ftp_server_ip_;
  uint16_t ftp_server_port_;
  std::string ftp_user_;
  std::string ftp_password_;
  uint32_t ftp_timeout_milliseconds_;
  uint32_t reconnect_retries_;

  FTPClient* ftp_client_{nullptr};

  std::vector<std::string> error_log_;
  std::vector<std::pair<std::string, std::string>> send_file_queue_;
};

#endif  // FTPLOGGER_H
