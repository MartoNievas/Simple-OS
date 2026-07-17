# mikernel

Educational monolithic kernel for **x86 (i386, 32-bit)**, booted with **Limine** via **Multiboot2**.
Written in **C (C11, freestanding)** and **GAS (AT&T syntax, via i686-elf-gcc)**.

Personal project to practice AyOC (Computer Architecture and Organization) and Operating Systems, courses at Exactas, UBA (University of Buenos Aires).

---

## Current status

| Phase | Description | Status |
|------|-------------|--------|
| 0 | Toolchain, boot, skeleton | ✅ done |
| 1 | Output (VGA/serial), panic, Multiboot2 parsing | ⬜ |
| 2 | GDT, IDT, PIC, PIT, keyboard | ⬜ |
| 3 | PMM, paging, VMM, heap | ⬜ |
| 4 | Multitasking, ring 3, syscalls | ⬜ |
| 5 | VFS, initrd (tar) | ⬜ |
| 6 | ELF loader, minimal libc, shell | ⬜ |

---

## Requirements

Cross-toolchain (do not use the system `gcc`: it links against glibc):

```sh
# Arch Linux (AUR)
paru -S i686-elf-gcc i686-elf-binutils
pacman -S qemu-system-x86 xorriso make gdb
```

Limine is added as a git submodule, pinned to `v11.x-binary` (the last binary
branch available; since v12.x the project ships only release tarballs, with
no `-binary` branch):

```sh
git submodule update --init --recursive
make -C third_party/limine
```

---

## Build & run

```sh
make            # builds kernel.elf
make iso        # builds mikernel.iso with Limine + xorriso
make run        # QEMU with serial on stdio
make debug      # QEMU with -s -S (waiting for GDB on :1234)
make gdb        # connects GDB to the QEMU instance above
make clean
```

Useful QEMU flags already set in the Makefile:
- `-serial stdio`: all kernel logging goes through here
- `-d int,cpu_reset -no-reboot -no-shutdown`: for catching triple faults

---

## Structure

```
.
├── boot/
│   ├── multiboot2.S      # Multiboot2 header + _start + higher-half bootstrap
│   └── limine.conf       # bootloader config (format >= Limine v5.x)
├── kernel/
│   ├── main.c            # kmain(magic, mb2_info)
│   ├── mb2.c/.h          # Multiboot2 tag parsing
│   ├── arch/i386/
│   │   ├── gdt.c, tss.c
│   │   ├── idt.c, isr.S, irq.S
│   │   ├── pic.c, pit.c
│   │   └── switch.S      # context switch
│   ├── mm/
│   │   ├── pmm.c         # 4 KiB frame bitmap
│   │   ├── vmm.c         # page dir/tables, map_page()
│   │   └── heap.c        # kmalloc/kfree
│   ├── sched/
│   ├── fs/
│   └── lib/              # printf, string.c, panic.c
├── include/
├── link.ld               # linker script
└── third_party/limine/
```

---

## Design decisions (do not change without a reason)

- **Higher-half kernel**: loaded at `0x100000` physical, mapped at `0xC0000000`
  virtual. The linker script uses `AT()` to separate VMA from LMA. Before
  paging is enabled, every symbol is accessed by subtracting `KERNEL_VBASE`.
- **Multiboot2, not Multiboot1.** The header requests a framebuffer tag, a
  memory map tag, and page alignment for modules.
- **4 KiB pages**, no PAE, no PSE for now.
- **No external dependencies.** Anything the kernel needs gets written here
  (`kernel/lib/`).
- **Everything that can be C is C.** Assembly only where unavoidable:
  `_start`, ISR/IRQ stubs, context switch, loading GDT/IDT/CR3.

---

## Code conventions

- Style: **snake_case**, K&R braces, 4-space indentation.
- Prefixes per subsystem: `pmm_`, `vmm_`, `sched_`, `vfs_`, `kbd_`.
- Compilation: `-std=c11 -ffreestanding -Wall -Wextra -Werror -fno-stack-protector -fno-pic -fno-omit-frame-pointer -mno-red-zone -mgeneral-regs-only`
- Types: use `<stdint.h>` (`uint32_t`, `uintptr_t`). Never `int` for
  addresses. Keep `phys_addr_t` separate from `virt_addr_t` (typedefs in
  `include/types.h`); that's the only real defense against mixing them up.
- Any function that can fail returns `int` (0 for success, negative
  errno-style) or `NULL`.
- `panic(fmt, ...)` for unrecoverable errors. `KASSERT(cond)` liberally.
- Comments and messages: in English. Commits: in English, imperative.

---

## Notes for the agent (Claude Code)

- **Don't invent hardware values.** PIC, PIT, VGA constants, CR0/CR4 bits,
  PDE/PTE layout: verify against the Intel SDM Vol. 3A or the OSDev Wiki. If
  unsure, say so instead of guessing.
- **Exceptions with and without an error code aren't symmetric.** Stubs must
  push a dummy error code for the vectors that don't have one (0, 1, 2, 3, 4,
  5, 6, 7, 9, 15, 16, 18, 19, 20) so the stack frame stays uniform.
- **Never use host libc functions.** There's no system `stdlib.h`,
  `stdio.h`, `string.h`. Only the freestanding headers: `stdint.h`,
  `stddef.h`, `stdbool.h`, `stdarg.h`, `limits.h`.
- **Watch out for compiler optimizations around MMIO**: use `volatile` for
  the framebuffer and any memory-mapped registers.
- **Change one subsystem at a time.** Each roadmap phase must compile and
  boot before moving to the next. If something doesn't boot, the previous
  commit still should.
- **Verify with `make run` before calling anything done.** If the kernel
  triple-faults, `make run` with `-d int` shows the original exception
  vector.
- Don't add dependencies, submodules, or new build systems (no CMake/meson).

---

## Verification milestones

Each phase is considered closed when:

0. Limine boots the kernel and `_start` does `hlt` with no panic in QEMU.
1. The kernel prints the Multiboot2 memory map over serial and on screen.
2. `int $0x3` produces a readable register dump; the keyboard writes to the
   screen.
3. A `#PF` at an unmapped address is handled and prints `CR2` plus the error
   code.
4. Two ring-3 processes alternate prints via `write` over `int 0x80`.
5. A file from the initrd tar archive is listed and read.
6. The shell runs from a userland ELF.

---

## References

- [OSDev Wiki](https://wiki.osdev.org/): starting point for everything
- Intel SDM Vol. 3A: protected mode, paging, interrupts
- [Multiboot2 spec](https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html)
- [Limine Protocol / Boot](https://github.com/limine-bootloader/limine)
- [8259A PIC datasheet](https://wiki.osdev.org/8259_PIC)

---

## License

MIT
