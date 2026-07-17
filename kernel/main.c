#include <stdint.h>

#include "types.h"

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289u

void kmain(uint32_t magic, phys_addr_t mb2_info_phys)
{
    (void)mb2_info_phys;

    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        for (;;) {
            __asm__ volatile("cli; hlt");
        }
    }

    /* Phase 0 milestone: reach here and halt without a panic.
       Multiboot2 tag parsing and serial/VGA output land in phase 1. */
    for (;;) {
        __asm__ volatile("hlt");
    }
}
