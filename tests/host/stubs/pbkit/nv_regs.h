#ifndef NV_REGS_H
#define NV_REGS_H

#include "nxdk/lib/pbkit/nv_objects.h"
#include "nxdk/lib/pbkit/nv_regs.h"
#include "xbox_types.h"

#define NEXT_SUBCH 5

inline uint32_t *pb_begin() { return 0; }
inline uint32_t *pb_push1(uint32_t *p, DWORD command, DWORD param1) { return p; }
inline uint32_t *pb_push3(uint32_t *p, DWORD command, DWORD param1, DWORD param2, DWORD param3) { return p; }
inline uint32_t *pb_push4(uint32_t *p, DWORD command, DWORD param1, DWORD param2, DWORD param3, DWORD param4) {
  return p;
}
inline uint32_t *pb_push4f(uint32_t *p, DWORD command, float param1, float param2, float param3, float param4) {
  return p;
}
inline void pb_end(uint32_t *pEnd) {};
inline int pb_busy() { return 0; }

#endif  // NV_REGS_H
