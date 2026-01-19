
import sys
import os
from PIL import Image

def generate_anim_c(input_path, output_path, name):
    try:
        img = Image.open(input_path)
    except Exception as e:
        print(f"Error opening image: {e}")
        return

    # Configuration
    RESIZE_W, RESIZE_H = 120, 120
    FRAME_SKIP = 2 # Keep every Nth frame
    
    frames = []
    try:
        while True:
            frames.append(img.copy())
            img.seek(img.tell() + 1)
    except EOFError:
        pass

    print(f"Total frames: {len(frames)}")
    
    # Process frames
    kept_frames = frames[::FRAME_SKIP]
    print(f"Kept frames: {len(kept_frames)}")

    c_content = f"""/**
 * @file {name}.c
 * @brief Auto-generated animation from {os.path.basename(input_path)}
 */

#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

"""
    
    dsc_names = []

    for idx, frame in enumerate(kept_frames):
        # Resize
        frame = frame.resize((RESIZE_W, RESIZE_H), Image.Resampling.BOX) # Box for pixel art look or LANCZOS? Let's use LANCZOS for smoothness
        frame = frame.resize((RESIZE_W, RESIZE_H), Image.Resampling.LANCZOS)
        frame = frame.convert("RGBA")
        
        # Name
        frame_name = f"{name}_f{idx}"
        dsc_names.append(frame_name)
        
        # Convert to RGB565
        data_bytes = []
        pixels = list(frame.getdata())
        for r, g, b, a in pixels:
            rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
            # Byte Swap (LE) -> CONFIG_LV_COLOR_16_SWAP=y
            low_byte = rgb565 & 0xFF
            high_byte = (rgb565 >> 8) & 0xFF
            data_bytes.append(high_byte)
            data_bytes.append(low_byte)
            data_bytes.append(a)
            
        # Write Map Array
        c_content += f"const LV_ATTRIBUTE_MEM_ALIGN uint8_t {frame_name}_map[] = {{\n"
        line = "    "
        for i, byte in enumerate(data_bytes):
            line += f"0x{byte:02x}, "
            if (i + 1) % 16 == 0:
                c_content += line + "\n"
                line = "    "
        if line != "    ":
            c_content += line + "\n"
        c_content += "};\n\n"
        
        # Write Descriptor
        c_content += f"""const lv_img_dsc_t {frame_name} = {{
    .header.always_zero = 0,
    .header.w = {RESIZE_W},
    .header.h = {RESIZE_H},
    .data_size = {len(data_bytes)},
    .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,
    .data = {frame_name}_map,
}};
"""

    # Animation Array
    c_content += f"\nconst lv_img_dsc_t* {name}_anim_imgs[] = {{\n"
    for dname in dsc_names:
        c_content += f"    &{dname},\n"
    c_content += "};\n"
    
    c_content += f"\nconst uint16_t {name}_count = {len(dsc_names)};\n"

    with open(output_path, "w") as f:
        f.write(c_content)
    
    print(f"Generated {output_path} with {len(dsc_names)} frames.")

if __name__ == "__main__":
    generate_anim_c(
        os.path.join("gif_files", "book.gif"), 
        os.path.join("components", "user_app", "book_anim.c"), 
        "book_anim"
    )
