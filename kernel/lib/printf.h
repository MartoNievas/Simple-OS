#ifndef MIKERNEL_LIB_PRINTF_H
#define MIKERNEL_LIB_PRINTF_H

#include <stdarg.h>

void kputc(char c);
void kvprintf(const char *fmt, va_list args);
void kprintf(const char *fmt, ...);

#endif
