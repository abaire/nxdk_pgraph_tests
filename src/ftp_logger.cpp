#include "ftp_logger.h"

#include "ftp_client.h"

static constexpr uint32_t kConnectTimeoutMilliseconds = 100;
static constexpr uint32_t kTransferProcessTimeoutMilliseconds = 30;

static FTPClientProcessStatus ProcessLoop(FTPClient* context, uint32_t timeout_milliseconds) {
  auto status = FTP_CLIENT_PROCESS_STATUS_SUCCESS;
  do {
    status = FTPClientProcess(context, timeout_milliseconds);
  } while (status == FTP_CLIENT_PROCESS_STATUS_SUCCESS ||
           (status == FTP_CLIENT_PROCESS_STATUS_TIMEOUT && FTPClientHasSendPending(context)));
  return status;
}

bool FTPLogger::IsConnected() const { return FTPClientIsFullyConnected(ftp_client_); }

bool FTPLogger::Connect() {
  if (IsConnected()) {
    return true;
  }

  ClearLog();

  for (auto i = 0; i < reconnect_retries_; ++i) {
    if (!ftp_client_) {
      auto status =
          FTPClientInit(&ftp_client_, ftp_server_ip_, ftp_server_port_, ftp_user_.c_str(), ftp_password_.c_str());
      if (status != FTP_CLIENT_INIT_STATUS_SUCCESS) {
        char error_message[64] = {0};
        snprintf(error_message, sizeof(error_message) - 1, "Init failed %d %d", status, FTPClientErrno(ftp_client_));
        LogError(error_message);
        FTPClientDestroy(&ftp_client_);
        continue;
      }
    }

    {
      FTPClientConnectStatus status = FTPClientConnect(ftp_client_, ftp_timeout_milliseconds_);
      if (status != FTP_CLIENT_CONNECT_STATUS_SUCCESS) {
        char error_message[64] = {0};
        snprintf(error_message, sizeof(error_message) - 1, "Connect failed %d %d", status, FTPClientErrno(ftp_client_));
        LogError(error_message);
        FTPClientDestroy(&ftp_client_);
        continue;
      }
    }

    FTPClientProcessStatus status = ProcessLoop(ftp_client_, kConnectTimeoutMilliseconds);
    if (FTPClientProcessStatusIsError(status)) {
      char error_message[64] = {0};
      snprintf(error_message, sizeof(error_message) - 1, "Auth process failed %d %d", status,
               FTPClientErrno(ftp_client_));
      LogError(error_message);
      FTPClientDestroy(&ftp_client_);
      continue;
    }
  }

  return IsConnected();
}

bool FTPLogger::Disconnect() {
  FTPClientDestroy(&ftp_client_);
  return true;
}

void FTPLogger::ClearLog() { error_log_.clear(); }

void FTPLogger::LogError(std::string message) { error_log_.emplace_back(std::move(message)); }

static void OnCompleted(bool success, void* userdata) { *static_cast<bool*>(userdata) = success; }

bool FTPLogger::WriteFile(const std::string& filename, const std::string& content) {
  if (!IsConnected()) return false;

  bool file_sent = false;
  if (!FTPClientCopyAndSendBuffer(ftp_client_, filename.c_str(), content.c_str(), content.size(), OnCompleted,
                                  &file_sent)) {
    char error_message[64] = {0};
    snprintf(error_message, sizeof(error_message) - 1, "CopyAndSendBuffer failed %d", FTPClientErrno(ftp_client_));
    LogError(error_message);
    return false;
  }

  FTPClientProcessStatus status = ProcessLoop(ftp_client_, kTransferProcessTimeoutMilliseconds);
  if (FTPClientProcessStatusIsError(status)) {
    char error_message[64] = {0};
    snprintf(error_message, sizeof(error_message) - 1, "CopyAndSendBuffer failed %d %d", status,
             FTPClientErrno(ftp_client_));
    LogError(error_message);
    FTPClientDestroy(&ftp_client_);
    return false;
  }

  if (!file_sent) {
    LogError("CopyAndSendBuffer - send failed");
  }
  return file_sent;
}

bool FTPLogger::AppendFile(const std::string& filename, const std::string& content) {
  if (!IsConnected()) return false;

  bool file_sent = false;
  if (!FTPClientCopyAndAppendBuffer(ftp_client_, filename.c_str(), content.c_str(), content.size(), OnCompleted,
                                    &file_sent)) {
    char error_message[64] = {0};
    snprintf(error_message, sizeof(error_message) - 1, "CopyAndSendBuffer failed %d", FTPClientErrno(ftp_client_));
    LogError(error_message);
    return false;
  }

  FTPClientProcessStatus status = ProcessLoop(ftp_client_, kTransferProcessTimeoutMilliseconds);
  if (FTPClientProcessStatusIsError(status)) {
    char error_message[64] = {0};
    snprintf(error_message, sizeof(error_message) - 1, "CopyAndSendBuffer failed %d %d", status,
             FTPClientErrno(ftp_client_));
    LogError(error_message);
    FTPClientDestroy(&ftp_client_);
    return false;
  }

  if (!file_sent) {
    LogError("CopyAndSendBuffer - send failed");
  }
  return file_sent;
}

bool FTPLogger::PutFile(const std::string& local_filename, const std::string& remote_filename) {
  if (!IsConnected()) return false;

  bool file_sent = false;
  if (!FTPClientSendFile(ftp_client_, local_filename.c_str(),
                         remote_filename.empty() ? nullptr : remote_filename.c_str(), OnCompleted, &file_sent)) {
    char error_message[64] = {0};
    snprintf(error_message, sizeof(error_message) - 1, "SendFile failed %d", FTPClientErrno(ftp_client_));
    LogError(error_message);
    return false;
  }

  FTPClientProcessStatus status = ProcessLoop(ftp_client_, kTransferProcessTimeoutMilliseconds);
  if (FTPClientProcessStatusIsError(status)) {
    char error_message[64] = {0};
    snprintf(error_message, sizeof(error_message) - 1, "SendFile failed %d %d", status, FTPClientErrno(ftp_client_));
    LogError(error_message);
    FTPClientDestroy(&ftp_client_);
    return false;
  }

  if (!file_sent) {
    LogError("SendFile - send failed");
  }

  return true;
}
