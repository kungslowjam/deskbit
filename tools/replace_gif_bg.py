#!/usr/bin/env python3
"""
GIF Background Color Replacer
Replaces the background color of a GIF with a target color.
"""

from PIL import Image
import os

# Configuration
INPUT_GIF = "gif_files/pomo.gif"
OUTPUT_GIF = "gif_files/pomo_fixed.gif"

# Target background color (matches col_bg in ui_settings.c: 0xBCAD82)
# This is the beige color from the UI
TARGET_BG_COLOR = (0xBC, 0xAD, 0x82)  # RGB

# Source colors to replace (common background colors)
# We'll try to detect and replace white, near-white, or the most common corner color
SOURCE_COLORS_TO_REPLACE = [
    (255, 255, 255),  # Pure white
    (254, 254, 254),  # Near white
    (253, 253, 253),
    (252, 252, 252),
    (0, 0, 0),        # Pure black (if GIF has black bg)
]

def get_corner_color(img):
    """Get the color of the top-left corner pixel (likely background)"""
    if img.mode == 'P':
        # Palette mode - convert to RGB first
        rgb_img = img.convert('RGB')
        return rgb_img.getpixel((0, 0))
    elif img.mode == 'RGBA':
        return img.getpixel((0, 0))[:3]
    else:
        return img.getpixel((0, 0))

def replace_color_in_frame(frame, source_colors, target_color, tolerance=10):
    """Replace source colors with target color in a single frame"""
    # Convert to RGB if needed
    if frame.mode == 'P':
        frame = frame.convert('RGBA')
    elif frame.mode != 'RGBA':
        frame = frame.convert('RGBA')
    
    pixels = frame.load()
    width, height = frame.size
    
    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]
            
            for src_r, src_g, src_b in source_colors:
                if (abs(r - src_r) <= tolerance and 
                    abs(g - src_g) <= tolerance and 
                    abs(b - src_b) <= tolerance):
                    pixels[x, y] = (target_color[0], target_color[1], target_color[2], a)
                    break
    
    return frame

def process_gif(input_path, output_path, target_color):
    """Process all frames of a GIF and replace background color"""
    print(f"Opening GIF: {input_path}")
    
    with Image.open(input_path) as img:
        frames = []
        durations = []
        
        # Get the background color from first frame's corner
        corner_color = get_corner_color(img)
        print(f"Detected corner color: RGB{corner_color}")
        
        # Add corner color to the list of colors to replace
        colors_to_replace = SOURCE_COLORS_TO_REPLACE.copy()
        if corner_color not in colors_to_replace:
            colors_to_replace.insert(0, corner_color)
        
        print(f"Colors to replace: {colors_to_replace}")
        print(f"Target color: RGB{target_color}")
        
        # Process each frame
        frame_count = 0
        try:
            while True:
                # Get frame duration
                duration = img.info.get('duration', 100)
                durations.append(duration)
                
                # Process frame
                frame = img.copy()
                processed_frame = replace_color_in_frame(frame, colors_to_replace, target_color)
                frames.append(processed_frame)
                
                frame_count += 1
                print(f"Processed frame {frame_count}", end='\r')
                
                # Move to next frame
                img.seek(img.tell() + 1)
                
        except EOFError:
            pass  # End of frames
        
        print(f"\nTotal frames processed: {frame_count}")
        
        # Save the new GIF
        print(f"Saving to: {output_path}")
        frames[0].save(
            output_path,
            save_all=True,
            append_images=frames[1:],
            duration=durations,
            loop=0,
            disposal=2
        )
        
        print("Done!")
        return True

if __name__ == "__main__":
    if not os.path.exists(INPUT_GIF):
        print(f"Error: Input file not found: {INPUT_GIF}")
        exit(1)
    
    success = process_gif(INPUT_GIF, OUTPUT_GIF, TARGET_BG_COLOR)
    
    if success:
        print(f"\n✅ GIF saved to: {OUTPUT_GIF}")
        print(f"\nNext steps:")
        print(f"1. Review the output GIF")
        print(f"2. If it looks good, rename it to pomo.gif or update the conversion script")
        print(f"3. Run: python tools/gif_to_raw_c.py gif_files/pomo_fixed.gif")
