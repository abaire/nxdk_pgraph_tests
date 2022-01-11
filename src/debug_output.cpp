#include "debug_output.h"

#include <hal/debug.h>
#include <pbkit/pbkit.h>
#include <windows.h>

#include <cstdio>

extern "C" {
void _putchar(char character) { putchar(character); }
}

void PrintAssertAndWaitForever(const char *assert_code, const char *filename, uint32_t line) {
  DbgPrint("ASSERT FAILED: '%s' at %s:%d\n", assert_code, filename, line);
  debugPrint("ASSERT FAILED!\n-=[\n\n%s\n\n]=-\nat %s:%d\n", assert_code, filename, line);
  debugPrint("\nHalted, please reboot.\n");
  pb_show_debug_screen();
  while (1) {
    Sleep(2000);
  }
}
