#!/usr/bin/env python3
'''
Convert an image to VGA Mode 13h format (320x200, 256 colors).

Mode 13h uses a linear framebuffer at 0xA0000 with one byte per pixel.
We use an RGB332 palette where each color is encoded as: RRRGGGBB
(3 bits red, 3 bits green, 2 bits blue).
'''

from PIL import Image
import sys


VGA_WIDTH = 320
VGA_HEIGHT = 200


def rgb_to_332(r, g, b):
    '''
    Convert 24-bit RGB to 8-bit RGB332 format.
    3 bits red, 3 bits green, 2 bits blue.
    '''
    r3 = (r >> 5) & 0x07  # Top 3 bits of red
    g3 = (g >> 5) & 0x07  # Top 3 bits of green
    b2 = (b >> 6) & 0x03  # Top 2 bits of blue
    return (r3 << 5) | (g3 << 2) | b2


def convert_image(input_path, output_path):
    '''Convert image to Mode 13h format and output as C header.'''
    # Load image
    img = Image.open(input_path)

    # Convert to RGB if necessary
    if img.mode != 'RGB':
        img = img.convert('RGB')

    # Calculate scaling to fit in VGA resolution while maintaining aspect ratio
    aspect = img.width / img.height
    if aspect > VGA_WIDTH / VGA_HEIGHT:
        # Image is wider - fit to width
        new_width = VGA_WIDTH
        new_height = int(VGA_WIDTH / aspect)
    else:
        # Image is taller - fit to height
        new_height = VGA_HEIGHT
        new_width = int(VGA_HEIGHT * aspect)

    # Center the image
    x_offset = (VGA_WIDTH - new_width) // 2
    y_offset = (VGA_HEIGHT - new_height) // 2

    # Resize with high-quality resampling
    img = img.resize((new_width, new_height), Image.Resampling.LANCZOS)

    print(f'Resized image to {new_width}x{new_height}', file=sys.stderr)
    print(f'Offset: ({x_offset}, {y_offset})', file=sys.stderr)

    # Create framebuffer (320x200 = 64000 bytes)
    framebuffer = bytearray(VGA_WIDTH * VGA_HEIGHT)

    # Convert each pixel
    for y in range(VGA_HEIGHT):
        for x in range(VGA_WIDTH):
            # Get source pixel (or black if outside image)
            src_x = x - x_offset
            src_y = y - y_offset

            if 0 <= src_x < new_width and 0 <= src_y < new_height:
                r, g, b = img.getpixel((src_x, src_y))
            else:
                r, g, b = 0, 0, 0  # Black border

            # Convert to RGB332 palette index
            color = rgb_to_332(r, g, b)
            framebuffer[y * VGA_WIDTH + x] = color

    # Write C header file
    with open(output_path, 'w') as f:
        f.write('/* Auto-generated VGA Mode 13h image data */\n')
        f.write('/* Original image: shakenfist.png */\n')
        f.write('/* VGA Mode 13h: 320x200, 256 colors (RGB332 palette) */\n\n')
        f.write('#ifndef VGA_IMAGE_H\n')
        f.write('#define VGA_IMAGE_H\n\n')
        f.write('#include <stdint.h>\n\n')
        f.write(f'#define IMAGE_WIDTH {new_width}\n')
        f.write(f'#define IMAGE_HEIGHT {new_height}\n')
        f.write(f'#define IMAGE_X_OFFSET {x_offset}\n')
        f.write(f'#define IMAGE_Y_OFFSET {y_offset}\n\n')

        # Framebuffer data
        f.write('/* Linear framebuffer data (320x200 = 64000 bytes) */\n')
        f.write('static const uint8_t vga_framebuffer[64000] = {\n')
        for i in range(0, 64000, 16):
            f.write('    ')
            f.write(', '.join(f'0x{b:02x}' for b in framebuffer[i:i+16]))
            f.write(',\n')
        f.write('};\n\n')

        f.write('#endif /* VGA_IMAGE_H */\n')

    print(f'Wrote {output_path}', file=sys.stderr)


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print(f'Usage: {sys.argv[0]} <input.png> <output.h>', file=sys.stderr)
        sys.exit(1)

    convert_image(sys.argv[1], sys.argv[2])
