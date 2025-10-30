#ifndef NXDK_PGRAPH_TESTS_SRC_FILE_UTIL_H_
#define NXDK_PGRAPH_TESTS_SRC_FILE_UTIL_H_

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmacro-redefined"
#include <windows.h>
#pragma clang diagnostic pop

/**
 * Copies a file to a new location, discarding original file metadata.
 *
 * This is primarily useful when working around issues copying from a DVD-ROM to
 * the cache drive. The nxdk implementation of CopyFileA seems to fail in that
 * case.
 */
BOOL InstallCacheFile(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, BOOL bFailIfExists);

#endif  // NXDK_PGRAPH_TESTS_SRC_FILE_UTIL_H_
