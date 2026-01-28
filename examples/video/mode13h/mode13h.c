/*
 * VGA Mode 13h Demonstration
 *
 * This bare-metal program demonstrates VGA Mode 13h (320x200, 256 colors) by
 * writing directly to video memory. This was THE iconic graphics mode for
 * DOS games in the late 1980s and early 1990s - Doom, Wolfenstein 3D, and
 * countless others used this mode.
 *
 * Mode 13h Memory Layout:
 *   - 320x200 pixels, 256 colors (8 bits per pixel)
 *   - Linear framebuffer at 0xA0000
 *   - One byte per pixel, laid out sequentially row by row
 *   - Total: 320 * 200 = 64,000 bytes
 *
 * This is beautifully simple compared to earlier modes:
 *   - No planar memory (unlike EGA's 16-color modes)
 *   - No interleaved banks (unlike CGA)
 *   - Direct pixel access: framebuffer[y * 320 + x] = color
 *
 * The VGA palette has 256 entries, each with 6-bit RGB components (0-63).
 * We set up an RGB332 palette: 3 bits red, 3 bits green, 2 bits blue,
 * giving us a reasonable color range without custom palette management.
 */

#include <stdint.h>
#include "vga_image.h"

/* VGA Mode 13h parameters */
#define VGA_WIDTH  320
#define VGA_HEIGHT 200
#define VGA_FRAMEBUFFER 0xA0000

/* VGA I/O ports */
#define VGA_MISC_WRITE    0x3C2
#define VGA_SEQ_INDEX     0x3C4
#define VGA_SEQ_DATA      0x3C5
#define VGA_PALETTE_MASK  0x3C6
#define VGA_PALETTE_WRITE 0x3C8
#define VGA_PALETTE_DATA  0x3C9
#define VGA_CRTC_INDEX    0x3D4
#define VGA_CRTC_DATA     0x3D5
#define VGA_GC_INDEX      0x3CE
#define VGA_GC_DATA       0x3CF
#define VGA_AC_INDEX      0x3C0
#define VGA_AC_WRITE      0x3C0
#define VGA_INPUT_STATUS  0x3DA

/* Pointer to VGA framebuffer */
static uint8_t *const vga = (uint8_t *) VGA_FRAMEBUFFER;

/* Write to an I/O port */
static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

/* Read from an I/O port */
static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/*
 * Set VGA Mode 13h (320x200, 256 colors).
 *
 * This programs all the VGA registers to achieve Mode 13h.
 * The register values are well-documented and standard.
 */
static void set_mode_13h(void)
{
    int i;

    /* Miscellaneous Output Register */
    static const uint8_t misc_output = 0x63;

    /* Sequencer registers */
    static const uint8_t seq_regs[5] = {
        0x03,  /* 0: Reset */
        0x01,  /* 1: Clocking mode */
        0x0F,  /* 2: Map mask - enable all 4 planes */
        0x00,  /* 3: Character map select */
        0x0E,  /* 4: Memory mode - chain-4, extended memory */
    };

    /* CRTC registers for 320x200 */
    static const uint8_t crtc_regs[25] = {
        0x5F,  /* 0x00: Horizontal Total */
        0x4F,  /* 0x01: Horizontal Display End */
        0x50,  /* 0x02: Start Horizontal Blanking */
        0x82,  /* 0x03: End Horizontal Blanking */
        0x54,  /* 0x04: Start Horizontal Retrace */
        0x80,  /* 0x05: End Horizontal Retrace */
        0xBF,  /* 0x06: Vertical Total */
        0x1F,  /* 0x07: Overflow */
        0x00,  /* 0x08: Preset Row Scan */
        0x41,  /* 0x09: Maximum Scan Line */
        0x00,  /* 0x0A: Cursor Start */
        0x00,  /* 0x0B: Cursor End */
        0x00,  /* 0x0C: Start Address High */
        0x00,  /* 0x0D: Start Address Low */
        0x00,  /* 0x0E: Cursor Location High */
        0x00,  /* 0x0F: Cursor Location Low */
        0x9C,  /* 0x10: Vertical Retrace Start */
        0x0E,  /* 0x11: Vertical Retrace End */
        0x8F,  /* 0x12: Vertical Display End */
        0x28,  /* 0x13: Offset */
        0x40,  /* 0x14: Underline Location */
        0x96,  /* 0x15: Start Vertical Blanking */
        0xB9,  /* 0x16: End Vertical Blanking */
        0xA3,  /* 0x17: CRTC Mode Control */
        0xFF,  /* 0x18: Line Compare */
    };

    /* Graphics Controller registers */
    static const uint8_t gc_regs[9] = {
        0x00,  /* 0: Set/Reset */
        0x00,  /* 1: Enable Set/Reset */
        0x00,  /* 2: Color Compare */
        0x00,  /* 3: Data Rotate */
        0x00,  /* 4: Read Map Select */
        0x40,  /* 5: Graphics Mode - 256 color mode */
        0x05,  /* 6: Miscellaneous - A0000, graphics mode */
        0x0F,  /* 7: Color Don't Care */
        0xFF,  /* 8: Bit Mask */
    };

    /* Attribute Controller registers */
    static const uint8_t ac_regs[21] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x41,  /* 0x10: Attribute Mode Control */
        0x00,  /* 0x11: Overscan Color */
        0x0F,  /* 0x12: Color Plane Enable */
        0x00,  /* 0x13: Horizontal Pixel Panning */
        0x00,  /* 0x14: Color Select */
    };

    /* Write Miscellaneous Output Register */
    outb(VGA_MISC_WRITE, misc_output);

    /* Program Sequencer */
    outb(VGA_SEQ_INDEX, 0x00);
    outb(VGA_SEQ_DATA, 0x01);  /* Reset */

    for (i = 1; i < 5; i++) {
        outb(VGA_SEQ_INDEX, i);
        outb(VGA_SEQ_DATA, seq_regs[i]);
    }

    outb(VGA_SEQ_INDEX, 0x00);
    outb(VGA_SEQ_DATA, 0x03);  /* End reset */

    /* Unlock CRTC registers */
    outb(VGA_CRTC_INDEX, 0x11);
    outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) & 0x7F);

    /* Program CRTC */
    for (i = 0; i < 25; i++) {
        outb(VGA_CRTC_INDEX, i);
        outb(VGA_CRTC_DATA, crtc_regs[i]);
    }

    /* Program Graphics Controller */
    for (i = 0; i < 9; i++) {
        outb(VGA_GC_INDEX, i);
        outb(VGA_GC_DATA, gc_regs[i]);
    }

    /* Program Attribute Controller */
    (void)inb(VGA_INPUT_STATUS);  /* Reset flip-flop */

    for (i = 0; i < 21; i++) {
        outb(VGA_AC_INDEX, i);
        outb(VGA_AC_WRITE, ac_regs[i]);
    }

    /* Enable video */
    (void)inb(VGA_INPUT_STATUS);
    outb(VGA_AC_INDEX, 0x20);
}

/*
 * Set up an RGB332 palette.
 *
 * This creates a 256-color palette where each byte encodes:
 *   Bits 7-5: Red (0-7)
 *   Bits 4-2: Green (0-7)
 *   Bits 1-0: Blue (0-3)
 *
 * This gives us 8 levels of red and green, 4 levels of blue.
 * Not perfect, but good enough for general images without custom palettes.
 */
static void set_rgb332_palette(void)
{
    int i;

    outb(VGA_PALETTE_MASK, 0xFF);
    outb(VGA_PALETTE_WRITE, 0);  /* Start at color 0 */

    for (i = 0; i < 256; i++) {
        /* Extract RGB332 components */
        uint8_t r3 = (i >> 5) & 0x07;  /* 3 bits */
        uint8_t g3 = (i >> 2) & 0x07;  /* 3 bits */
        uint8_t b2 = i & 0x03;         /* 2 bits */

        /* Scale to 6-bit DAC values (0-63) */
        /* Red: 0-7 -> 0-63 (multiply by 9) */
        /* Green: 0-7 -> 0-63 (multiply by 9) */
        /* Blue: 0-3 -> 0-63 (multiply by 21) */
        uint8_t r6 = r3 * 9;
        uint8_t g6 = g3 * 9;
        uint8_t b6 = b2 * 21;

        outb(VGA_PALETTE_DATA, r6);
        outb(VGA_PALETTE_DATA, g6);
        outb(VGA_PALETTE_DATA, b6);
    }
}

/* Clear screen to black */
static void clear_screen(void)
{
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = 0;
    }
}

/* Copy pre-converted image to framebuffer */
static void display_image(void)
{
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = vga_framebuffer[i];
    }
}

/* Draw a single pixel */
static void put_pixel(int x, int y, uint8_t color)
{
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        vga[y * VGA_WIDTH + x] = color;
    }
}

/* Draw a border to demonstrate direct pixel access */
static void draw_border(void)
{
    /* White border (RGB332: 0xFF = white) */
    uint8_t white = 0xFF;

    for (int x = 0; x < VGA_WIDTH; x++) {
        put_pixel(x, 0, white);
        put_pixel(x, VGA_HEIGHT - 1, white);
    }
    for (int y = 0; y < VGA_HEIGHT; y++) {
        put_pixel(0, y, white);
        put_pixel(VGA_WIDTH - 1, y, white);
    }
}

/* Main kernel entry point */
void kernel_main(void)
{
    /* Set Mode 13h */
    set_mode_13h();

    /* Set up RGB332 palette */
    set_rgb332_palette();

    /* Clear screen */
    clear_screen();

    /* Display the image */
    display_image();

    /* Draw border */
    draw_border();

    /* Halt */
    while (1) {
        __asm__ volatile ("hlt");
    }
}
