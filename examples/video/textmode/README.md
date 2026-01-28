# VGA Text Mode Demonstration

This bare-metal C program demonstrates how VGA text mode works by writing
directly to video memory. It boots in QEMU without an operating system.

## What it demonstrates

- **Memory-mapped video**: Writing directly to physical address 0xB8000
- **Character/attribute pairs**: Each screen cell is 2 bytes (char + color)
- **The 16-color palette**: All VGA text mode colors displayed
- **Simple animation**: A scrolling marquee showing dynamic updates

## Requirements

You need either:

1. **A cross-compiler** (recommended): `i686-elf-gcc` toolchain
2. **System GCC with multilib**: `gcc` with 32-bit support installed

Plus:

- `qemu-system-x86_64` or `qemu-system-i386`
- `grub-mkrescue`, `xorriso`, and `mtools` (for creating the bootable ISO)
- `grub-pc-bin` (BIOS boot modules - multiboot requires legacy BIOS, not EFI)

### Installing on Debian/Ubuntu

```bash
# For system GCC approach (easier)
sudo apt install gcc-multilib qemu-system-x86 grub-pc-bin xorriso mtools

# For cross-compiler approach (cleaner)
# See: https://wiki.osdev.org/GCC_Cross-Compiler
```

## Building and running

```bash
make        # Build the kernel
make run    # Build and run in QEMU (curses mode)
make run-sdl    # Run with graphical window
make run-vnc    # Run with VNC server on :0
make clean  # Remove build artifacts
```

To exit QEMU in curses mode: press `Ctrl+A` then `X`.

## How it works

The program uses the [Multiboot specification](https://www.gnu.org/software/grub/manual/multiboot/multiboot.html).
We create a bootable ISO with GRUB as the bootloader, which QEMU boots via
`-cdrom`. The boot sequence:

1. GRUB loads from the ISO and reads `grub.cfg`
2. GRUB loads `textmode.bin` using the multiboot protocol
3. `boot.s` - Sets up a small stack and calls the C entry point
4. `kernel_main()` in `textmode.c` - Takes over and writes to VGA memory

VGA text mode memory layout at 0xB8000:

```
Offset 0x0000: Row 0, Column 0 - [char][attr][char][attr]...
Offset 0x00A0: Row 1, Column 0 - [char][attr][char][attr]...
...
```

Each cell is 2 bytes:
- **Byte 0**: ASCII character code
- **Byte 1**: Attribute (bits 0-3 = foreground, bits 4-6 = background, bit 7 = blink)
