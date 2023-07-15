#include "debug_output.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#include <hal/debug.h>
#pragma clang diagnostic pop

#include <pbkit/pbkit.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmacro-redefined"
#include <windows.h>
#pragma clang diagnostic pop

#include <cstdio>

extern "C" {
void _putchar(char character) { putchar(character); }
}

[[noreturn]] void PrintAssertAndWaitForever(const char *assert_code, const char *filename, uint32_t line) {
  DbgPrint("ASSERT FAILED: '%s' at %s:%d\n", assert_code, filename, line);
  debugPrint("ASSERT FAILED!\n-=[\n\n%s\n\n]=-\nat %s:%d\n", assert_code, filename, line);
  debugPrint("\nHalted, please reboot.\n");
  pb_show_debug_screen();
  while (true) {
    Sleep(2000);
  }
}
