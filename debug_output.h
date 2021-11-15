#ifndef NXDK_PGRAPH_TESTS_DEBUG_OUTPUT_H
#define NXDK_PGRAPH_TESTS_DEBUG_OUTPUT_H

#include "printf/printf.h"

template <typename... VarArgs>
inline void PrintMsg(const char *fmt, VarArgs &&...args) {
  int string_length = snprintf_(nullptr, 0, fmt, args...);
  std::string buf;
  buf.resize(string_length);

  snprintf_(&buf[0], string_length + 1, fmt, args...);
  DbgPrint("%s", buf.c_str());
}

#endif  // NXDK_PGRAPH_TESTS_DEBUG_OUTPUT_H
