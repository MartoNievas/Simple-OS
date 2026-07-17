#include <stdarg.h>
#include <stdint.h>

#include "../arch/i386/serial.h"
#include "../arch/i386/vga.h"
#include "printf.h"

#define NUMBER_BUF_SIZE 12 // enough for a 32-bit value in base 10 or 16, plus sign

void kputc(char c) {
  vga_write_char(c);
  serial_write_char(c);
}

static void kputs_raw(const char *s) {
  while (*s) {
    kputc(*s++);
  }
}

static void print_unsigned(uint32_t value, unsigned base, int uppercase) {
  static const char digits_lower[] = "0123456789abcdef";
  static const char digits_upper[] = "0123456789ABCDEF";
  const char *digits = uppercase ? digits_upper : digits_lower;

  char buf[NUMBER_BUF_SIZE];
  int i = 0;

  if (value == 0) {
    kputc('0');
    return;
  }

  while (value > 0 && i < NUMBER_BUF_SIZE) {
    buf[i++] = digits[value % base];
    value /= base;
  }

  while (i > 0) {
    kputc(buf[--i]);
  }
}

static void print_signed(int32_t value) {
  if (value < 0) {
    kputc('-');
    // value + 1 first avoids negating INT32_MIN, which overflows a signed int
    print_unsigned((uint32_t)(-(value + 1)) + 1u, 10, 0);
  } else {
    print_unsigned((uint32_t)value, 10, 0);
  }
}

void kvprintf(const char *fmt, va_list args) {
  for (const char *p = fmt; *p != '\0'; p++) {
    if (*p != '%') {
      kputc(*p);
      continue;
    }

    p++;
    switch (*p) {
      case 's': {
        const char *s = va_arg(args, const char *);
        kputs_raw(s ? s : "(null)");
        break;
      }
      case 'c':
        kputc((char)va_arg(args, int));
        break;
      case 'd':
      case 'i':
        print_signed(va_arg(args, int));
        break;
      case 'u':
        print_unsigned(va_arg(args, uint32_t), 10, 0);
        break;
      case 'x':
        print_unsigned(va_arg(args, uint32_t), 16, 0);
        break;
      case 'X':
        print_unsigned(va_arg(args, uint32_t), 16, 1);
        break;
      case 'p':
        kputs_raw("0x");
        print_unsigned((uint32_t)(uintptr_t)va_arg(args, void *), 16, 0);
        break;
      case '%':
        kputc('%');
        break;
      case '\0':
        return;
      default:
        kputc('%');
        kputc(*p);
        break;
    }
  }
}

void kprintf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  kvprintf(fmt, args);
  va_end(args);
}
