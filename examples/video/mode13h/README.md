# VGA Mode 13h Demonstration

This bare-metal C program demonstrates VGA Mode 13h (320x200, 256 colors) by
writing directly to video memory. Mode 13h was THE iconic graphics mode for
DOS games - Doom, Wolfenstein 3D, and countless others used it.

## What it demonstrates

- **Linear framebuffer**: One byte per pixel at 0xA0000, laid out sequentially
- **256 colors**: Using an RGB332 palette (3 bits red, 3 bits green, 2 bits blue)
- **Simple pixel access**: `framebuffer[y * 320 + x] = color`
- **VGA register programming**: Setting up the mode via I/O ports

## Why Mode 13h was revolutionary

Compared to earlier graphics modes:

- **No planar memory** (unlike EGA's 16-color modes where you had to write to
  4 separate bit planes)
- **No interleaved banks** (unlike CGA where even/odd scan lines were in
  different memory regions)
- **Direct pixel access** - just write a byte to set a pixel's color

This simplicity made it the go-to mode for game developers in the late 80s
and early 90s.

## Requirements

You need either:

1. **A cross-compiler** (recommended): `i686-elf-gcc` toolchain
2. **System GCC with multilib**: `gcc` with 32-bit support installed

Plus:

- `qemu-system-x86_64` or `qemu-system-i386`
- `grub-mkrescue`, `xorriso`, and `mtools` (for creating the bootable ISO)
- `grub-pc-bin` (BIOS boot modules - multiboot requires legacy BIOS, not EFI)

For image conversion (optional - pre-converted data is included):

- Python 3 with Pillow (`pip install pillow`)

### Installing on Debian/Ubuntu

```bash
sudo apt install gcc-multilib qemu-system-x86 grub-pc-bin xorriso mtools
```

## Building and running

```bash
make        # Build the kernel and ISO
make run    # Build and run in QEMU
make run-vnc    # Run with VNC server on :0
make clean  # Remove build artifacts
```

## Converting your own images

The `convert_image.py` script converts any image to Mode 13h format:

```bash
python3 convert_image.py input.png vga_image.h
```

This will:
1. Resize the image to fit within 320x200 while preserving aspect ratio
2. Convert colors to RGB332 format (256 colors)
3. Generate a C header file with the pixel data

## How it works

1. `boot.s` - Multiboot header and minimal bootstrap
2. `mode13h.c` - Sets Mode 13h via VGA registers and copies image to framebuffer
3. `vga_image.h` - Pre-converted image data (64KB)

The program:
1. Programs VGA sequencer, CRTC, graphics controller, and attribute registers
2. Sets up an RGB332 palette in the VGA DAC
3. Copies the image data to the framebuffer at 0xA0000
4. Halts (image remains on screen)

## Memory layout

```
0xA0000 + 0:      Pixel at (0, 0)
0xA0000 + 1:      Pixel at (1, 0)
...
0xA0000 + 319:    Pixel at (319, 0)
0xA0000 + 320:    Pixel at (0, 1)
...
0xA0000 + 63999:  Pixel at (319, 199)
```

Total: 64,000 bytes for a complete frame.
