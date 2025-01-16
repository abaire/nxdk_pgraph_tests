#include "pgraph_diff_token.h"

#include <set>
#include <vector>

#include "configure.h"
#include "debug_output.h"
#include "logger.h"

// This list was compiled by comparing the raw diff across a NV097_SET_SHADOW_COMPARE_FUNC to a diff across a
// NV097_SET_MATERIAL_ALPHA. Addresses that were modified in both are assumed to be utility registers without any
// particularly interesting meaning.
static std::set<uint32_t> kDiffBlacklist{
    0xFD40000C, 0xFD40010C, 0xFD400704, 0xFD400708, 0xFD40070C, 0xFD40072C, 0xFD400740, 0xFD400744,
    0xFD400748, 0xFD40074C,
    0xFD400750,  // Value changes even when no pgraph commands are set
    0xFD400760, 0xFD400764, 0xFD400768, 0xFD40076C, 0xFD400788, 0xFD4007A0, 0xFD4007A4, 0xFD4007A8,
    0xFD4007AC, 0xFD4007B0, 0xFD4007B4, 0xFD4007B8, 0xFD4007BC, 0xFD4007E0, 0xFD4007E4, 0xFD4007E8,
    0xFD4007EC, 0xFD4007F0, 0xFD4007F4, 0xFD4007F8, 0xFD4007FC, 0xFD40110C, 0xFD401704, 0xFD401708,
    0xFD40170C, 0xFD40172C, 0xFD401740, 0xFD401744, 0xFD401748, 0xFD40174C,
    0xFD401750,  // Value changes even when no pgraph commands are set
    0xFD401760, 0xFD401764, 0xFD401768, 0xFD40176C, 0xFD401788, 0xFD4017A0, 0xFD4017A4, 0xFD4017A8,
    0xFD4017AC, 0xFD4017B0, 0xFD4017B4, 0xFD4017B8, 0xFD4017BC, 0xFD4017E0, 0xFD4017E4, 0xFD4017E8,
    0xFD4017EC, 0xFD4017F0, 0xFD4017F4, 0xFD4017F8, 0xFD4017FC,
};

PGRAPHDiffToken::PGRAPHDiffToken(bool initialize, bool enable_progress_log)
    : registers{0}, enable_progress_log{enable_progress_log} {
  if (initialize) {
    Capture();
  }
}

void PGRAPHDiffToken::Capture() { pb_fetch_pgraph_registers(registers); }

void PGRAPHDiffToken::DumpDiff() const {
  std::vector<uint8_t> new_registers;
  new_registers.resize(PGRAPH_REGISTER_ARRAY_SIZE);
  pb_fetch_pgraph_registers(new_registers.data());

  std::list<uint32_t> modified_registers;
  pb_diff_registers(registers, new_registers.data(), modified_registers);

  auto old_vals = reinterpret_cast<const uint32_t*>(registers);
  auto new_vals = reinterpret_cast<const uint32_t*>(new_registers.data());

  for (auto addr : modified_registers) {
    //    if (kDiffBlacklist.find(addr) != kDiffBlacklist.end()) {
    //      continue;
    //    }
    const auto offset = (addr - PGRAPH_REGISTER_BASE) / 4;
    PrintMsg("0x%08X: 0x%08X => 0x%08X\n", addr, old_vals[offset], new_vals[offset]);

    if (enable_progress_log) {
      Logger::Log() << std::setw(8) << std::hex << "0x" << addr << ": 0x" << old_vals[offset] << " => 0x"
                    << new_vals[offset] << std::endl;
    }
  }
}
