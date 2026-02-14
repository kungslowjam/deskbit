from PIL import Image
import sys
import os

def rgb888_to_rgb565(r, g, b):
    # Word format: RRRRRGGG GGGBBBBB
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def main():
    if len(sys.argv) < 4:
        print("Usage: python png_to_c.py <input_image> <output_c_file> <var_name>")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]
    var_name = sys.argv[3]

    try:
        img = Image.open(input_path)
    except Exception as e:
        print(f"Error opening image: {e}")
        sys.exit(1)

    # Resize to 140x140
    img = img.resize((140, 140), Image.Resampling.LANCZOS)
    img = img.convert("RGBA")

    width, height = img.size
    data = []

    for y in range(height):
        for x in range(width):
            r, g, b, a = img.getpixel((x, y))
            
            rgb565 = rgb888_to_rgb565(r, g, b)
            
            high_byte = (rgb565 >> 8) & 0xFF
            low_byte = rgb565 & 0xFF
            
            # Swapped Byte order as seen in pomodoro_icon.c [High, Low, Alpha]
            data.append(high_byte)
            data.append(low_byte)
            data.append(a)

    with open(output_path, "w") as f:
        f.write("#if defined(LV_LVGL_H_INCLUDE_SIMPLE)\n")
        f.write("#include \"lvgl.h\"\n")
        f.write("#else\n")
        f.write("#include \"lvgl/lvgl.h\"\n")
        f.write("#endif\n\n")
        
        f.write("#ifndef LV_ATTRIBUTE_MEM_ALIGN\n")
        f.write("#define LV_ATTRIBUTE_MEM_ALIGN\n")
        f.write("#endif\n\n")
        
        f.write(f"const LV_ATTRIBUTE_MEM_ALIGN uint8_t {var_name}_map[] = {{\n")
        
        # Write hex data
        for i, b in enumerate(data):
            if i % 12 == 0:
                f.write("    ")
            f.write(f"0x{b:02x}, ")
            if i % 12 == 11:
                f.write("\n")
        
        f.write("\n};\n\n")
        
        f.write(f"const lv_img_dsc_t {var_name} = {{\n")
        f.write("    .header.always_zero = 0,\n")
        f.write(f"    .header.w = {width},\n")
        f.write(f"    .header.h = {height},\n")
        f.write(f"    .data_size = {len(data)},\n")
        f.write("    .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,\n")
        f.write(f"    .data = {var_name}_map,\n")
        f.write("};\n")

if __name__ == "__main__":
    main()
