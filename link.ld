/*
 * Higher-half layout: the kernel is loaded at 1 MiB physical and linked at
 * KERNEL_VBASE + 1 MiB virtual. AT() gives each section its physical (LMA)
 * placement while the symbol addresses stay in the higher half (VMA).
 *
 * KERNEL_VBASE must match the literal used in boot/multiboot2.S — passed in
 * here via --defsym (see Makefile) instead of being hardcoded twice.
 */

ENTRY(_start)

SECTIONS
{
    . = 0x00100000;
    _kernel_phys_start = .;

    /*
     * Multiboot2 header + the early boot trampoline (_start) must run with
     * paging disabled, so this part is linked at its physical load address
     * (no KERNEL_VBASE offset yet).
     */
    .multiboot ALIGN(8) :
    {
        *(.multiboot.header)
        *(.multiboot.text)
    }

    . += KERNEL_VBASE;

    _kernel_start = .;

    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VBASE)
    {
        *(.text)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VBASE)
    {
        *(.rodata)
    }

    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VBASE)
    {
        *(.data)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VBASE)
    {
        *(COMMON)
        *(.bss)
        *(.bootstrap_stack)
    }

    _kernel_end = .;
    _kernel_phys_end = . - KERNEL_VBASE;

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.note.*)
    }
}
