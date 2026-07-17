#include <stdarg.h>

#include "panic.h"
#include "printf.h"

void panic_impl(const char *file, int line, const char *fmt, ...) {
  kprintf("\nKERNEL PANIC at %s:%d: ", file, line);

  va_list args;
  va_start(args, fmt);
  kvprintf(fmt, args);
  va_end(args);

  kputc('\n');

  __asm__ volatile("cli");
  for (;;) {
    __asm__ volatile("hlt");
  }
}
