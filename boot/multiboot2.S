/*
 * Multiboot2 header + early boot trampoline.
 *
 * This code runs with paging disabled, so it cannot use any symbol whose
 * linked (higher-half) address hasn't been corrected by hand first:
 * `symbol - KERNEL_VBASE` gives its physical address. Past the jump into
 * `2:` below, paging is active and symbols resolve normally again.
 *
 * KERNEL_VBASE is injected by the Makefile (-DKERNEL_VBASE=0xC0000000) so
 * it is defined in exactly one place; link.ld receives the same value via
 * --defsym.
 */

.section .multiboot.header, "a"
.align 8
mb2_header_start:
    .long 0xE85250D6                                                /* magic */
    .long 0                                                         /* architecture: 0 = i386 protected mode */
    .long mb2_header_end - mb2_header_start                         /* header_length */
    .long -(0xE85250D6 + 0 + (mb2_header_end - mb2_header_start))   /* checksum */

    /* framebuffer tag (type=5, size=20): no preference, let Limine pick */
    .align 8
    .word 5, 0
    .long 20
    .long 0     /* width */
    .long 0     /* height */
    .long 0     /* depth */

    /* module alignment tag (type=6, size=8): page-align modules */
    .align 8
    .word 6, 0
    .long 8

    /* end tag (type=0, size=8) */
    .align 8
    .word 0, 0
    .long 8
mb2_header_end:

/* 16 KiB boot stack, used from the moment paging comes up. */
.section .bootstrap_stack, "aw", @nobits
stack_bottom:
    .skip 16384
stack_top:

/*
 * Boot-time page directory/table: identity-maps the first 4 MiB of physical
 * memory and also maps it at KERNEL_VBASE. 4 KiB pages only (no PSE/PAE,
 * per project convention). The identity mapping is deliberately kept (not
 * torn down after the jump): phase 0/1 still need to reach low physical
 * memory directly (VGA text buffer at 0xB8000, and the Multiboot2 info
 * struct at whatever physical address Limine left it — not guaranteed to
 * fall inside the kernel's own range). Replacing this with a proper VMM
 * that maps things on demand is phase 3 work, not this one.
 */
.section .bss, "aw", @nobits
.align 4096
boot_page_directory:
    .skip 4096
boot_page_table1:
    .skip 4096

mb_magic:
    .skip 4
mb_info_phys:
    .skip 4

.section .multiboot.text, "a"
.global _start
.type _start, @function
_start:
    cli

    /* Save the Multiboot2 handoff registers before they get clobbered.
       EAX = 0x36D76289 boot magic, EBX = physical pointer to the MBI. */
    movl %eax, mb_magic - KERNEL_VBASE
    movl %ebx, mb_info_phys - KERNEL_VBASE

    /* Fill boot_page_table1 with 1024 identity-mapped entries (4 KiB each
       => 4 MiB total), present + writable. */
    movl $(boot_page_table1 - KERNEL_VBASE), %edi
    xorl %esi, %esi
    movl $1024, %ecx

1:
    movl %esi, %edx
    orl $0x003, %edx           /* present, writable */
    movl %edx, (%edi)

    addl $4096, %esi
    addl $4, %edi
    loop 1b

    /* PDE 0 (virtual 0x00000000) and PDE (KERNEL_VBASE >> 22) (virtual
       KERNEL_VBASE) both point at the same page table. */
    movl $(boot_page_table1 - KERNEL_VBASE + 0x003), boot_page_directory - KERNEL_VBASE + 0
    movl $(boot_page_table1 - KERNEL_VBASE + 0x003), boot_page_directory - KERNEL_VBASE + (KERNEL_VBASE >> 22) * 4

    movl $(boot_page_directory - KERNEL_VBASE), %ecx
    movl %ecx, %cr3

    /* Enable paging (CR0.PG) and write-protect (CR0.WP). */
    movl %cr0, %ecx
    orl $0x80010000, %ecx
    movl %ecx, %cr0

    /* Absolute jump into the higher half: the linked address of `2f` is
       only valid now that paging is active. */
    lea 2f, %ecx
    jmp *%ecx
.size _start, . - _start

.section .text
.global kernel_higher_half_entry
.type kernel_higher_half_entry, @function
2:
kernel_higher_half_entry:
    mov $stack_top, %esp
    xor %ebp, %ebp

    pushl mb_info_phys
    pushl mb_magic
    call kmain

    cli
3:  hlt
    jmp 3b
.size kernel_higher_half_entry, . - kernel_higher_half_entry
