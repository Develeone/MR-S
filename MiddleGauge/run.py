from PIL import Image

def convert_to_rgb565(image_path, output_path):
    img = Image.open(image_path)
    img = img.convert('RGB')  # Ensure image is in RGB mode
    width, height = img.size
    pixels = list(img.getdata())

    with open(output_path, 'w') as f:
        for y in range(height):
            for x in range(width):
                r, g, b = pixels[(height - 1 - y) * width + x]  # Flip vertically
                # Convert RGB888 to RGB565
                rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                # Ensure correct byte order (little-endian)
                rgb565 = (rgb565 >> 8) | ((rgb565 & 0xFF) << 8)
                f.write(f'0x{rgb565:04X}, ')

                # New line for better readability (optional)
                if x % 16 == 15:
                    f.write('\n')

convert_to_rgb565('on_0.png', 'on_0.txt')
convert_to_rgb565('on_1.png', 'on_1.txt')
convert_to_rgb565('on_2.png', 'on_2.txt')
convert_to_rgb565('on_3.png', 'on_3.txt')
convert_to_rgb565('on_4.png', 'on_4.txt')
convert_to_rgb565('off_0.png', 'off_0.txt')
convert_to_rgb565('off_1.png', 'off_1.txt')
convert_to_rgb565('off_2.png', 'off_2.txt')
convert_to_rgb565('off_4.png', 'off_4.txt')