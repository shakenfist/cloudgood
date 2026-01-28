/*
 * VGA Text Mode Demonstration
 *
 * This bare-metal program demonstrates how VGA text mode works by writing
 * directly to video memory at 0xB8000. In text mode, the video card
 * displays an 80x25 grid of characters. Each cell is represented by two
 * bytes in memory:
 *
 *   Byte 0: ASCII character code
 *   Byte 1: Attribute byte (foreground color, background color, blink)
 *
 * The attribute byte format:
 *   Bits 0-3: Foreground color (16 colors)
 *   Bits 4-6: Background color (8 colors)
 *   Bit 7:    Blink enable (or high-intensity background, depending on mode)
 *
 * The video memory is laid out row by row, with each row being 80 characters
 * (160 bytes). The character at position (x, y) is at offset: (y * 80 + x) * 2
 */

#include <stdint.h>

/* VGA text mode dimensions */
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

/* VGA text mode memory address */
#define VGA_MEMORY 0xB8000

/* VGA colors */
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,  /* Also known as yellow */
    VGA_COLOR_WHITE = 15,
};

/* Create an attribute byte from foreground and background colors */
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg)
{
    return fg | (bg << 4);
}

/* Create a 16-bit VGA entry from a character and attribute byte */
static inline uint16_t vga_entry(unsigned char c, uint8_t color)
{
    return (uint16_t) c | ((uint16_t) color << 8);
}

/* Pointer to VGA text buffer */
static uint16_t *const vga_buffer = (uint16_t *) VGA_MEMORY;

/* Clear the screen with a given color */
static void clear_screen(uint8_t color)
{
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            const int index = y * VGA_WIDTH + x;
            vga_buffer[index] = vga_entry(' ', color);
        }
    }
}

/* Write a character at a specific position */
static void put_char(int x, int y, char c, uint8_t color)
{
    const int index = y * VGA_WIDTH + x;
    vga_buffer[index] = vga_entry(c, color);
}

/* Write a string at a specific position */
static void put_string(int x, int y, const char *str, uint8_t color)
{
    for (int i = 0; str[i] != '\0'; i++) {
        put_char(x + i, y, str[i], color);
    }
}

/* Draw a horizontal line using box-drawing characters */
static void draw_hline(int x, int y, int width, uint8_t color)
{
    for (int i = 0; i < width; i++) {
        put_char(x + i, y, 196, color);  /* Single horizontal line */
    }
}

/* Draw the color palette demonstration */
static void draw_color_demo(int start_y)
{
    const char *color_names[] = {
        "Black", "Blue", "Green", "Cyan",
        "Red", "Magenta", "Brown", "Light Grey",
        "Dark Grey", "Light Blue", "Light Green", "Light Cyan",
        "Light Red", "Pink", "Yellow", "White"
    };

    put_string(2, start_y, "Color Palette:",
               vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));

    for (int i = 0; i < 16; i++) {
        int row = start_y + 1 + (i / 4);
        int col = 2 + (i % 4) * 19;

        /* Color swatch (inverted to show background color) */
        uint8_t swatch_color = vga_entry_color(VGA_COLOR_WHITE, i);
        put_string(col, row, "   ", swatch_color);

        /* Color name */
        uint8_t text_color = vga_entry_color(i, VGA_COLOR_BLACK);
        put_string(col + 4, row, color_names[i], text_color);
    }
}

/* Draw memory layout explanation */
static void draw_memory_demo(int start_y)
{
    uint8_t heading = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    uint8_t normal = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    put_string(2, start_y, "Memory Layout at 0xB8000:", heading);
    put_string(2, start_y + 1,
        "Each character = 2 bytes: [ASCII code] [Attribute]", normal);
    put_string(2, start_y + 2,
        "Attribute byte: bits 0-3 = FG color, bits 4-6 = BG color", normal);

    /* Show a worked example */
    put_string(2, start_y + 4, "Example: 'A' in yellow on blue:", heading);
    put_string(4, start_y + 5, "Character byte: 0x41 (ASCII 'A')", normal);
    put_string(4, start_y + 6, "Attribute byte: 0x1E (FG=14/yellow, BG=1/blue)",
               normal);
    put_string(4, start_y + 7, "Result: ", normal);
    put_char(12, start_y + 7, 'A',
             vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLUE));
}

/* Simple delay loop */
static void delay(int cycles)
{
    for (volatile int i = 0; i < cycles * 1000000; i++) {
        /* Busy wait */
    }
}

/* Animate a simple marquee to show dynamic updates */
static void animate_marquee(int y, const char *text, uint8_t color)
{
    int len = 0;
    while (text[len] != '\0') len++;

    /* Clear the line first */
    for (int x = 0; x < VGA_WIDTH; x++) {
        put_char(x, y, ' ', color);
    }

    /* Scroll the text across */
    for (int offset = VGA_WIDTH; offset > -len; offset--) {
        for (int i = 0; i < len; i++) {
            int x = offset + i;
            if (x >= 0 && x < VGA_WIDTH) {
                put_char(x, y, text[i], color);
            }
        }
        /* Clear character that just scrolled out */
        if (offset + len < VGA_WIDTH && offset + len >= 0) {
            put_char(offset + len, y, ' ', color);
        }
        delay(1);
    }
}

/* Main kernel entry point */
void kernel_main(void)
{
    uint8_t title_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    uint8_t border_color = vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);

    /* Clear screen to black */
    clear_screen(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));

    /* Draw title bar */
    for (int x = 0; x < VGA_WIDTH; x++) {
        put_char(x, 0, ' ', title_color);
    }
    put_string(2, 0, "VGA Text Mode Demonstration - Bare Metal C", title_color);

    /* Decorative line under title */
    draw_hline(0, 1, VGA_WIDTH, border_color);

    /* Draw the demonstrations */
    draw_memory_demo(3);
    draw_hline(0, 11, VGA_WIDTH, border_color);
    draw_color_demo(12);

    /* Draw bottom border */
    draw_hline(0, 17, VGA_WIDTH, border_color);

    /* Info text */
    uint8_t info_color = vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
    put_string(2, 18,
        "This program writes directly to video memory at physical address",
        info_color);
    put_string(2, 19,
        "0xB8000. No OS, no drivers - just raw hardware access.",
        info_color);

    /* Animate a scrolling message at the bottom */
    uint8_t marquee_color = vga_entry_color(VGA_COLOR_LIGHT_GREEN,
                                            VGA_COLOR_BLACK);

    /* Loop forever with animation */
    while (1) {
        animate_marquee(23,
            ">>> VGA text mode: 80x25 characters, 16 colors, "
            "memory-mapped at 0xB8000 <<<   ",
            marquee_color);
    }
}
