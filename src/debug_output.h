#ifndef NXDK_PGRAPH_TESTS_DEBUG_OUTPUT_H
#define NXDK_PGRAPH_TESTS_DEBUG_OUTPUT_H

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmacro-redefined"
#include <windows.h>
#pragma clang diagnostic pop

#include <string>

#include "printf/printf.h"

#define ASSERT(c)                                      \
  if (!(c)) {                                          \
    PrintAssertAndWaitForever(#c, __FILE__, __LINE__); \
  }

template <typename... VarArgs>
inline void PrintMsg(const char *fmt, VarArgs &&...args) {
  int string_length = snprintf_(nullptr, 0, fmt, args...);
  std::string buf;
  buf.resize(string_length);

  snprintf_(&buf[0], string_length + 1, fmt, args...);
  DbgPrint("%s", buf.c_str());
}

[[noreturn]] void PrintAssertAndWaitForever(const char *assert_code, const char *filename, uint32_t line);

#endif  // NXDK_PGRAPH_TESTS_DEBUG_OUTPUT_H
