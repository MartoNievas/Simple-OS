# mikernel

Kernel monolítico educativo para **x86 (i386, 32 bits)**, arrancado con **Limine** vía **Multiboot2**.
Escrito en **C (C11, freestanding)** y **NASM**.

Proyecto personal para poner en práctica AyOC y Sistemas Operativos (Exactas, UBA).

---

## Estado actual

| Fase | Descripción | Estado |
|------|-------------|--------|
| 0 | Toolchain, boot, esqueleto | 🚧 en progreso |
| 1 | Salida (VGA/serial), panic, parseo Multiboot2 | ⬜ |
| 2 | GDT, IDT, PIC, PIT, teclado | ⬜ |
| 3 | PMM, paginación, VMM, heap | ⬜ |
| 4 | Multitarea, ring 3, syscalls | ⬜ |
| 5 | VFS, initrd (tar) | ⬜ |
| 6 | ELF loader, libc mínima, shell | ⬜ |

---

## Requisitos

Cross-toolchain (no usar el `gcc` del sistema — linkea contra glibc):

```sh
# Arch Linux (AUR)
paru -S i686-elf-gcc i686-elf-binutils
pacman -S nasm qemu-system-x86 xorriso make gdb
```

Limine se clona como submódulo (branch binario):

```sh
git submodule update --init --recursive
make -C third_party/limine
```

---

## Build & run

```sh
make            # compila kernel.elf
make iso        # arma mikernel.iso con Limine + xorriso
make run        # QEMU con serial en stdio
make debug      # QEMU con -s -S (esperando GDB en :1234)
make gdb        # conecta GDB al QEMU de arriba
make clean
```

Flags de QEMU útiles ya presentes en el Makefile:
- `-serial stdio` — todo el logging del kernel sale por acá
- `-d int,cpu_reset -no-reboot -no-shutdown` — para cazar triple faults

---

## Estructura

```
.
├── boot/
│   ├── multiboot2.S      # header Multiboot2 + _start
│   └── limine.cfg        # config del bootloader
├── kernel/
│   ├── main.c            # kmain(magic, mb2_info)
│   ├── mb2.c/.h          # parseo de tags Multiboot2
│   ├── arch/i386/
│   │   ├── gdt.c, tss.c
│   │   ├── idt.c, isr.S, irq.S
│   │   ├── pic.c, pit.c
│   │   └── switch.S      # context switch
│   ├── mm/
│   │   ├── pmm.c         # bitmap de frames de 4 KiB
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

## Decisiones de diseño (no cambiar sin motivo)

- **Higher-half kernel**: cargado en `0x100000` físico, mapeado en `0xC0000000` virtual.
  El linker script usa `AT()` para separar VMA de LMA. Antes de habilitar paginación,
  todo símbolo se accede restándole `KERNEL_VBASE`.
- **Multiboot2, no Multiboot1.** El header pide: framebuffer tag, memory map tag,
  y alineación de módulos a página.
- **Páginas de 4 KiB**, sin PAE, sin PSE por ahora.
- **Sin dependencias externas.** Todo lo que necesite el kernel se escribe acá (`kernel/lib/`).
- **Todo lo que pueda ser C, es C.** ASM solo donde es inevitable: `_start`, stubs de
  ISR/IRQ, context switch, carga de GDT/IDT/CR3.

---

## Convenciones de código

- Estilo: **snake_case**, llaves K&R, indentación 4 espacios.
- Prefijos por subsistema: `pmm_`, `vmm_`, `sched_`, `vfs_`, `kbd_`.
- Compilación: `-std=c11 -ffreestanding -Wall -Wextra -Werror -fno-stack-protector -fno-pic -fno-omit-frame-pointer -mno-red-zone -mgeneral-regs-only`
- Tipos: usar `<stdint.h>` (`uint32_t`, `uintptr_t`). Nunca `int` para direcciones.
  Distinguir `phys_addr_t` de `virt_addr_t` (typedefs en `include/types.h`) — es la única
  defensa contra confundirlas.
- Toda función que pueda fallar devuelve `int` (0 ok, negativo errno-style) o `NULL`.
- `panic(fmt, ...)` para errores irrecuperables. `KASSERT(cond)` liberalmente.
- Comentarios y mensajes: **en inglés**. Commits: en inglés, imperativo.

---

## Notas para el agente (Claude Code)

- **No inventes valores de hardware.** Constantes de PIC, PIT, VGA, bits de CR0/CR4,
  formato de PDE/PTE: verificar contra el Intel SDM Vol. 3A o el OSDev Wiki. Si no
  estás seguro, decilo en vez de adivinar.
- **Las excepciones con error code y sin error code no son simétricas.** Los stubs
  deben pushear un dummy error code en las que no lo tienen (0, 1, 2, 3, 4, 5, 6, 7,
  9, 15, 16, 18, 19, 20) para que el stack frame sea uniforme.
- **Nunca uses funciones de libc del host.** No hay `stdlib.h`, `stdio.h`, `string.h`
  del sistema. Solo los headers freestanding: `stdint.h`, `stddef.h`, `stdbool.h`,
  `stdarg.h`, `limits.h`.
- **Cuidado con las optimizaciones sobre MMIO**: usar `volatile` para el framebuffer
  y los registros mapeados.
- **Cambios de un subsistema por vez.** Cada fase del roadmap debe compilar y bootear
  antes de pasar a la siguiente. Si algo no bootea, el commit anterior sí lo hacía.
- **Verificá con `make run` antes de dar algo por terminado.** Si el kernel triplefaultea,
  `make run` con `-d int` muestra el vector de la excepción original.
- No agregues dependencias, ni submódulos, ni build systems nuevos (nada de CMake/meson).

---

## Milestones de verificación

Cada fase se considera cerrada cuando:

0. Limine bootea el kernel y `_start` hace `hlt` sin panic en QEMU.
1. El kernel imprime el memory map de Multiboot2 por serial y por pantalla.
2. `int $0x3` produce un dump de registros legible; el teclado escribe en pantalla.
3. Un `#PF` en dirección no mapeada se maneja e imprime `CR2` + código de error.
4. Dos procesos en ring 3 alternan prints vía `write` sobre `int 0x80`.
5. Se lista y se lee un archivo del initrd tar.
6. La shell corre desde un ELF de userland.

---

## Referencias

- [OSDev Wiki](https://wiki.osdev.org/) — punto de partida para todo
- Intel SDM Vol. 3A — modo protegido, paginación, interrupciones
- [Multiboot2 spec](https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html)
- [Limine Protocol / Boot](https://github.com/limine-bootloader/limine)
- [8259A PIC datasheet](https://wiki.osdev.org/8259_PIC)

---

## Licencia

MIT
