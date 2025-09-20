SRCDIR := sources
BUILDDIR := objects
DISTDIR := distribution
INCDIR := headers

KERNEL_SRCS := $(wildcard $(SRCDIR)/kernel/*.c)
DEVICE_SRCS := $(wildcard $(SRCDIR)/devices/*.c)
SYSTEM_SRCS := $(wildcard $(SRCDIR)/system/*.c)
ARCH_C_SRCS := $(wildcard $(SRCDIR)/arch/x86_64/*.c)
ARCH_ASM_SRCS := $(wildcard $(SRCDIR)/arch/x86_64/*.s)

KERNEL_OBJS := $(KERNEL_SRCS:$(SRCDIR)/kernel/%.c=$(BUILDDIR)/kernel/%.o)
DEVICE_OBJS := $(DEVICE_SRCS:$(SRCDIR)/devices/%.c=$(BUILDDIR)/devices/%.o)
SYSTEM_OBJS := $(SYSTEM_SRCS:$(SRCDIR)/system/%.c=$(BUILDDIR)/system/%.o)
ARCH_C_OBJS := $(ARCH_C_SRCS:$(SRCDIR)/arch/x86_64/%.c=$(BUILDDIR)/arch/%.o)
ARCH_ASM_OBJS := $(ARCH_ASM_SRCS:$(SRCDIR)/arch/x86_64/%.s=$(BUILDDIR)/arch/%.o)

ALL_OBJS := $(KERNEL_OBJS) $(DEVICE_OBJS) $(SYSTEM_OBJS) $(ARCH_C_OBJS) $(ARCH_ASM_OBJS)

CROSS_COMPILER := x86_64-elf-gcc
ASSEMBLER := nasm
LINKER := x86_64-elf-ld

CFLAGS := -I$(INCDIR) -std=c99 -ffreestanding -O2 -Wall -Wextra \
          -nostdlib -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
          -mcmodel=kernel -fno-stack-protector -fno-pic

ASMFLAGS := -f elf64
LDFLAGS := -nostdlib -T bootloader/linker.ld -N

.PHONY: all kernel iso clean run debug verify

all: iso

kernel: $(DISTDIR)/apollo.bin

iso: $(DISTDIR)/apollo.iso

$(BUILDDIR)/kernel/%.o: $(SRCDIR)/kernel/%.c
	@mkdir -p $(dir $@)
	$(CROSS_COMPILER) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/arch/%.o: $(SRCDIR)/arch/x86_64/%.c
	@mkdir -p $(dir $@)
	$(CROSS_COMPILER) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/arch/%.o: $(SRCDIR)/arch/x86_64/%.s
	@mkdir -p $(dir $@)
	$(ASSEMBLER) $(ASMFLAGS) $< -o $@

$(BUILDDIR)/devices/%.o: $(SRCDIR)/devices/%.c
	@mkdir -p $(dir $@)
	$(CROSS_COMPILER) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/system/%.o: $(SRCDIR)/system/%.c
	@mkdir -p $(dir $@)
	$(CROSS_COMPILER) $(CFLAGS) -c $< -o $@

$(DISTDIR)/apollo.bin: $(ALL_OBJS)
	@mkdir -p $(DISTDIR)
	$(LINKER) $(LDFLAGS) -o $@ $(ALL_OBJS)

$(DISTDIR)/apollo.iso: $(DISTDIR)/apollo.bin
	@mkdir -p $(DISTDIR)/iso_root/boot/grub
	@cp $(DISTDIR)/apollo.bin $(DISTDIR)/iso_root/boot/apollo.bin
	@cp bootloader/grub.cfg $(DISTDIR)/iso_root/boot/grub/
	grub-mkrescue -o $@ $(DISTDIR)/iso_root

clean:
	rm -rf $(BUILDDIR) $(DISTDIR)

run: iso
	qemu-system-x86_64 -cdrom $(DISTDIR)/apollo.iso -m 512M -display gtk

debug: iso
	qemu-system-x86_64 -cdrom $(DISTDIR)/apollo.iso -m 512M -s -S -monitor stdio

verify: $(DISTDIR)/apollo.bin
	@chmod +x verify_elf.sh
	@./verify_elf.sh