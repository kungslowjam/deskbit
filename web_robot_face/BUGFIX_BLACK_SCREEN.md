# Animation Black Screen Fix

## Problem
When playing animations on the robot that were created using shape objects (rectangles, ellipses, lines), the screen remained black even though the animation was playing.

## Root Cause
The issue had two parts:

### 1. JavaScript Export Missing Shape Data
The `sendToRobot()` function in `script.js` was only sending pixel data to the bridge server, not shape data:

```javascript
// OLD CODE (line 1719-1722)
frames: frames.map(f => ({
    pixels: f.pixels.map((c, i) => c ? { i, c } : null).filter(p => p),
    duration: f.duration
}))
```

### 2. Python Export Not Rendering Shapes
The `export_to_project.py` script had a TODO comment indicating shapes weren't being rendered:

```python
# TODO: Render shapes to pixels (if needed)
# For now, we only handle pixel data
```

## Solution

### Fix 1: Updated JavaScript Export (script.js)
Added shape data to the export payload:

```javascript
frames: frames.map(f => ({
    pixels: f.pixels.map((c, i) => c ? { i, c } : null).filter(p => p),
    duration: f.duration,
    shapes: f.shapes ? f.shapes.map(s => ({
        type: s.type,
        x: s.x,
        y: s.y,
        width: s.width,
        height: s.height,
        color: s.color,
        opacity: s.opacity,
        blendMode: s.blendMode,
        rotation: s.rotation,
        lineEnd: s.lineEnd,
        id: s.id
    })) : []
}))
```

### Fix 2: Added Shape Rendering (export_to_project.py)
Implemented three new functions:

1. **`blend_colors(bg_rgb, fg_rgb, opacity)`** - Blends colors with opacity support
2. **`render_shape_to_pixels(pixels, shape, width, height)`** - Main rendering function that:
   - Renders filled rectangles
   - Renders filled ellipses using ellipse equation
   - Renders lines using Bresenham's algorithm
   - Supports opacity blending

The shapes are now "baked" into the pixel data before conversion to RGB565 format for the robot.

## Testing
To test the fix:

1. Create an animation in Robot Face Studio using shape objects (rectangles, ellipses, or lines)
2. Click "⚡ Send to Robot" 
3. Build and flash the firmware: `idf.py build flash`
4. The animation should now display correctly on the robot screen

## Files Modified
- `script.js` - Line 1713-1723 (sendToRobot function)
- `export_to_project.py` - Added shape rendering functions (lines 25-108) and updated frame processing (lines 102-106)

## Notes
- Shape objects are rendered in the order they appear in the frame
- Opacity blending is supported
- Blend modes from the web editor are not preserved (shapes are baked to pixels)
- The web preview will continue to work as before
