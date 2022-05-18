#ifndef NXDK_PGRAPH_TESTS_PGRAPH_DIFF_TOKEN_H
#define NXDK_PGRAPH_TESTS_PGRAPH_DIFF_TOKEN_H

#include "pbkit_ext.h"

struct PGRAPHDiffToken {
  uint8_t registers[PGRAPH_REGISTER_ARRAY_SIZE];

  explicit PGRAPHDiffToken(bool initialize = true);
  void Capture();
  void DumpDiff() const;
};

#endif  // NXDK_PGRAPH_TESTS_PGRAPH_DIFF_TOKEN_H
