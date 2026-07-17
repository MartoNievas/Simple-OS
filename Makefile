CROSS := i686-elf
CC := $(CROSS)-gcc

KERNEL_VBASE := 0xC0000000

CFLAGS := -std=c11 -ffreestanding -Wall -Wextra -Werror \
          -fno-stack-protector -fno-pic -fno-omit-frame-pointer \
          -mno-red-zone -mgeneral-regs-only \
          -Iinclude -DKERNEL_VBASE=$(KERNEL_VBASE)

ASFLAGS := -Iinclude -DKERNEL_VBASE=$(KERNEL_VBASE)

LDFLAGS := -T link.ld -ffreestanding -nostdlib \
           -Wl,--defsym=KERNEL_VBASE=$(KERNEL_VBASE)

LIBGCC := $(shell $(CC) -print-libgcc-file-name)

SRCS_C := $(shell find kernel -name '*.c')
SRCS_S := boot/multiboot2.S
OBJS := $(SRCS_C:.c=.o) $(SRCS_S:.S=.o)
DEPS := $(SRCS_C:.c=.d)

KERNEL := kernel.elf
ISO := mikernel.iso
LIMINE_DIR := third_party/limine

QEMU_FLAGS := -serial stdio -d int,cpu_reset -no-reboot -no-shutdown

.PHONY: all iso run debug gdb clean

all: $(KERNEL)

$(KERNEL): $(OBJS) link.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBGCC)

%.o: %.c
	$(CC) $(CFLAGS) -MMD -c $< -o $@

%.o: %.S
	$(CC) $(ASFLAGS) -c $< -o $@

-include $(DEPS)

iso: $(KERNEL)
	rm -rf iso_root
	mkdir -p iso_root/boot/limine
	cp $(KERNEL) iso_root/boot/kernel.elf
	cp boot/limine.conf iso_root/boot/limine/limine.conf
	cp $(LIMINE_DIR)/limine-bios.sys $(LIMINE_DIR)/limine-bios-cd.bin iso_root/boot/limine/
	xorriso -as mkisofs -R -r -J \
	    -b boot/limine/limine-bios-cd.bin \
	    -no-emul-boot -boot-load-size 4 -boot-info-table \
	    iso_root -o $(ISO)
	$(LIMINE_DIR)/limine bios-install $(ISO)

run: iso
	qemu-system-i386 -cdrom $(ISO) $(QEMU_FLAGS)

debug: iso
	qemu-system-i386 -cdrom $(ISO) $(QEMU_FLAGS) -s -S

gdb:
	gdb $(KERNEL) -ex "target remote localhost:1234"

clean:
	rm -f $(OBJS) $(DEPS) $(KERNEL) $(ISO)
	rm -rf iso_root
