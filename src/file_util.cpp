#include "file_util.h"

#include "debug_output.h"

static NTSTATUS DeleteHandle(HANDLE handle) {
  IO_STATUS_BLOCK ioStatusBlock;
  FILE_DISPOSITION_INFORMATION dispositionInformation;
  dispositionInformation.DeleteFile = TRUE;

  return NtSetInformationFile(handle, &ioStatusBlock, &dispositionInformation, sizeof(dispositionInformation),
                              FileDispositionInformation);
}

BOOL InstallCacheFile(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, BOOL bFailIfExists) {
  NTSTATUS status;
  HANDLE sourceHandle;
  HANDLE targetHandle = INVALID_HANDLE_VALUE;
  ANSI_STRING targetPath;
  OBJECT_ATTRIBUTES objectAttributes;
  IO_STATUS_BLOCK ioStatusBlock;
  FILE_NETWORK_OPEN_INFORMATION networkOpenInformation;
  LPVOID readBuffer = nullptr;
  SIZE_T readBufferRegionSize = 64 * 1024;
  DWORD bytesRead;

  sourceHandle = CreateFile(lpExistingFileName, FILE_GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
  if (sourceHandle == INVALID_HANDLE_VALUE) {
    return FALSE;
  }

  status = NtQueryInformationFile(sourceHandle, &ioStatusBlock, &networkOpenInformation, sizeof(networkOpenInformation),
                                  FileNetworkOpenInformation);
  if (!NT_SUCCESS(status)) {
    NtClose(sourceHandle);
    SetLastError(RtlNtStatusToDosError(status));
    return FALSE;
  }

  RtlInitAnsiString(&targetPath, lpNewFileName);
  InitializeObjectAttributes(&objectAttributes, &targetPath, OBJ_CASE_INSENSITIVE, ObDosDevicesDirectory(), nullptr);

  status = NtCreateFile(
      &targetHandle, FILE_GENERIC_WRITE, &objectAttributes, &ioStatusBlock, &networkOpenInformation.AllocationSize,
      networkOpenInformation.FileAttributes & ~FILE_ATTRIBUTE_READONLY, 0, bFailIfExists ? FILE_CREATE : FILE_SUPERSEDE,
      FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE | FILE_SEQUENTIAL_ONLY);
  if (!NT_SUCCESS(status)) {
    NtClose(sourceHandle);
    SetLastError(RtlNtStatusToDosError(status));
    return FALSE;
  }

  status = NtAllocateVirtualMemory(&readBuffer, 0, &readBufferRegionSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (!NT_SUCCESS(status)) {
    NtClose(sourceHandle);
    NtClose(targetHandle);
    SetLastError(RtlNtStatusToDosError(status));
    return FALSE;
  }

  while (TRUE) {
    auto bufferPos = static_cast<const BYTE *>(readBuffer);
    if (!ReadFile(sourceHandle, readBuffer, readBufferRegionSize, &bytesRead, nullptr)) {
      NtFreeVirtualMemory(&readBuffer, &readBufferRegionSize, MEM_RELEASE);
      DeleteHandle(targetHandle);
      NtClose(sourceHandle);
      NtClose(targetHandle);
      return FALSE;
    }

    if (!bytesRead) {
      break;
    }

    while (bytesRead > 0) {
      DWORD bytesWritten = 0;
      if (!WriteFile(targetHandle, bufferPos, bytesRead, &bytesWritten, nullptr)) {
        NtFreeVirtualMemory(&readBuffer, &readBufferRegionSize, MEM_RELEASE);
        DeleteHandle(targetHandle);
        NtClose(sourceHandle);
        NtClose(targetHandle);
        return FALSE;
      }
      bytesRead -= bytesWritten;
      bufferPos += bytesWritten;
    }
  }

  status = NtFreeVirtualMemory(&readBuffer, &readBufferRegionSize, MEM_RELEASE);
  ASSERT(NT_SUCCESS(status));

  status = NtClose(sourceHandle);
  if (!NT_SUCCESS(status)) {
    SetLastError(RtlNtStatusToDosError(status));
    NtClose(targetHandle);
    return FALSE;
  }

  status = NtClose(targetHandle);
  if (!NT_SUCCESS(status)) {
    SetLastError(RtlNtStatusToDosError(status));
    return FALSE;
  }
  return TRUE;
}
