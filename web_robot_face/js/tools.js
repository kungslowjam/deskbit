import { state } from './state.js';
import { getPixelIndex, getCoordsFromEvent, rgbToHex } from './utils.js';
// renderer imports removed to prevent circular dependency
import { updateFrameInfo, updateToolButtons, updateLayersPanel, showToast, updateInteractionEditor } from './ui.js';
import { renderTimeline } from './timeline.js';
import { Shape, Frame } from './models.js';
import { undo, redo, saveUndo } from './history.js';

let canvas = null;

export function initTools(c) {
    canvas = c;
    setupEventListeners();
}

// Drawing Logic
export function drawPixel(x, y, color) {
    if (x < 0 || x >= state.GRID_WIDTH || y < 0 || y >= state.GRID_HEIGHT) return;
    const frame = state.frames[state.currentFrameIndex];
    if (!frame) return;

    const idx = getPixelIndex(x, y);
    if (idx !== -1) {
        if (state.isSymmetryEnabled) {
            const symX = state.GRID_WIDTH - 1 - x;
            const symIdx = getPixelIndex(symX, y);
            if (symIdx !== -1) {
                frame.pixels[symIdx] = color;
            }
        }
        frame.pixels[idx] = color;
        frame.isCacheDirty = true;
    }
}

export function drawBrush(cx, cy, color, isEraser) {
    const r = Math.floor(state.brushSize / 2);
    for (let y = cy - r; y <= cy + r; y++) {
        for (let x = cx - r; x <= cx + r; x++) {
            if (x >= 0 && x < state.GRID_WIDTH && y >= 0 && y < state.GRID_HEIGHT) {
                drawPixel(x, y, isEraser ? null : color);
            }
        }
    }
}

export function floodFill(startX, startY, targetColor, fillColor) {
    if (startX < 0 || startX >= state.GRID_WIDTH || startY < 0 || startY >= state.GRID_HEIGHT) return;
    if (targetColor === fillColor) return;

    const frame = state.frames[state.currentFrameIndex];
    const queue = [[startX, startY]];
    const seen = new Set();
    const w = state.GRID_WIDTH;
    const h = state.GRID_HEIGHT;

    while (queue.length > 0) {
        const [cx, cy] = queue.pop();
        const key = cx + ',' + cy;
        if (seen.has(key)) continue;
        seen.add(key);

        const idx = cy * w + cx;
        const currentColor = frame.pixels[idx];

        if (currentColor === targetColor) {
            frame.pixels[idx] = fillColor;
            frame.isCacheDirty = true;

            if (cx + 1 < w) queue.push([cx + 1, cy]);
            if (cx - 1 >= 0) queue.push([cx - 1, cy]);
            if (cy + 1 < h) queue.push([cx, cy + 1]);
            if (cy - 1 >= 0) queue.push([cx, cy - 1]);
        }
    }
}

function debugPaint(e) {
    const coords = getCoordsFromEvent(e, canvas);
    let { x, y } = coords;

    // Pixel Tools
    if (['pen', 'eraser', 'fill', 'eyedropper'].includes(state.currentTool)) {
        if (state.currentTool === 'eyedropper') {
            // Handled in mousedown usually
            return;
        }
        if (state.currentTool === 'fill') {
            // Handled in mousedown
            return;
        }

        const isEraser = state.currentTool === 'eraser';
        drawBrush(x, y, state.currentColor, isEraser);
        if (state.actions.renderEditor) state.actions.renderEditor();
    }
}

export function setupEventListeners() {
    if (!canvas) return;

    canvas.onmousedown = (e) => {
        state.isDrawing = true;
        const coords = getCoordsFromEvent(e, canvas);
        state.lastCoords = coords;

        if (state.currentTool === 'eyedropper') {
            const frame = state.frames[state.currentFrameIndex];
            const idx = getPixelIndex(coords.x, coords.y);
            const color = frame.pixels[idx];
            if (color) {
                state.currentColor = color;
                const picker = document.getElementById('color-picker');
                if (picker) picker.value = rgbToHex(color);
                state.currentTool = 'pen';
                updateToolButtons();
                showToast(`Picked color: ${color}`);
            }
            state.isDrawing = false;
            return;
        }

        if (state.currentTool === 'fill') {
            const frame = state.frames[state.currentFrameIndex];
            const idx = getPixelIndex(coords.x, coords.y);
            const targetColor = frame.pixels[idx];
            floodFill(coords.x, coords.y, targetColor, state.currentColor);
            if (state.actions.renderEditor) state.actions.renderEditor();
            state.isDrawing = false;
            return;
        }

        // Shape Tools Creation
        if (state.currentTool.endsWith('-obj')) {
            state.isDrawingShape = true;
            state.shapeStartCoords = coords;
            state.selectedShape = null; // Deselect to draw new
            if (state.actions.renderEditor) state.actions.renderEditor();
            return;
        }

        // Select Tool Logic (Selection, Moving, Resizing)
        if (state.currentTool === 'select') {
            const mx = coords.x;
            const my = coords.y;

            // Check Resize Handles first
            if (state.selectedShape) {
                const s = state.selectedShape;
                const b = s.getBounds(); // x,y,width,height
                const p = 4; // padding used in renderer
                const hs = 8; // handle size

                // Coordinates matching renderer.js
                const handles = {
                    tl: { x: b.x - p, y: b.y - p },
                    tr: { x: b.x + b.width + p - hs, y: b.y - p },
                    bl: { x: b.x - p, y: b.y + b.height + p - hs },
                    br: { x: b.x + b.width + p - hs, y: b.y + b.height + p - hs }
                };

                for (const [key, h] of Object.entries(handles)) {
                    if (mx >= h.x && mx <= h.x + hs && my >= h.y && my <= h.y + hs) {
                        state.draggingHandle = key;
                        state.isResizing = true;
                        state.resizeStartShape = s.clone(); // Snapshot for ref
                        state.resizeStartCoords = { x: mx, y: my };
                        return;
                    }
                }
            }

            // Check Shapes
            const frame = state.frames[state.currentFrameIndex];
            let clickedShape = null;
            if (frame.shapes) {
                for (let i = frame.shapes.length - 1; i >= 0; i--) {
                    if (frame.shapes[i].containsPoint(coords.x, coords.y)) {
                        clickedShape = frame.shapes[i];
                        break;
                    }
                }
            }
            state.selectedShape = clickedShape;
            state.isDraggingShape = !!clickedShape;
            state.mouseDownOffset = clickedShape ? { x: coords.x - clickedShape.x, y: coords.y - clickedShape.y } : { x: 0, y: 0 };
            updateInteractionEditor();
            if (state.actions.renderEditor) state.actions.renderEditor();
            return;
        }

        // Pen/Eraser
        debugPaint(e);
    };

    canvas.onmousemove = (e) => {
        const coords = getCoordsFromEvent(e, canvas);

        if (state.isResizing && state.selectedShape && state.resizeStartShape) {
            const dx = coords.x - state.resizeStartCoords.x;
            const dy = coords.y - state.resizeStartCoords.y;
            const s = state.selectedShape;
            const orig = state.resizeStartShape;

            // Handle Logic
            if (state.draggingHandle === 'br') {
                s.width = orig.width + dx;
                s.height = orig.height + dy;
            } else if (state.draggingHandle === 'bl') {
                s.x = orig.x + dx;
                s.width = orig.width - dx;
                s.height = orig.height + dy;
            } else if (state.draggingHandle === 'tr') {
                s.y = orig.y + dy;
                s.width = orig.width + dx;
                s.height = orig.height - dy;
            } else if (state.draggingHandle === 'tl') {
                s.x = orig.x + dx;
                s.y = orig.y + dy;
                s.width = orig.width - dx;
                s.height = orig.height - dy;
            }

            if (state.actions.renderEditor) state.actions.renderEditor();
            return;
        }

        if (state.isDrawing && ['pen', 'eraser'].includes(state.currentTool)) {
            if (state.lastCoords) debugPaint(e);
            state.lastCoords = coords;
        }

        if (state.isDraggingShape && state.selectedShape) {
            state.selectedShape.x = coords.x - state.mouseDownOffset.x;
            state.selectedShape.y = coords.y - state.mouseDownOffset.y;
            if (state.actions.renderEditor) state.actions.renderEditor();
        }
    };

    canvas.onmouseup = (e) => {
        state.isDrawing = false;
        state.isDraggingShape = false;
        state.isDrawingShape = false;
        state.isResizing = false;
        state.draggingHandle = null;
        state.resizeStartShape = null;
        state.lastCoords = null;

        // Finalize Shape Creation
        if (state.currentTool.endsWith('-obj') && state.shapeStartCoords) {
            const endCoords = getCoordsFromEvent(e, canvas);
            const w = endCoords.x - state.shapeStartCoords.x;
            const h = endCoords.y - state.shapeStartCoords.y;
            if (Math.abs(w) > 2 || Math.abs(h) > 2) {
                const type = state.currentTool.replace('-obj', '').replace('tool-', '');
                const newShape = new Shape(type, state.shapeStartCoords.x, state.shapeStartCoords.y, w, h, state.currentColor);
                // Standardize negative
                if (newShape.width < 0) { newShape.x += newShape.width; newShape.width *= -1; }
                if (newShape.height < 0) { newShape.y += newShape.height; newShape.height *= -1; }

                state.frames[state.currentFrameIndex].shapes.push(newShape);
                state.selectedShape = newShape;
                state.currentTool = 'select'; // Switch to select after drawing
                updateToolButtons();
                updateLayersPanel();
                if (state.actions.renderEditor) state.actions.renderEditor();
            }
        }
        renderTimeline();
    };

    // Key Events
    document.addEventListener('keydown', (e) => {
        if (e.ctrlKey && e.key === 'z') {
            e.preventDefault();
            undo();
        } else if (e.ctrlKey && e.key === 'y') {
            e.preventDefault();
            redo();
        } else if (e.key === 'Delete' || e.key === 'Backspace') {
            // Only delete if NOT in an input field
            if (document.activeElement.tagName !== 'INPUT' && document.activeElement.tagName !== 'TEXTAREA') {
                deleteShape();
            }
        }
    });
}

export function applyEyePreset(type) {
    const frame = state.frames[state.currentFrameIndex];
    if (!frame) return;

    frame.shapes = []; // Clear for clean preset

    const centerX = state.GRID_WIDTH / 2;
    const centerY = state.GRID_HEIGHT / 2;
    const padding = 75;
    const color = state.currentColor || '#00d2ff';

    // Standard Size
    const w = 120;
    const h = 100;

    // Normalized Paths (0-100 coordinate space) - High Fidelity EMO/EILIK Style
    const paths = {
        // === Original EMO Style ===
        // Angry: Sharp Crescent
        angry: "M0,60 Q50,15 100,60 Q50,95 0,60 Z",
        // Alert: Wide Egg
        alert: "M50,5 C80,5 100,35 100,65 C100,90 80,100 50,100 C20,100 0,90 0,65 C0,35 20,5 50,5 Z",
        // Sad/Neutral: Friendly Squircle
        sad: "M10,15 Q50,5 90,15 Q100,50 90,85 Q50,95 10,85 Q0,50 10,15 Z",
        // Evil: Thin Fox Eye
        evil: "M0,50 Q50,25 100,50 Q50,75 0,50 Z",
        // Bean: Perfect Half-Round (Sleepy)
        bean: "M2,50 L98,50 Q98,100 50,100 Q2,100 2,50 Z",
        // Slit: Clean Rounded Bar
        slit: "M5,42 L95,42 Q100,42 100,50 Q100,58 95,58 L5,58 Q0,58 0,50 Q0,42 5,42 Z",
        // Happy: Curved Arc (Smile Eyes)
        happy: "M5,70 Q50,20 95,70 Q50,85 5,70 Z",
        // Love: Heart Shape
        love: "M50,20 C20,-10 -10,30 50,80 C110,30 80,-10 50,20 Z",
        // Sleepy: Droopy Half-Closed
        sleepy: "M5,55 L95,45 Q95,80 50,85 Q5,80 5,55 Z",
        // Bored: Flat Line
        bored: "M5,45 L95,45 Q100,50 95,55 L5,55 Q0,50 5,45 Z",
        // Excited: Big Round
        excited: "M50,0 C85,0 100,25 100,50 C100,75 85,100 50,100 C15,100 0,75 0,50 C0,25 15,0 50,0 Z",
        // Cry: Sad with Tear Shape
        cry: "M10,10 Q50,0 90,10 Q100,45 90,80 Q50,95 10,80 Q0,45 10,10 Z",

        // === NEW EILIK Style Presets ===
        // Neutral: Rounded Rectangle (EILIK's default look)
        neutral: "M15,10 L85,10 Q100,10 100,25 L100,75 Q100,90 85,90 L15,90 Q0,90 0,75 L0,25 Q0,10 15,10 Z",
        // Wink: One eye closed (use for right eye)
        wink: "M10,48 L90,48 Q95,50 90,52 L10,52 Q5,50 10,48 Z",
        // Curious: Raised brow effect
        curious: "M20,5 Q60,0 95,15 Q100,50 95,85 Q60,95 20,90 Q0,55 20,5 Z",
        // Thinking: Looking up
        thinking: "M10,5 Q50,0 90,5 Q100,35 90,65 Q50,75 10,65 Q0,35 10,5 Z",
        // Skeptic: One raised eyebrow
        skeptic: "M5,30 Q50,15 95,35 Q100,55 95,75 Q50,85 5,75 Q0,55 5,30 Z",
        // Dead: X shape
        dead: "M20,20 L50,50 L80,20 M20,80 L50,50 L80,80",
        // Dizzy: Spiral-ish
        dizzy: "M50,10 Q80,10 90,30 Q95,60 70,80 Q40,90 20,70 Q10,45 30,25 Q45,15 60,25 Q70,40 55,55 Q40,65 30,50 Q25,35 40,30 Q50,28 55,40 Z",
        // Heart Eyes: Filled hearts
        heart: "M50,15 C30,-5 5,20 50,65 C95,20 70,-5 50,15 Z",
        // Star Eyes
        star: "M50,0 L60,35 L100,40 L70,60 L80,100 L50,75 L20,100 L30,60 L0,40 L40,35 Z",
        // Question: Confused
        question: "M30,10 Q70,5 80,30 Q85,50 55,60 L55,70 M55,80 L55,90",
        // Dot: Minimal
        dot: "M35,35 Q50,20 65,35 Q80,50 65,65 Q50,80 35,65 Q20,50 35,35 Z",
        // UwU: Cute squiggly
        uwu: "M5,60 Q25,40 50,60 Q75,40 95,60 Q75,75 50,60 Q25,75 5,60 Z",
        // OwO: Big round cute
        owo: "M50,5 C85,5 100,30 100,55 C100,80 85,100 50,100 C15,100 0,80 0,55 C0,30 15,5 50,5 Z"
    };

    const rotations = {
        angry: { left: 25, right: -25 },
        evil: { left: 15, right: -15 },
        sad: { left: -10, right: 10 },
        happy: { left: 0, right: 0 },
        love: { left: 0, right: 0 },
        sleepy: { left: 5, right: -5 },
        bored: { left: 0, right: 0 },
        excited: { left: -5, right: 5 },
        cry: { left: -15, right: 15 },
        alert: { left: 0, right: 0 },
        bean: { left: 0, right: 0 },
        slit: { left: 0, right: 0 },
        // EILIK-style rotations
        neutral: { left: 0, right: 0 },
        wink: { left: 0, right: 0 },
        curious: { left: -8, right: 8 },
        thinking: { left: 0, right: 0 },
        skeptic: { left: -12, right: 5 },
        dead: { left: 0, right: 0 },
        dizzy: { left: 15, right: -15 },
        heart: { left: 0, right: 0 },
        star: { left: 5, right: -5 },
        question: { left: 0, right: 0 },
        dot: { left: 0, right: 0 },
        uwu: { left: 0, right: 0 },
        owo: { left: 0, right: 0 }
    };

    const createEye = (isRight) => {
        const pathData = paths[type] || paths.sad;
        const x = isRight ? centerX + padding : centerX - padding - w;
        const y = centerY - (h / 2);

        const s = new Shape('path', x, y, w, h, color);
        s.pathData = pathData;

        // Dynamic Rotation
        const rot = rotations[type] || { left: 0, right: 0 };
        s.rotation = isRight ? rot.right : rot.left;

        if (isRight) {
            s.isMirrored = true;
        }
        return s;
    };

    frame.shapes.push(createEye(false));
    frame.shapes.push(createEye(true));

    state.currentTool = 'select';
    updateToolButtons();
    if (state.actions.renderEditor) state.actions.renderEditor();
    if (state.actions.renderPreview) state.actions.renderPreview();
    updateLayersPanel();
    renderTimeline();
    showToast(`Preset: ${type.toUpperCase()} Applied`);
}

export function applyAnimationPreset(type) {
    try {
        if (typeof Frame === 'undefined') {
            showToast("Error: Frame class not loaded");
            return;
        }

        saveUndo();
        state.frames.length = 0; // Clear current sequence in-place

        const centerX = state.GRID_WIDTH / 2;
        const centerY = state.GRID_HEIGHT / 2;
        const padding = 75;
        const color = state.currentColor || '#00d2ff';
        const w = 120, h = 100;

        // EMO Style Eye Paths
        const paths = {
            normal: "M10,15 Q50,5 90,15 Q100,50 90,85 Q50,95 10,85 Q0,50 10,15 Z",
            closed: "M5,48 L95,48 Q95,55 50,55 Q5,55 5,48 Z",
            wide: "M50,0 C85,0 100,25 100,50 C100,75 85,100 50,100 C15,100 0,75 0,50 C0,25 15,0 50,0 Z",
            squint: "M0,60 Q50,15 100,60 Q50,95 0,60 Z",
            happy: "M5,70 Q50,20 95,70 Q50,85 5,70 Z",
            sad: "M10,20 Q50,40 90,20 Q100,55 90,80 Q50,90 10,80 Q0,55 10,20 Z",
            love: "M50,15 C25,-10 -5,25 50,75 C105,25 75,-10 50,15 Z",
            angry: "M0,55 Q50,20 100,55 Q50,85 0,55 Z",
            sleepy: "M5,50 L95,45 Q95,65 50,68 Q5,65 5,50 Z",
            confused: "M15,25 Q50,10 85,35 Q95,55 85,75 Q50,90 15,75 Q5,55 15,25 Z",
            excited: "M50,5 C80,5 100,30 100,55 C100,80 80,100 50,100 C20,100 0,80 0,55 C0,30 20,5 50,5 Z",
            tiny: "M30,35 Q50,30 70,35 Q75,50 70,65 Q50,70 30,65 Q25,50 30,35 Z"
        };

        // Consistent IDs for interpolation
        const LEFT_EYE_ID = 'anim_left_eye';
        const RIGHT_EYE_ID = 'anim_right_eye';

        const createFrame = (leftPath, rightPath, duration, easing = 'ease-in-out', leftRot = 0, rightRot = 0, offsetY = 0) => {
            const frame = new Frame();
            frame.duration = duration;
            frame.easing = easing;

            const createEye = (isRight, pathKey, rot) => {
                const x = isRight ? centerX + padding : centerX - padding - w;
                const y = centerY - (h / 2) + offsetY;
                const s = new Shape('path', x, y, w, h, color);
                s.pathData = paths[pathKey] || paths.normal;
                s.rotation = rot;
                s.id = isRight ? RIGHT_EYE_ID : LEFT_EYE_ID; // Consistent ID for interpolation
                if (isRight) s.isMirrored = true;
                return s;
            };

            frame.shapes.push(createEye(false, leftPath, leftRot));
            frame.shapes.push(createEye(true, rightPath, rightRot));
            return frame;
        };

        // Animation Definitions - EMO Style with Life!
        if (type === 'wink') {
            // Playful wink with subtle bounce
            state.frames.push(createFrame('normal', 'normal', 300));
            state.frames.push(createFrame('normal', 'normal', 80, 'ease-out', 0, 3)); // prepare
            state.frames.push(createFrame('normal', 'closed', 60, 'ease-out'));
            state.frames.push(createFrame('normal', 'closed', 80, 'linear'));
            state.frames.push(createFrame('normal', 'normal', 120, 'overshoot', 0, -5)); // bounce back
            state.frames.push(createFrame('normal', 'normal', 150, 'ease-out'));
        }
        else if (type === 'blink') {
            // Natural blink with micro-movement
            state.frames.push(createFrame('normal', 'normal', 400));
            state.frames.push(createFrame('normal', 'normal', 50, 'ease-out', 0, 0, -2)); // squish
            state.frames.push(createFrame('closed', 'closed', 40, 'ease-out', 0, 0, 0));
            state.frames.push(createFrame('closed', 'closed', 30, 'linear'));
            state.frames.push(createFrame('normal', 'normal', 100, 'overshoot', 0, 0, 3)); // bounce
            state.frames.push(createFrame('normal', 'normal', 120, 'ease-out'));
        }
        else if (type === 'surprise') {
            // Dramatic surprise with shake
            state.frames.push(createFrame('normal', 'normal', 80));
            state.frames.push(createFrame('wide', 'wide', 80, 'overshoot', -3, 3, -8)); // jump up
            state.frames.push(createFrame('wide', 'wide', 60, 'linear', 3, -3, -5));
            state.frames.push(createFrame('wide', 'wide', 60, 'linear', -2, 2, -6));
            state.frames.push(createFrame('wide', 'wide', 600, 'linear', 0, 0, -4)); // hold
            state.frames.push(createFrame('normal', 'normal', 250, 'ease-in-out'));
        }
        else if (type === 'skeptic') {
            // Suspicious look with tilt
            state.frames.push(createFrame('normal', 'normal', 200));
            state.frames.push(createFrame('normal', 'squint', 180, 'ease-out', 5, -20));
            state.frames.push(createFrame('normal', 'squint', 300, 'linear', 8, -22)); // tilt more
            state.frames.push(createFrame('normal', 'squint', 800, 'linear', 8, -22));
            state.frames.push(createFrame('normal', 'squint', 200, 'ease-in', 3, -18));
            state.frames.push(createFrame('normal', 'normal', 300, 'ease-in-out'));
        }
        else if (type === 'happy') {
            // Joyful bounce with squish
            state.frames.push(createFrame('normal', 'normal', 150));
            state.frames.push(createFrame('normal', 'normal', 80, 'ease-out', 0, 0, 5)); // prepare
            state.frames.push(createFrame('happy', 'happy', 100, 'overshoot', -3, 3, -10)); // jump!
            state.frames.push(createFrame('happy', 'happy', 80, 'linear', 3, -3, -5));
            state.frames.push(createFrame('happy', 'happy', 80, 'linear', -2, 2, -7));
            state.frames.push(createFrame('happy', 'happy', 600, 'linear', 0, 0, -3));
            state.frames.push(createFrame('normal', 'normal', 300, 'ease-in-out'));
        }
        else if (type === 'sad') {
            // Droopy sad with trembling
            state.frames.push(createFrame('normal', 'normal', 250));
            state.frames.push(createFrame('sad', 'sad', 400, 'ease-out', -10, 10, 8)); // droop
            state.frames.push(createFrame('sad', 'sad', 200, 'linear', -12, 12, 10)); // tremble
            state.frames.push(createFrame('sad', 'sad', 200, 'linear', -10, 10, 9));
            state.frames.push(createFrame('sad', 'sad', 800, 'linear', -12, 12, 10));
            state.frames.push(createFrame('normal', 'normal', 400, 'ease-in-out'));
        }
        else if (type === 'love') {
            // Heart eyes with pulse
            state.frames.push(createFrame('normal', 'normal', 150));
            state.frames.push(createFrame('love', 'love', 150, 'overshoot', 0, 0, -5)); // pop!
            state.frames.push(createFrame('love', 'love', 120, 'ease-out', 0, 0, -2)); // pulse
            state.frames.push(createFrame('love', 'love', 120, 'ease-in', 0, 0, -6)); // pulse
            state.frames.push(createFrame('love', 'love', 120, 'ease-out', 0, 0, -3));
            state.frames.push(createFrame('love', 'love', 120, 'ease-in', 0, 0, -5));
            state.frames.push(createFrame('love', 'love', 400, 'linear', 0, 0, -4));
            state.frames.push(createFrame('normal', 'normal', 300, 'ease-in-out'));
        }
        else if (type === 'angry') {
            // Fierce shake with intensity
            state.frames.push(createFrame('normal', 'normal', 100));
            state.frames.push(createFrame('angry', 'angry', 80, 'ease-out', 25, -25, -3));
            state.frames.push(createFrame('angry', 'angry', 60, 'linear', 22, -22, -5)); // shake
            state.frames.push(createFrame('angry', 'angry', 60, 'linear', 28, -28, -3));
            state.frames.push(createFrame('angry', 'angry', 60, 'linear', 22, -22, -4));
            state.frames.push(createFrame('angry', 'angry', 600, 'linear', 25, -25, -3));
            state.frames.push(createFrame('normal', 'normal', 300, 'ease-in-out'));
        }
        else if (type === 'sleepy') {
            // Slow droopy blinks
            state.frames.push(createFrame('normal', 'normal', 400));
            state.frames.push(createFrame('sleepy', 'sleepy', 500, 'ease-out', 5, -5, 5));
            state.frames.push(createFrame('sleepy', 'sleepy', 300, 'ease-out', 3, -3, 6));
            state.frames.push(createFrame('closed', 'closed', 400, 'ease-out', 0, 0, 8));
            state.frames.push(createFrame('sleepy', 'sleepy', 600, 'ease-in', 4, -4, 6)); // struggle
            state.frames.push(createFrame('closed', 'closed', 800, 'ease-out', 0, 0, 8)); // give up
        }
        else if (type === 'confused') {
            // Tilting back and forth
            state.frames.push(createFrame('normal', 'normal', 150));
            state.frames.push(createFrame('confused', 'normal', 180, 'ease-out', -12, 0, 0));
            state.frames.push(createFrame('normal', 'confused', 180, 'ease-out', 0, 12, 0));
            state.frames.push(createFrame('confused', 'confused', 150, 'linear', -8, 8, -3));
            state.frames.push(createFrame('confused', 'confused', 150, 'linear', -10, 10, 0));
            state.frames.push(createFrame('confused', 'confused', 400, 'linear', -8, 8, -2));
            state.frames.push(createFrame('normal', 'normal', 300, 'ease-in-out'));
        }
        else if (type === 'excited') {
            // Bouncy excitement with rapid changes
            state.frames.push(createFrame('normal', 'normal', 80));
            state.frames.push(createFrame('excited', 'excited', 60, 'overshoot', -8, 8, -12));
            state.frames.push(createFrame('wide', 'wide', 50, 'linear', 8, -8, -8));
            state.frames.push(createFrame('excited', 'excited', 50, 'linear', -6, 6, -14));
            state.frames.push(createFrame('wide', 'wide', 50, 'linear', 6, -6, -10));
            state.frames.push(createFrame('excited', 'excited', 50, 'linear', -8, 8, -12));
            state.frames.push(createFrame('wide', 'wide', 50, 'linear', 5, -5, -9));
            state.frames.push(createFrame('excited', 'excited', 400, 'ease-out', -3, 3, -8));
            state.frames.push(createFrame('normal', 'normal', 250, 'ease-in-out'));
        }
        else if (type === 'nod') {
            // Natural nodding with follow-through
            state.frames.push(createFrame('normal', 'normal', 150, 'ease-in-out', 0, 0, 0));
            state.frames.push(createFrame('normal', 'normal', 100, 'ease-out', 0, 0, 18));
            state.frames.push(createFrame('normal', 'normal', 80, 'ease-in', 0, 0, 5));
            state.frames.push(createFrame('normal', 'normal', 100, 'ease-out', 0, 0, 15));
            state.frames.push(createFrame('normal', 'normal', 80, 'ease-in', 0, 0, 3));
            state.frames.push(createFrame('normal', 'normal', 150, 'ease-in-out', 0, 0, 0));
        }
        else if (type === 'shake') {
            // Vigorous head shake
            state.frames.push(createFrame('normal', 'normal', 80, 'linear', 0, 0, 0));
            state.frames.push(createFrame('normal', 'normal', 50, 'ease-out', -12, -12, 0));
            state.frames.push(createFrame('normal', 'normal', 50, 'ease-out', 12, 12, 0));
            state.frames.push(createFrame('normal', 'normal', 50, 'ease-out', -10, -10, 0));
            state.frames.push(createFrame('normal', 'normal', 50, 'ease-out', 10, 10, 0));
            state.frames.push(createFrame('normal', 'normal', 50, 'ease-out', -6, -6, 0));
            state.frames.push(createFrame('normal', 'normal', 50, 'ease-out', 6, 6, 0));
            state.frames.push(createFrame('normal', 'normal', 120, 'ease-in-out', 0, 0, 0));
        }
        // NEW: Idle breathing animation
        else if (type === 'idle') {
            state.frames.push(createFrame('normal', 'normal', 800, 'ease-in-out', 0, 0, 0));
            state.frames.push(createFrame('normal', 'normal', 600, 'ease-in-out', 0, 0, 3)); // breathe in
            state.frames.push(createFrame('normal', 'normal', 800, 'ease-in-out', 0, 0, 0));
            state.frames.push(createFrame('normal', 'normal', 600, 'ease-in-out', 0, 0, 3));
            state.frames.push(createFrame('closed', 'closed', 60, 'ease-out')); // blink
            state.frames.push(createFrame('closed', 'closed', 40, 'linear'));
            state.frames.push(createFrame('normal', 'normal', 100, 'overshoot'));
            state.frames.push(createFrame('normal', 'normal', 600, 'ease-in-out', 0, 0, 2));
        }
        // NEW: Curious look around
        else if (type === 'curious') {
            state.frames.push(createFrame('normal', 'normal', 200));
            state.frames.push(createFrame('wide', 'wide', 150, 'ease-out', 0, 0, 0));
            state.frames.push(createFrame('wide', 'wide', 300, 'ease-in-out', -15, -15, 0)); // look left
            state.frames.push(createFrame('wide', 'wide', 200, 'ease-in-out', 0, 0, 0));
            state.frames.push(createFrame('wide', 'wide', 300, 'ease-in-out', 15, 15, 0)); // look right
            state.frames.push(createFrame('wide', 'wide', 200, 'ease-in-out', 0, 0, 0));
            state.frames.push(createFrame('normal', 'normal', 250, 'ease-in-out'));
        }
        // NEW: Hearty laugh
        else if (type === 'laugh') {
            state.frames.push(createFrame('normal', 'normal', 100));
            state.frames.push(createFrame('happy', 'happy', 80, 'ease-out', 0, 0, -8));
            state.frames.push(createFrame('happy', 'happy', 60, 'linear', 0, 0, -3));
            state.frames.push(createFrame('happy', 'happy', 60, 'linear', 0, 0, -10));
            state.frames.push(createFrame('happy', 'happy', 60, 'linear', 0, 0, -4));
            state.frames.push(createFrame('happy', 'happy', 60, 'linear', 0, 0, -9));
            state.frames.push(createFrame('happy', 'happy', 60, 'linear', 0, 0, -5));
            state.frames.push(createFrame('happy', 'happy', 300, 'ease-out', 0, 0, -6));
            state.frames.push(createFrame('normal', 'normal', 250, 'ease-in-out'));
        }
        // NEW: Bouncy attention
        else if (type === 'bounce') {
            state.frames.push(createFrame('normal', 'normal', 100, 'ease-in', 0, 0, 5)); // squat
            state.frames.push(createFrame('wide', 'wide', 80, 'overshoot', 0, 0, -20)); // jump!
            state.frames.push(createFrame('wide', 'wide', 100, 'ease-in', 0, 0, 5)); // land
            state.frames.push(createFrame('normal', 'normal', 80, 'overshoot', 0, 0, -12)); // bounce
            state.frames.push(createFrame('normal', 'normal', 80, 'ease-in', 0, 0, 3)); // settle
            state.frames.push(createFrame('normal', 'normal', 60, 'ease-out', 0, 0, -5)); // tiny bounce
            state.frames.push(createFrame('normal', 'normal', 150, 'ease-in-out', 0, 0, 0));
        }
        // NEW: Dizzy spinning
        else if (type === 'dizzy') {
            state.frames.push(createFrame('normal', 'normal', 100));
            state.frames.push(createFrame('confused', 'confused', 100, 'linear', 15, 15, 0));
            state.frames.push(createFrame('confused', 'confused', 100, 'linear', -20, -20, 0));
            state.frames.push(createFrame('confused', 'confused', 100, 'linear', 25, 25, 3));
            state.frames.push(createFrame('confused', 'confused', 100, 'linear', -18, -18, -2));
            state.frames.push(createFrame('confused', 'confused', 100, 'linear', 20, 20, 4));
            state.frames.push(createFrame('confused', 'confused', 150, 'ease-out', -10, -10, 0));
            state.frames.push(createFrame('sleepy', 'sleepy', 300, 'ease-out', 5, -5, 5));
            state.frames.push(createFrame('normal', 'normal', 300, 'ease-in-out'));
        }

        state.currentFrameIndex = 0;

        // Refresh Everything
        if (typeof renderTimeline === 'function') renderTimeline();
        if (state.actions.renderEditor) state.actions.renderEditor();
        if (state.actions.renderPreview) state.actions.renderPreview();
        updateLayersPanel();
        updateFrameInfo();
        showToast(`Anim: ${type.toUpperCase()} Loaded ✨`);

    } catch (err) {
        console.error("Preset Error:", err);
        showToast("Error loading preset: " + err.message);
    }
}

export function duplicateShape() {
    if (!state.selectedShape) {
        showToast('Select a shape first!');
        return;
    }
    const newShape = state.selectedShape.clone();
    newShape.x += 20;
    newShape.y += 20;
    // Ensure unique ID
    newShape.id = Date.now() + Math.random();

    state.frames[state.currentFrameIndex].shapes.push(newShape);
    state.selectedShape = newShape;
    if (state.actions.renderEditor) state.actions.renderEditor();
    updateLayersPanel();
    renderTimeline();
}

export function deleteShape() {
    if (!state.selectedShape) {
        // alert('Select a shape first!');
        return;
    }
    const shapes = state.frames[state.currentFrameIndex].shapes;
    const idx = shapes.findIndex(s => s.id === state.selectedShape.id);
    if (idx !== -1) {
        shapes.splice(idx, 1);
        state.selectedShape = null;
        if (state.actions.renderEditor) state.actions.renderEditor();
        updateLayersPanel();
    }
}

export function scaleShape(factor) {
    if (!state.selectedShape) {
        return;
    }
    state.selectedShape.width *= factor;
    state.selectedShape.height *= factor;
    if (state.actions.renderEditor) state.actions.renderEditor();
    renderTimeline();
}

export function alignShapes(direction) {
    if (!state.selectedShape) return;
    const bounds = state.selectedShape.getBounds();

    switch (direction) {
        case 'left': state.selectedShape.x = 0; break;
        case 'center-h': state.selectedShape.x = (state.GRID_WIDTH - bounds.width) / 2; break;
        case 'right': state.selectedShape.x = state.GRID_WIDTH - bounds.width; break;
        case 'top': state.selectedShape.y = 0; break;
        case 'center-v': state.selectedShape.y = (state.GRID_HEIGHT - bounds.height) / 2; break;
        case 'bottom': state.selectedShape.y = state.GRID_HEIGHT - bounds.height; break;
    }
    if (state.actions.renderEditor) state.actions.renderEditor();
}

export function moveLayerUp() {
    if (!state.selectedShape) return;
    const shapes = state.frames[state.currentFrameIndex].shapes;
    const idx = shapes.findIndex(s => s.id === state.selectedShape.id);
    if (idx < shapes.length - 1) {
        [shapes[idx], shapes[idx + 1]] = [shapes[idx + 1], shapes[idx]];
        if (state.actions.renderEditor) state.actions.renderEditor();
        updateLayersPanel();
    }
}

export function moveLayerDown() {
    if (!state.selectedShape) return;
    const shapes = state.frames[state.currentFrameIndex].shapes;
    const idx = shapes.findIndex(s => s.id === state.selectedShape.id);
    if (idx > 0) {
        [shapes[idx], shapes[idx - 1]] = [shapes[idx - 1], shapes[idx]];
        if (state.actions.renderEditor) state.actions.renderEditor();
        updateLayersPanel();
    }
}
