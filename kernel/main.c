#include <stdint.h>

#include "../include/types.h"
#include "arch/i386/serial.h"
#include "arch/i386/vga.h"
#include "lib/panic.h"
#include "lib/printf.h"
#include "mb2.h"
#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289u

void kmain(uint32_t magic, phys_addr_t mb2_info_phys) {
  kprintf("mb2_info_phys=0x%x\n", mb2_info_phys);
  vga_init();
  serial_init();

  if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
    panic("bad multiboot2 magic: 0x%x", magic);
  }

  kprintf("mikernel: booted OK, magic=0x%x\n", magic);

  mb2_parse(mb2_info_phys);

  for (;;) {
    __asm__ volatile("hlt");
  }
}
