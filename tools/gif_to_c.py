from PIL import Image
import sys
import os

def convert_gif_to_c(gif_path, output_c_path, anim_name):
    try:
        im = Image.open(gif_path)
    except IOError:
        print(f"Error: Could not open {gif_path}")
        return

    frames = []
    try:
        while True:
            frames.append(im.copy())
            im.seek(len(frames))
    except EOFError:
        pass

    with open(output_c_path, 'w') as f:
        f.write(f'#include "lvgl.h"\n\n')
        
        frame_arrays = []
        for i, frame in enumerate(frames):
            # Convert to RGB565 format (approximate)
            frame = frame.convert('RGB')
            pixels = list(frame.getdata())
            width, height = frame.size
            
            array_name = f"{anim_name}_frame_{i}"
            frame_arrays.append(array_name)
            
            f.write(f"const uint8_t {array_name}_map[] = {{\n")
            
            # Simple RGB888 to RGB565 conversion for LVGL default
            # Note: This is a basic conversion, transparency handling might be needed depending on the GIF
            # For 16-bit color (LV_COLOR_DEPTH 16)
            data = []
            for r, g, b in pixels:
                # RGB565: RRRRRGGG GGGBBBBB
                rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                # Little Endian for ESP32/most MCUs
                data.append(rgb565 & 0xFF)        # Low byte
                data.append((rgb565 >> 8) & 0xFF) # High byte

            # Write hex data
            for j in range(0, len(data), 16):
                line = data[j:j+16]
                hex_str = ", ".join([f"0x{b:02x}" for b in line])
                f.write(f"    {hex_str},\n")
            
            f.write("};\n\n")
            
            f.write(f"const lv_img_dsc_t {array_name} = {{\n")
            f.write(f"  .header.always_zero = 0,\n")
            f.write(f"  .header.w = {width},\n")
            f.write(f"  .header.h = {height},\n")
            f.write(f"  .data_size = {len(data)},\n")
            f.write(f"  .header.cf = LV_IMG_CF_TRUE_COLOR,\n")
            f.write(f"  .data = {array_name}_map,\n")
            f.write(f"}};\n\n")

        f.write(f"const lv_img_dsc_t* {anim_name}_anim_imgs[] = {{\n")
        for name in frame_arrays:
            f.write(f"    &{name},\n")
        f.write("};\n\n")
        f.write(f"const uint16_t {anim_name}_count = {len(frame_arrays)};\n")

# Install PIL if not exists (This script assumes python environment has Pillow or PIL)
# pip install Pillow 

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python gif_converter.py <input_gif> <output_c> <anim_name>")
    else:
        convert_gif_to_c(sys.argv[1], sys.argv[2], sys.argv[3])
