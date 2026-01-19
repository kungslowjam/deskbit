
import sys
import os
from PIL import Image

def convert_to_c_array(input_path, output_path, name):
    try:
        img = Image.open(input_path)
    except Exception as e:
        print(f"Error opening image: {e}")
        return

    # Resize to 140x140 to match UI
    if img.size != (140, 140):
        print(f"Resizing from {img.size} to (140, 140)")
        img = img.resize((140, 140), Image.Resampling.LANCZOS)

    img = img.convert("RGBA")
    width, height = img.size
    
    # 16-bit Color (RGB565) + 8-bit Alpha
    # Format: [Color1, Color2, Alpha] per pixel
    data_bytes = []
    
    pixels = list(img.getdata())
    
    for r, g, b, a in pixels:
        # Convert RGB888 to RGB565
        # Red: 5 bits, Green: 6 bits, Blue: 5 bits
        rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
        
        # Little Endian Integers
        low_byte = rgb565 & 0xFF
        high_byte = (rgb565 >> 8) & 0xFF
        
        # CHECKED: CONFIG_LV_COLOR_16_SWAP=y in sdkconfig
        # This means the system expects the bytes to be swapped (High byte first)
        data_bytes.append(high_byte)
        data_bytes.append(low_byte)
        
        # Alpha byte follows color
        data_bytes.append(a)

    c_content = f"""/**
 * @file {name}.c
 * @brief Auto-generated from {os.path.basename(input_path)}
 * SWAPPED BYTES due to CONFIG_LV_COLOR_16_SWAP=y
 */

#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

const LV_ATTRIBUTE_MEM_ALIGN uint8_t {name}_map[] = {{
"""
    
    # Add data bytes
    line = "    "
    for i, byte in enumerate(data_bytes):
        line += f"0x{byte:02x}, "
        if (i + 1) % 12 == 0:
            c_content += line + "\n"
            line = "    "
    if line != "    ":
        c_content += line + "\n"

    c_content += f"""}};

const lv_img_dsc_t {name} = {{
    .header.always_zero = 0,
    .header.w = {width},
    .header.h = {height},
    .data_size = {len(data_bytes)},
    .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,
    .data = {name}_map,
}};
"""

    with open(output_path, "w") as f:
        f.write(c_content)
    
    print(f"Generated {output_path} with Byte Swap")

if __name__ == "__main__":
    # Pomodoro Icon
    convert_to_c_array(r"c:\Users\hello\Desktop\desktop\image\pomodoro-technique.png", 
                       r"c:\Users\hello\Desktop\desktop\components\user_app\pomodoro_icon.c",
                       "pomodoro_icon")
