#ifndef MIKERNEL_LIB_PANIC_H
#define MIKERNEL_LIB_PANIC_H

void panic_impl(const char *file, int line, const char *fmt, ...) __attribute__((noreturn));

#define panic(fmt, ...) panic_impl(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define KASSERT(cond)                                  \
  do {                                                 \
    if (!(cond)) {                                     \
      panic("assertion failed: %s", #cond);             \
    }                                                  \
  } while (0)

#endif
