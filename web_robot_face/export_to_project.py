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
    if not hex_color: return (255, 255, 255)
    hex_color = hex_color.lstrip('#')
    try:
        if len(hex_color) == 6:
            return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))
        elif len(hex_color) == 3:
            return tuple(int(hex_color[i]*2, 16) for i in (0, 1, 2))
    except Exception:
        pass
    return (255, 255, 255)

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
    
    # Anti-aliasing width (in pixels)
    aa_width = 1.2
    
    if shape_type == 'rect':
        half_w = w / 2
        half_h = h / 2
        
        for py in range(start_y, end_y):
            for px in range(start_x, end_x):
                dx = px - center_x
                dy = py - center_y
                
                local_x = abs(dx * cos_a + dy * sin_a)
                local_y = abs(-dx * sin_a + dy * cos_a)
                
                # Check distance to edges
                dist_x = local_x - half_w
                dist_y = local_y - half_h
                
                # Signed distance field (SDF) for a box
                dist = max(dist_x, dist_y)
                
                if dist < aa_width:
                    # Smoothing logic
                    coverage = 1.0
                    if dist > -aa_width:
                        coverage = (aa_width - dist) / (2 * aa_width)
                        coverage = max(0.0, min(1.0, coverage))
                    
                    final_opacity = opacity * coverage
                    if final_opacity > 0:
                        idx = py * width + px
                        if idx < len(pixels):
                            pixels[idx] = blend_colors(pixels[idx], fg_rgb, final_opacity)
    
    elif shape_type == 'ellipse':
        half_w = w / 2
        half_h = h / 2
        if half_w < 0.1 or half_h < 0.1: return

        for py in range(start_y, end_y):
            for px in range(start_x, end_x):
                dx = px - center_x
                dy = py - center_y
                
                lx = dx * cos_a + dy * sin_a
                ly = -dx * sin_a + dy * cos_a
                
                # Approximate distance to ellipse
                # Use the normalized radius approach for smoothing
                norm_dist = math.sqrt((lx * lx) / (half_w * half_w) + (ly * ly) / (half_h * half_h))
                
                # Convert normalized distance to pixel distance (approximate by average radius)
                avg_rad = (half_w + half_h) / 2
                pixel_dist = (norm_dist - 1.0) * avg_rad
                
                if pixel_dist < aa_width:
                    coverage = 1.0
                    if pixel_dist > -aa_width:
                        coverage = (aa_width - pixel_dist) / (2 * aa_width)
                        coverage = max(0.0, min(1.0, coverage))
                    
                    final_opacity = opacity * coverage
                    if final_opacity > 0:
                        idx = py * width + px
                        if idx < len(pixels):
                            pixels[idx] = blend_colors(pixels[idx], fg_rgb, final_opacity)
                            
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


def lerp(start, end, t):
    """Linear interpolation between two values"""
    if start is None or end is None:
        return start if start is not None else end
    return start + (end - start) * t


def find_matching_shape(shape, shapes_list):
    """Find a shape in the next frame with the same ID for interpolation"""
    if not shapes_list or 'id' not in shape:
        return None
    for s in shapes_list:
        if s.get('id') == shape.get('id'):
            return s
    return None


def interpolate_shape(shape1, shape2, t):
    """Interpolate between two shape states"""
    if shape1 is None:
        return None
    if shape2 is None:
        return shape1.copy() if isinstance(shape1, dict) else None
    
    result = shape1.copy()
    
    # Interpolate numeric properties
    for prop in ['x', 'y', 'width', 'height', 'rotation', 'opacity']:
        v1 = shape1.get(prop, 0)
        v2 = shape2.get(prop, v1)
        if v1 is not None and v2 is not None:
            result[prop] = lerp(float(v1), float(v2), t)
    
    # Keep non-interpolated properties from shape1
    result['type'] = shape1.get('type')
    result['color'] = shape1.get('color')
    result['id'] = shape1.get('id')
    result['lineEnd'] = shape1.get('lineEnd')
    
    # Interpolate lineEnd if both have it
    le1 = shape1.get('lineEnd')
    le2 = shape2.get('lineEnd')
    if isinstance(le1, dict) and isinstance(le2, dict):
        result['lineEnd'] = {
            'x': lerp(float(le1.get('x', 0)), float(le2.get('x', 0)), t),
            'y': lerp(float(le1.get('y', 0)), float(le2.get('y', 0)), t)
        }
    
    return result



def bake_animation_frames(frames, width, height, target_fps=30):
    """
    Generate interpolated frames from keyframes.
    This 'bakes' the smooth animation into discrete frames.
    """
    if not frames or len(frames) < 2:
        return frames  # Nothing to interpolate
    
    baked_frames = []
    total_duration_ms = sum(int(f.get('duration', 100)) for f in frames)
    frame_interval_ms = 1000 / target_fps  # e.g., 33.33ms for 30fps
    
    print(f"   🎬 Baking animation at {target_fps} FPS...")
    print(f"   Total duration: {total_duration_ms}ms")
    
    current_time = 0
    
    while current_time < total_duration_ms:
        # Find which keyframe we're in and the progress within it
        time_accum = 0
        frame_idx = 0
        local_time = 0
        
        for i, f in enumerate(frames):
            dur = int(f.get('duration', 100))
            if current_time >= time_accum and current_time < time_accum + dur:
                frame_idx = i
                local_time = current_time - time_accum
                break
            time_accum += dur
        else:
            # Past the last frame, use last frame
            frame_idx = len(frames) - 1
            local_time = 0
        
        current_frame = frames[frame_idx]
        next_frame_idx = (frame_idx + 1) % len(frames)
        next_frame = frames[next_frame_idx]
        
        frame_dur = int(current_frame.get('duration', 100))
        t = local_time / frame_dur if frame_dur > 0 else 0
        t = max(0.0, min(1.0, t))
        
        # Create interpolated frame
        interpolated = {
            'pixels': current_frame.get('pixels', []).copy(),  # Pixels don't interpolate well
            'shapes': [],
            'duration': int(frame_interval_ms)
        }
        
        # Interpolate shapes
        current_shapes = current_frame.get('shapes', []) or []
        next_shapes = next_frame.get('shapes', []) or []
        
        for shape in current_shapes:
            if shape is None: continue
            matching = find_matching_shape(shape, next_shapes)
            interpolated_shape = interpolate_shape(shape, matching, t)
            if interpolated_shape is not None:
                interpolated['shapes'].append(interpolated_shape)
        
        baked_frames.append(interpolated)
        current_time += frame_interval_ms

    
    print(f"   Generated {len(baked_frames)} interpolated frames")
    return baked_frames

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
    # Bake animation (interpolate frames for smooth playback)
    # Only bake if there are 2+ frames with shapes to interpolate
    has_shapes = any(f.get('shapes') for f in frames)
    if len(frames) >= 2 and has_shapes:
        # Lower FPS + Runtime interpolation = Less rounding artifacts
        target_fps = json_data.get('fps', 20)  # 20 FPS - runtime interpolation handles the rest
        target_fps = max(10, min(target_fps, 60))  # Allow 10-60 FPS
        frames = bake_animation_frames(frames, width, height, target_fps)

    
    print(f"   Final Frames: {len(frames)}")

    total_size_mb = (len(frames) * width * height * 2) / (1024 * 1024)

    c_content = f"""/**
 * @file {anim_name}.c
 * @brief Auto-generated VECTOR animation from Robot Face Studio
 */

#include "anim_manager.h"

"""
    
    # Mapping for shape types
    type_map = {
        'rect': 'SHAPE_RECT',
        'ellipse': 'SHAPE_ELLIPSE',
        'line': 'SHAPE_LINE',
        'text': 'SHAPE_TEXT'
    }

    # Generate shape arrays for each frame
    frame_list = []
    
    for idx, frame in enumerate(frames):
        if not frame: continue # Skip null frames
        
        shapes = frame.get('shapes', [])
        frame_var = f"{anim_name}_f{idx}_shapes"
        
        if shapes:
            c_content += f"static const anim_shape_t {frame_var}[] = {{\n"
            for s in shapes:
                if not s: continue # Skip null shapes
                
                t = type_map.get(s.get('type'), 'SHAPE_RECT')
                x = s.get('x', 0)
                y = s.get('y', 0)
                w = s.get('width', 0)
                h = s.get('height', 0)
                rot = s.get('rotation', 0)
                opacity = s.get('opacity', 1.0)
                
                # Color conversion (#FFFFFF -> 0xFFFFFF)
                color_str = s.get('color', '#FFFFFF')
                color_hex = color_str.replace('#', '0x') if color_str else '0xFFFFFF'
                
                # Line specific handling
                line_end = s.get('lineEnd')
                if not isinstance(line_end, dict):
                    line_end = {}
                
                x2 = line_end.get('x', 0)
                y2 = line_end.get('y', 0)
                
                # Text specific handling
                text_content = s.get('text', '')
                font_size = s.get('fontSize', 14)
                
                if s.get('type') == 'text' and text_content:
                    # Escape quotes in text
                    text_escaped = text_content.replace('\\', '\\\\').replace('"', '\\"')
                    c_content += f'    {{ {t}, {x:.2f}f, {y:.2f}f, {w:.2f}f, {h:.2f}f, {rot:.2f}f, {color_hex}, {opacity:.2f}f, {x2:.2f}f, {y2:.2f}f, "{text_escaped}", {font_size} }},\n'
                else:
                    c_content += f"    {{ {t}, {x:.2f}f, {y:.2f}f, {w:.2f}f, {h:.2f}f, {rot:.2f}f, {color_hex}, {opacity:.2f}f, {x2:.2f}f, {y2:.2f}f, NULL, 0 }},\n"
            c_content += "};\n\n"

        
        frame_list.append({
            'var': frame_var if shapes else 'NULL',
            'count': len(shapes),
            'duration': frame.get('duration', 100)
        })

    # Generate frame array
    c_content += f"static const anim_vector_frame_t {anim_name}_frames[] = {{\n"
    for f in frame_list:
        c_content += f"    {{ {f['var']}, {f['count']}, {int(f['duration'])} }},\n"
    c_content += "};\n\n"

    # Main animation struct
    c_content += f"""const anim_vector_t {anim_name}_data = {{
    .name = "{anim_name}",
    .frames = {anim_name}_frames,
    .frame_count = {len(frame_list)}
}};
"""

    # Write C file
    c_file = output_dir / f"{anim_name}.c"
    with open(c_file, 'w', encoding='utf-8') as f:
        f.write(c_content)
    
    print(f"✅ Generated: {c_file} (Vector Format)")
    return anim_name

def generate_h_file(anim_name, output_dir):
    """Generate H file for Vector animation"""
    h_content = f"""#ifndef {anim_name.upper()}_H
#define {anim_name.upper()}_H

#include "anim_manager.h"

extern const anim_vector_t {anim_name}_data;

#endif
"""
    h_file = output_dir / f"{anim_name}.h"
    with open(h_file, 'w', encoding='utf-8') as f:
        f.write(h_content)
    print(f"✅ Generated: {h_file}")

def update_registry(anim_name, registry_file, width):
    """Automatically register the VECTOR animation in C code"""
    if not registry_file.exists():
        print("⚠️ anim_registry.c not found")
        return

    with open(registry_file, 'r', encoding='utf-8') as f:
        content = f.read()

    # 1. Add Include
    include_line = f'#include "animations/{anim_name}.h"'
    if include_line not in content:
        # Insert near top
        if '// Include your animation headers here' in content:
            content = content.replace('// Include your animation headers here', f'// Include your animation headers here\n{include_line}')
        else:
            # Fallback insertion (after first include)
            lines = content.split('\n')
            for i, line in enumerate(lines):
                if line.startswith('#include'):
                    lines.insert(i + 1, include_line)
                    break
            content = '\n'.join(lines)
        print(f"✅ Added include to anim_registry.c")

    # 2. Add Vector Register Call
    register_line = f'    anim_manager_register_vector(&{anim_name}_data);'
    
    if register_line not in content:
        # Insert inside register_all_animations function
        if 'void register_all_animations(void) {' in content:
            content = content.replace('void register_all_animations(void) {', f'void register_all_animations(void) {{\n{register_line}')
            print(f"✅ Registered vector animation in anim_registry.c")
        else:
             print("⚠️ Could not find register function")

    with open(registry_file, 'w', encoding='utf-8') as f:
        f.write(content)

def export_rbat(json_data, output_path):
    """Export animation data to binary .rbat format"""
    import struct
    
    width = json_data.get('width', 466)
    height = json_data.get('height', 466)
    frames = json_data.get('frames', [])
    
    # Header: Magic(4), Version(2), Width(2), Height(2), FrameCount(2), Reserved(4) = 16 bytes
    header = struct.pack('<4sHHHH L', b'RBAT', 1, width, height, len(frames), 0)
    
    with open(output_path, 'wb') as f:
        f.write(header)
        
        for frame in frames:
            duration = int(frame.get('duration', 100))
            shapes = frame.get('shapes', [])
            
            # Frame: Duration(2), ShapeCount(2)
            f.write(struct.pack('<HH', duration, len(shapes)))
            
            for s in shapes:
                stype = 0 # Rect
                if s.get('type') == 'ellipse': stype = 1
                elif s.get('type') == 'line': stype = 2
                elif s.get('type') == 'text': stype = 3
                
                x = float(s.get('x', 0))
                y = float(s.get('y', 0))
                w = float(s.get('width', 0))
                h = float(s.get('height', 0))
                rot = float(s.get('rotation', 0))
                opacity = float(s.get('opacity', 1.0))
                
                rgb = hex_to_rgb(s.get('color', '#FFFFFF'))
                color = (rgb[0] << 16) | (rgb[1] << 8) | rgb[2]
                
                le = s.get('lineEnd', {})
                x2 = float(le.get('x', 0))
                y2 = float(le.get('y', 0))
                
                font_size = int(s.get('fontSize', 14))
                text = s.get('text', '')
                text_bytes = text.encode('utf-8')
                text_len = len(text_bytes)
                
                # Shape: Type(1), x(f), y(f), w(f), h(f), rot(f), opa(f), color(I), x2(f), y2(f), fs(B), tlen(H) = 43 bytes + text
                shape_data = struct.pack('<B ffffff I ff B H', 
                    stype, x, y, w, h, rot, opacity, color, x2, y2, font_size, text_len)
                f.write(shape_data)
                if text_len > 0:
                    f.write(text_bytes)
    
    print(f"✅ Generated Binary: {output_path} (RBAT Format)")

def update_cmake(anim_name, cmake_file):
    """Update CMakeLists.txt to include new animation"""
    if not cmake_file.exists():
        print(f"⚠️  CMakeLists.txt not found: {cmake_file}")
        return
    
    with open(cmake_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    file_entry = f'"animations/{anim_name}.c"'
    if file_entry in content:
        return
    
    if 'SRCS ' in content:
        import re
        pattern = r'(SRCS\s+[^)]+?)(\s+PRIV_REQUIRES|\s+REQUIRES|\s+INCLUDE_DIRS|\n|\))'
        
        def add_file(match):
            srcs_part = match.group(1)
            rest = match.group(2)
            return f'{srcs_part} {file_entry}{rest}'
        
        new_content = re.sub(pattern, add_file, content, count=1)
        if new_content != content:
            with open(cmake_file, 'w', encoding='utf-8') as f:
                f.write(new_content)
            print(f"✅ Updated CMakeLists.txt")

def main():
    if len(sys.argv) < 2:
        print("Usage: python export_to_project.py <animation.json>")
        sys.exit(1)
    
    json_file = Path(sys.argv[1])
    if not json_file.exists():
        print(f"❌ File not found: {json_file}")
        sys.exit(1)
    
    with open(json_file, 'r', encoding='utf-8') as f:
        json_data = json.load(f)
    
    # Paths
    script_dir = Path(__file__).parent
    project_dir = script_dir.parent
    user_app_dir = project_dir / "components" / "user_app"
    anim_dir = user_app_dir / "animations"
    
    if not anim_dir.exists(): anim_dir.mkdir(parents=True)
    
    # Generate
    anim_name = generate_c_file(json_data, anim_dir)
    if anim_name:
        generate_h_file(anim_name, anim_dir)
        update_cmake(anim_name, user_app_dir / "CMakeLists.txt")
        update_registry(anim_name, user_app_dir / "anim_registry.c", json_data.get('width', 466))
        
        # Also export binary version
        export_rbat(json_data, anim_dir / f"{anim_name}.rbat")
        
        print(f"\n🎉 Export Complete (C + Binary)!")

if __name__ == "__main__":
    main()
