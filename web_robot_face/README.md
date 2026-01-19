# Robot Face Studio - User Guide

This tool allows you to create pixel-perfect facial animations for your Eilik robot (or any LVGL-based display) directly in your browser.

## How to Open
Double-click `index.html` to open the application in your web browser.

## Quick Start

### 1. File Management (Sidebar)
- **Local Files:** Your animations are listed on the left.
- **Create New:** Click the **`+`** button next to "Local Files" to create a new empty animation.
- **Switching:** Click on a file name to switch between different animations.

### 2. Drawing Tools (Toolbar)
- **✏️ Pen:** Click to draw pixels.
- **🧼 Eraser:** Click to remove pixels.
- **🪣 Fill:** Flood fill an area with color.
- **🗑️ Clear:** Clear the entire current frame.
- **🌗 Invert:** Invert all colors on the current frame.
- **Color Picker:** Click the colored circles to choose a color.

### 3. Animation Control (Timeline)
- **+ Add Frame:** Create a blank new frame at the end.
- **Duplicate:** Copy the current frame (useful for making small changes).
- **Delete:** Remove the current frame.
- **Play/Stop:** Preview your animation properly.
- **Speed:** Adjust the FPS (Frames Per Second).

### 4. Settings
- **W / H:** Set the resolution (Default: 466x466 for AMOLED).
- **Round:** Toggle the circular mask to preview how it looks on a round screen.

### 5. Exporting to Robot
1.  When your animation is ready, click **"Export to C"**.
2.  Save the downloaded file (e.g., `custom_anim.c`).
3.  Copy this file to your robot project folder (`components/user_app`).
4.  Use `ui_animimg_set_src(anim_obj, (lv_img_dsc_t**)my_anim_imgs, my_anim_count);` in your code.
