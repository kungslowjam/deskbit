# RobotStudio User Manual Summary

Based on the RobotStudio User Manual (Support Eilik) - EN.pdf, here is a summary of the key features and workflows for controlling Eilik's motion and expressions.

## 1. Introduction
RobotStudio is the official motion and animation editing software for Eilik.
- **Platform:** Windows 10+
- **Languages:** English, Simplified Chinese
- **Purpose:** Create custom motions and facial animations.

## 2. Connection
- **Hardware:** Connect via USB-C.
- **Firmware:** Requires Eilik RFV 13.0 or above.
- **Software:**
    - Select Serial Port.
    - Baud Rate: 1,000,000.
    - Click "Connect".

## 3. Programming Interface
The interface is Timeline-based.

### Servo Mapping
- **ID 1:** Right Hand
- **ID 2:** Left Hand
- **ID 3:** Body
- **ID 4:** Head

### Motion & Timeline
1.  **Motion Frames:** Middle section of timeline. Controls physical servos.
2.  **Animation Frames:** Bottom section. Controls facial OLED display.
3.  **Time Scale:** Top section.

### Editing Process
- **Servo Control:** Use "Servo Dials" or enter numbers.
- **Keyframing:** MUST click "New Frame" or "Save Frame" to persist changes to the timeline.
- **Speed:** Adjustable acceleration (Constant, Fast, Medium, Slow).

## 4. Animation Editing (OLED)
- **Canvas:** Draw pixel-based animations.
- **Tools:** Brush, Eraser, Clear.
- **Sync:** Align animation frames with motion frames on the timeline to synchronize face and body.

## 5. File Management
- **Projects:** Save and load project files to manage different behavior sets.
