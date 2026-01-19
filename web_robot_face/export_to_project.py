#!/usr/bin/env python3
"""
Auto-export animation from Robot Face Studio to ESP32 project
Usage: python export_to_project.py animation.json
"""

import json
import sys
import os
from pathlib import Path

def rgb565_convert(r, g, b):
    """Convert RGB888 to RGB565"""
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F
    rgb565 = (r5 << 11) | (g6 << 5) | b5
    # Return High Byte, Low Byte (Big Endian - commonly needed for proper color)
    return (rgb565 >> 8) & 0xFF, rgb565 & 0xFF

def hex_to_rgb(hex_color):
    """Convert #RRGGBB to (r, g, b)"""
    hex_color = hex_color.lstrip('#')
    return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))

def blend_colors(bg_rgb, fg_rgb, opacity):
    """Blend foreground color onto background with opacity"""
    r = int(bg_rgb[0] * (1 - opacity) + fg_rgb[0] * opacity)
    g = int(bg_rgb[1] * (1 - opacity) + fg_rgb[1] * opacity)
    b = int(bg_rgb[2] * (1 - opacity) + fg_rgb[2] * opacity)
    return (r, g, b)

def render_shape_to_pixels(pixels, shape, width, height):
    """Render a shape object to the pixel array"""
    if not shape or 'type' not in shape:
        return
    
    import math
    
    shape_type = shape['type']
    # Use float for precision during calculation
    sx = float(shape.get('x', 0))
    sy = float(shape.get('y', 0))
    w = float(shape.get('width', 0))
    h = float(shape.get('height', 0))
    rotation = float(shape.get('rotation', 0)) # Degrees
    
    color = shape.get('color', '#FFFFFF')
    opacity = float(shape.get('opacity', 1.0))
    
    fg_rgb = hex_to_rgb(color)
    
    # Calculate bounding box for optimization
    # Simple max dimension to cover rotated area
    max_dim = math.sqrt(w*w + h*h)
    center_x = sx + w / 2
    center_y = sy + h / 2
    
    start_x = max(0, int(center_x - max_dim / 2))
    end_x = min(width, int(center_x + max_dim / 2 + 1))
    start_y = max(0, int(center_y - max_dim / 2))
    end_y = min(height, int(center_y + max_dim / 2 + 1))
    
    rad = math.radians(rotation)
    cos_a = math.cos(rad)
    sin_a = math.sin(rad)
    
    if shape_type == 'rect':
        half_w = w / 2
        half_h = h / 2
        
        for py in range(start_y, end_y):
            for px in range(start_x, end_x):
                # Rotate point back to axis-aligned space relative to center
                dx = px - center_x
                dy = py - center_y
                
                # Inverse rotation to check axis-aligned bounds
                # x' = x*cos(-a) - y*sin(-a) = x*cos(a) + y*sin(a)
                # y' = x*sin(-a) + y*cos(-a) = -x*sin(a) + y*cos(a)
                local_x = dx * cos_a + dy * sin_a
                local_y = -dx * sin_a + dy * cos_a
                
                if -half_w <= local_x <= half_w and -half_h <= local_y <= half_h:
                    idx = py * width + px
                    if idx < len(pixels):
                        if opacity < 1.0:
                            pixels[idx] = blend_colors(pixels[idx], fg_rgb, opacity)
                        else:
                            pixels[idx] = list(fg_rgb)
    
    elif shape_type == 'ellipse':
        half_w = w / 2
        half_h = h / 2
        
        # Avoid division by zero
        if half_w < 0.1 or half_h < 0.1: return

        for py in range(start_y, end_y):
            for px in range(start_x, end_x):
                dx = px - center_x
                dy = py - center_y
                
                local_x = dx * cos_a + dy * sin_a
                local_y = -dx * sin_a + dy * cos_a
                
                # Ellipse equation: (x/rx)^2 + (y/ry)^2 <= 1
                if (local_x * local_x) / (half_w * half_w) + (local_y * local_y) / (half_h * half_h) <= 1:
                    idx = py * width + px
                    if idx < len(pixels):
                        if opacity < 1.0:
                            pixels[idx] = blend_colors(pixels[idx], fg_rgb, opacity)
                        else:
                            pixels[idx] = list(fg_rgb)
                            
    elif shape_type == 'line' and 'lineEnd' in shape:
        # Drawing rotated thick lines is complex, falling back to simple Bresenham for endpoints
        # Real rotation would involve drawing a rotated rectangle
        
        line_end = shape['lineEnd']
        x1_base = sx
        y1_base = sy
        x2_base = float(line_end.get('x', 0))
        y2_base = float(line_end.get('y', 0))
        
        # Actually, lines in the editor are just define by start (x,y) and end (x,y)
        # Rotation applies to the group or visual only? 
        # Usually lines are not 'rotated' by a property, but by moving endpoints.
        # But if the user applied a rotation transform to a line object...
        # For simplicity, let's treat lines as lines connected by endpoints.
        
        # But if rotation is present, we must rotate the endpoints around the center?
        # NO, usually lines in fabric.js/canvas just use endpoints. 
        # If 'rotation' property exists, it rotates around the center of the bounding box.
        
        # Let's simplify: Standard Bresenham (ignore rotation for lines for now as they are rarely rotated as objects)
        
        x1, y1 = int(x1_base), int(y1_base)
        x2, y2 = int(x2_base), int(y2_base)
        
        # ... (Existing Bresenham Code) ...
        dx = abs(x2 - x1)
        dy = abs(y2 - y1)
        sx = 1 if x1 < x2 else -1
        sy = 1 if y1 < y2 else -1
        err = dx - dy
        
        curr_x, curr_y = x1, y1
        
        while True:
            if 0 <= curr_x < width and 0 <= curr_y < height:
                idx = curr_y * width + curr_x
                if idx < len(pixels):
                    if opacity < 1.0:
                        pixels[idx] = blend_colors(pixels[idx], fg_rgb, opacity)
                    else:
                        pixels[idx] = list(fg_rgb)
            
            if curr_x == x2 and curr_y == y2:
                break
            
            e2 = 2 * err
            if e2 > -dy:
                err -= dy
                curr_x += sx
            if e2 < dx:
                err += dx
                curr_y += sy


def generate_c_file(json_data, output_dir):
    """Generate C file from JSON animation data"""
    
    width = json_data.get('width', 466)
    height = json_data.get('height', 466)
    frames = json_data.get('frames', [])
    
    if not frames:
        print("❌ No frames found in JSON!")
        return None
    
    # Get animation name
    if 'name' in json_data and json_data['name']:
        anim_name = json_data['name']
    else:
        # Ask user for animation name (Only if not provided automatically)
        anim_name = "exported_anim"
        try:
            print(f"Animation name (default: {anim_name}): ", end='')
            # Flush stdout to ensure prompt appears
            sys.stdout.flush() 
            # Check if stdin is interactive before asking
            if sys.stdin.isatty():
                user_name = input().strip()
                if user_name:
                    anim_name = user_name
        except Exception:
            pass # Keep default if input fails

    # Sanitize name
    anim_name = ''.join(c if c.isalnum() or c == '_' else '_' for c in anim_name.lower())
    
    print(f"\n📝 Generating: {anim_name}.c")
    print(f"   Size: {width}x{height}")
    print(f"   Frames: {len(frames)}")
    
    # Start building C content
    c_content = f"""/**
 * @file {anim_name}.c
 * @brief Auto-generated animation from Robot Face Studio
 * Compatible with LVGL (RGB565 Format)
 * Generated: {__import__('datetime').datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
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
    
    frame_names = []
    
    # Generate each frame
    for idx, frame_data in enumerate(frames):
        frame_name = f"{anim_name}_f{idx}"
        frame_names.append(frame_name)
        
        print(f"   Processing frame {idx + 1}/{len(frames)}...", end='\r')
        
        # Create pixel array (black background)
        pixels = [[0, 0, 0] for _ in range(width * height)]
        
        # Fill in pixels from JSON
        if 'pixels' in frame_data:
            for pixel in frame_data['pixels']:
                if pixel and 'i' in pixel and 'c' in pixel:
                    idx_px = pixel['i']
                    color = pixel['c']
                    if idx_px < len(pixels):
                        pixels[idx_px] = list(hex_to_rgb(color))
        
        
        # Render shapes to pixels
        if 'shapes' in frame_data and frame_data['shapes']:
            for shape in frame_data['shapes']:
                render_shape_to_pixels(pixels, shape, width, height)
        
        # Convert to RGB565
        c_content += f"const LV_ATTRIBUTE_MEM_ALIGN uint8_t {frame_name}_map[] = {{\n"
        
        line = "    "
        byte_count = 0
        
        for r, g, b in pixels:
            low, high = rgb565_convert(r, g, b)
            line += f"0x{low:02x}, 0x{high:02x}, "
            byte_count += 2
            
            if byte_count >= 24:
                c_content += line + "\n"
                line = "    "
                byte_count = 0
        
        if line.strip() != "":
            c_content += line + "\n"
        
        c_content += f"}};\n\n"
        
        # Frame descriptor
        c_content += f"""const lv_img_dsc_t {frame_name} = {{
    .header.always_zero = 0,
    .header.w = {width},
    .header.h = {height},
    .data_size = {width * height * 2},
    .header.cf = LV_IMG_CF_TRUE_COLOR,
    .data = {frame_name}_map,
}};

"""
    
    print(f"   Processing frame {len(frames)}/{len(frames)}... ✅")
    
    # Frame array
    c_content += f"const lv_img_dsc_t* {anim_name}_frames[] = {{\n"
    for name in frame_names:
        c_content += f"    &{name},\n"
    c_content += f"}};\n\n"
    c_content += f"const uint8_t {anim_name}_frame_count = {len(frames)};\n"
    
    # Write C file
    c_file = output_dir / f"{anim_name}.c"
    with open(c_file, 'w', encoding='utf-8') as f:
        f.write(c_content)
    
    print(f"\n✅ Generated: {c_file}")
    
    # Generate H file
    h_content = f"""/**
 * @file {anim_name}.h
 * @brief Header for {anim_name}.c
 */

#ifndef {anim_name.upper()}_H
#define {anim_name.upper()}_H

#include "lvgl/lvgl.h"

extern const lv_img_dsc_t* {anim_name}_frames[];
extern const uint8_t {anim_name}_frame_count;

#endif // {anim_name.upper()}_H
"""
    
    h_file = output_dir / f"{anim_name}.h"
    with open(h_file, 'w', encoding='utf-8') as f:
        f.write(h_content)
    
    print(f"✅ Generated: {h_file}")
    
    return anim_name

def update_cmake(anim_name, cmake_file):
    """Update CMakeLists.txt to include new animation"""
    
    if not cmake_file.exists():
        print(f"⚠️  CMakeLists.txt not found: {cmake_file}")
        return
    
    with open(cmake_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Check if already exists
    if f'"{anim_name}.c"' in content:
        print(f"ℹ️  {anim_name}.c already in CMakeLists.txt")
        return
    
    # Find SRCS line and add our file
    import re
    pattern = r'(SRCS\s+[^)]+)'
    
    def add_file(match):
        srcs = match.group(1)
        # Add before the closing quote
        if srcs.strip().endswith('"'):
            return srcs[:-1] + f' "{anim_name}.c"'
        else:
            return srcs + f' "{anim_name}.c"'
    
    new_content = re.sub(pattern, add_file, content)
    
    if new_content != content:
        # Backup original
        backup = cmake_file.with_suffix('.txt.backup')
        with open(backup, 'w', encoding='utf-8') as f:
            f.write(content)
        
        with open(cmake_file, 'w', encoding='utf-8') as f:
            f.write(new_content)
        
        print(f"✅ Updated CMakeLists.txt (backup: {backup.name})")
    else:
        print(f"⚠️  Could not update CMakeLists.txt automatically")

def main():
    if len(sys.argv) < 2:
        print("Usage: python export_to_project.py <animation.json>")
        print("\nOr drag and drop a JSON file onto this script!")
        input("\nPress Enter to exit...")
        sys.exit(1)
    
    json_file = Path(sys.argv[1])
    
    if not json_file.exists():
        print(f"❌ File not found: {json_file}")
        input("\nPress Enter to exit...")
        sys.exit(1)
    
    print(f"📂 Loading: {json_file}")
    
    try:
        with open(json_file, 'r', encoding='utf-8') as f:
            json_data = json.load(f)
    except Exception as e:
        print(f"❌ Error loading JSON: {e}")
        input("\nPress Enter to exit...")
        sys.exit(1)
    
    # Output directory (user_app/animations)
    script_dir = Path(__file__).parent
    project_dir = script_dir.parent
    user_app_dir = project_dir / "components" / "user_app"
    anim_dir = user_app_dir / "animations"
    
    if not user_app_dir.exists():
        print(f"❌ User app directory not found: {user_app_dir}")
        input("\nPress Enter to exit...")
        sys.exit(1)
        
    # Create animation directory if needed
    if not anim_dir.exists():
        anim_dir.mkdir(parents=True)
        print(f"📁 Created animations folder: {anim_dir}")
    
    print(f"📁 Target: {anim_dir}")
    
    # Generate files
    anim_name = generate_c_file(json_data, anim_dir)
    
    if anim_name:
        # 1. Update CMakeLists.txt
        cmake_file = user_app_dir / "CMakeLists.txt"
        update_cmake(anim_name, cmake_file)
        
        # 2. Update anim_registry.c (The Magic Step!)
        registry_file = user_app_dir / "anim_registry.c"
        update_registry(anim_name, registry_file, json_data.get('width', 466))
        
        print(f"\n🎉 DONE! Zero manual editing required.")
        print(f"   Just run 'idf.py build' now!")
    
    input("\nPress Enter to exit...")

def update_registry(anim_name, registry_file, width):
    """Automatically register the animation in C code"""
    if not registry_file.exists():
        print("⚠️ anim_registry.c not found")
        return

    with open(registry_file, 'r', encoding='utf-8') as f:
        content = f.read()

    # 1. Add Include
    include_line = f'#include "animations/{anim_name}.h"'
    if include_line not in content:
        # Insert after the blink include or near top
        if '#include "animations/blink_anim.h"' in content:
            content = content.replace('#include "animations/blink_anim.h"', f'#include "animations/blink_anim.h"\n{include_line}')
        else:
            # Fallback insertion
            content = content.replace('// Include your animation headers here', f'// Include your animation headers here\n{include_line}')
        print(f"✅ Added include to anim_registry.c")

    # 2. Add Register Call
    # Register with duration based on frame count (approx 100ms per frame default)
    # or just use 500ms default
    register_line = f'    anim_manager_register("{anim_name.replace("_anim", "")}", {anim_name}_frames, {anim_name}_frame_count, 500);'
    
    if register_line not in content:
        # Insert inside register_all_animations function
        if 'void register_all_animations(void) {' in content:
            content = content.replace('void register_all_animations(void) {', f'void register_all_animations(void) {{\n{register_line}')
            print(f"✅ Registered animation in anim_registry.c")
        else:
             print("⚠️ Could not find register function")

    with open(registry_file, 'w', encoding='utf-8') as f:
        f.write(content)

def update_cmake(anim_name, cmake_file):
    """Update CMakeLists.txt to include new animation"""
    
    if not cmake_file.exists():
        print(f"⚠️  CMakeLists.txt not found: {cmake_file}")
        return
    
    with open(cmake_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Check if already exists (look for the exact file path relative to CMakeLists)
    file_entry = f'"animations/{anim_name}.c"'
    
    if file_entry in content:
        print(f"ℹ️  CMakeLists already has {anim_name}.c")
        return
    
    # Find SRCS line end and append
    if 'SRCS ' in content:
        import re
        # Look for the SRCS line and append the new file with proper spacing
        # Match the SRCS line and add the new file before the next keyword or closing paren
        pattern = r'(SRCS\s+[^)]+?)(\s+PRIV_REQUIRES|\s+REQUIRES|\s+INCLUDE_DIRS|\n)'
        
        def add_file(match):
            srcs_part = match.group(1)
            rest = match.group(2)
            # Add the file with proper spacing
            return f'{srcs_part} {file_entry}{rest}'
        
        new_content = re.sub(pattern, add_file, content, count=1)
        
        if new_content != content:
            content = new_content
        else:
            # Fallback: try simpler pattern
            print("⚠️  Could not automatically update CMakeLists.txt")

    with open(cmake_file, 'w', encoding='utf-8') as f:
        f.write(content)  
    print(f"✅ Updated CMakeLists.txt")

if __name__ == "__main__":
    main()
