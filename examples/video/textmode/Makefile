# VGA Text Mode Bare Metal Example
#
# This Makefile builds a minimal kernel that demonstrates VGA text mode
# and runs it in QEMU via a bootable ISO with GRUB.
#
# Requirements:
#   - i686-elf cross-compiler (i686-elf-gcc, i686-elf-as, i686-elf-ld)
#     OR gcc with multilib support
#   - grub-mkrescue, xorriso, and mtools (for creating bootable ISO)
#   - grub-pc-bin (BIOS boot modules - multiboot requires legacy BIOS, not EFI)
#   - qemu-system-i386 or qemu-system-x86_64
#
# Usage:
#   make        - Build the kernel binary and bootable ISO
#   make run    - Build and run in QEMU
#   make clean  - Remove build artifacts

# Try to find a suitable compiler
# Prefer i686-elf cross-compiler if available, otherwise use system gcc
ifneq ($(shell which i686-elf-gcc 2>/dev/null),)
    CC = i686-elf-gcc
    AS = i686-elf-as
    LD = i686-elf-ld
else ifneq ($(shell which x86_64-elf-gcc 2>/dev/null),)
    CC = x86_64-elf-gcc
    AS = x86_64-elf-as
    LD = x86_64-elf-ld
    # Need to specify 32-bit for x86_64 toolchain
    CFLAGS_EXTRA = -m32
    ASFLAGS_EXTRA = --32
    LDFLAGS_EXTRA = -m elf_i386
else
    # Fall back to system gcc (requires multilib support)
    CC = gcc
    AS = as
    LD = ld
    CFLAGS_EXTRA = -m32
    ASFLAGS_EXTRA = --32
    LDFLAGS_EXTRA = -m elf_i386
endif

# Compiler flags for bare-metal 32-bit x86
CFLAGS = -ffreestanding -O2 -Wall -Wextra -nostdlib -fno-builtin \
         -fno-stack-protector -fno-pie -fno-pic $(CFLAGS_EXTRA)

# Assembler flags
ASFLAGS = $(ASFLAGS_EXTRA)

# Linker flags
LDFLAGS = -T linker.ld -nostdlib $(LDFLAGS_EXTRA)

# QEMU command
ifneq ($(shell which qemu-system-i386 2>/dev/null),)
    QEMU = qemu-system-i386
else
    QEMU = qemu-system-x86_64
endif

# Output files
KERNEL = textmode.bin
ISO = textmode.iso

# Object files
OBJS = boot.o textmode.o

# ISO build directory
ISODIR = isodir

.PHONY: all run run-sdl run-vnc clean info

all: $(ISO)

# Link the final kernel binary
$(KERNEL): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)
	@echo "Built $(KERNEL) successfully"

# Compile C source
textmode.o: textmode.c
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble boot code
boot.o: boot.s
	$(AS) $(ASFLAGS) $< -o $@

# Create bootable ISO with GRUB
$(ISO): $(KERNEL) grub.cfg
	@echo "Creating bootable ISO..."
	@mkdir -p $(ISODIR)/boot/grub
	@cp $(KERNEL) $(ISODIR)/boot/
	@cp grub.cfg $(ISODIR)/boot/grub/
	@grub-mkrescue -o $(ISO) $(ISODIR) 2>/dev/null
	@echo "Built $(ISO) successfully"
	@echo "Run 'make run' to launch in QEMU"

# Run in QEMU (curses mode for terminal)
run: $(ISO)
	@echo "Starting QEMU... (press Ctrl+A then X to exit)"
	$(QEMU) -cdrom $(ISO) -display curses

# Run with SDL window (default QEMU graphical output)
run-sdl: $(ISO)
	@echo "Starting QEMU with SDL window..."
	$(QEMU) -cdrom $(ISO)

# Run with VNC for graphical output (useful for remote/headless systems)
run-vnc: $(ISO)
	@echo "Starting QEMU with VNC on :0 (connect to localhost:5900)"
	$(QEMU) -cdrom $(ISO) -vnc :0

# Debug info
info:
	@echo "Compiler: $(CC)"
	@echo "Assembler: $(AS)"
	@echo "Linker: $(LD)"
	@echo "QEMU: $(QEMU)"
	@echo "GRUB: $(shell which grub-mkrescue 2>/dev/null || echo 'not found')"

clean:
	rm -f $(OBJS) $(KERNEL) $(ISO)
	rm -rf $(ISODIR)
