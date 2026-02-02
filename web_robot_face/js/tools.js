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
    const padding = 40;
    const color = state.currentColor || '#00d2ff';

    // Standard Size
    const w = 160;
    const h = 130;

    // Normalized Paths (0-100 coordinate space) - High Fidelity EMO/EILIK Style
    const paths = {
        // === Original EMO Style ===
        // Angry: Sharp Crescent
        angry: "M0,30 L100,30 Q100,100 50,100 Q0,100 0,30 Z",
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
        happy: "M5,70 Q50,20 95,70 Q50,50 5,70 Z",
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
        owo: "M50,5 C85,5 100,30 100,55 C100,80 85,100 50,100 C15,100 0,80 0,55 C0,30 15,5 50,5 Z",

        // === NEW Essential Presets ===
        // Shock: Very wide open
        shock: "M50,0 C90,0 100,20 100,50 C100,80 90,100 50,100 C10,100 0,80 0,50 C0,20 10,0 50,0 Z",
        // Focused: Slightly narrowed, determined
        focused: "M5,30 L95,30 Q100,50 95,70 L5,70 Q0,50 5,30 Z",
        // Tired: Heavy droopy
        tired: "M0,60 L100,50 Q100,85 50,90 Q0,85 0,60 Z",
        // Scared: Wide but trembling shape
        scared: "M50,5 Q85,5 95,35 Q100,60 90,85 Q50,95 10,85 Q0,60 5,35 Q15,5 50,5 Z",
        // Annoyed: Half-lidded, unimpressed
        annoyed: "M5,40 L95,35 Q100,50 95,65 L5,60 Q0,50 5,40 Z",
        // Shy: Small and looking away
        shy: "M30,30 Q50,25 70,35 Q80,55 70,75 Q50,80 30,70 Q20,50 30,30 Z",
        // Loading: Circular progress style
        loading: "M50,10 A40,40 0 1,1 50,90 A40,40 0 1,1 50,10 Z",
        // Closed: Completely shut
        closed: "M5,48 L95,48 Q100,50 95,52 L5,52 Q0,50 5,48 Z",
        // Squint: Almost closed, suspicious
        squint: "M5,45 L95,42 Q100,50 95,58 L5,55 Q0,50 5,45 Z",
        // Glare: Sharp angry stare
        glare: "M0,35 L100,35 Q100,75 50,80 Q0,75 0,35 Z",
        // Pleading: Puppy eyes
        pleading: "M50,0 C90,10 100,40 95,70 Q50,100 5,70 C0,40 10,10 50,0 Z",
        // Sparkle: Excited with highlight
        sparkle: "M50,5 C80,5 100,30 100,55 C100,80 80,100 50,100 C20,100 0,80 0,55 C0,30 20,5 50,5 Z"
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
        owo: { left: 0, right: 0 },
        // New presets rotations
        shock: { left: 0, right: 0 },
        focused: { left: 0, right: 0 },
        tired: { left: 8, right: -8 },
        scared: { left: -5, right: 5 },
        annoyed: { left: 5, right: -5 },
        shy: { left: 15, right: -15 },
        loading: { left: 0, right: 0 },
        closed: { left: 0, right: 0 },
        squint: { left: 3, right: -3 },
        glare: { left: 20, right: -20 },
        pleading: { left: -8, right: 8 },
        sparkle: { left: -3, right: 3 }
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

export function applyAnimationPreset(type, options = {}) {
    try {
        if (typeof Frame === 'undefined') {
            showToast("Error: Frame class not loaded");
            return;
        }

        const lastFrameIndex = state.frames.length > 0 ? state.frames.length - 1 : 0;
        const currentFrame = state.frames[lastFrameIndex];

        if (!currentFrame || !currentFrame.shapes || currentFrame.shapes.length === 0) {
            showToast("No eyes found to animate! Draw or select a preset first.");
            return;
        }

        // Configuration with default values
        const config = {
            speed: options.speed || 1.0,      // Speed multiplier (0.5 = slower, 2.0 = faster)
            intensity: options.intensity || 1.0  // Intensity multiplier (0.5 = subtle, 2.0 = exaggerated)
        };

        // Animation Templates - Define timing and transformations as parameters
        const animationTemplates = {
            wink: {
                frames: [
                    { duration: 300, easing: 'linear', transform: 'reset' },
                    { duration: 80, easing: 'ease-out', transform: { type: 'squash', target: 'right', factor: 0.8 } },
                    { duration: 60, easing: 'ease-out', transform: { type: 'squash', target: 'right', factor: 0.1 } },
                    { duration: 80, easing: 'linear', transform: { type: 'squash', target: 'right', factor: 0.1 } },
                    { duration: 120, easing: 'overshoot', transform: { type: 'squash', target: 'right', factor: 1.1 } },
                    { duration: 150, easing: 'ease-out', transform: 'reset' }
                ]
            },
            blink: {
                frames: [
                    { duration: 400, easing: 'linear', transform: 'reset' },
                    { duration: 50, easing: 'ease-out', transform: { type: 'squash', target: 'both', factor: 0.8 } },
                    { duration: 40, easing: 'ease-out', transform: { type: 'squash', target: 'both', factor: 0.1 } },
                    { duration: 30, easing: 'linear', transform: { type: 'squash', target: 'both', factor: 0.1 } },
                    { duration: 100, easing: 'overshoot', transform: { type: 'squash', target: 'both', factor: 1.1 } },
                    { duration: 120, easing: 'ease-out', transform: 'reset' }
                ]
            },
            surprise: {
                frames: [
                    { duration: 80, easing: 'linear', transform: 'reset' },
                    { duration: 80, easing: 'overshoot', transform: { type: 'translate', y: -8, scale: { w: 1.1, h: 1.2 } } },
                    { duration: 60, easing: 'linear', transform: { type: 'translate', y: -5, scale: { w: 1.05, h: 1.1 } } },
                    { duration: 600, easing: 'linear', transform: { type: 'translate', y: -4 } },
                    { duration: 250, easing: 'ease-in-out', transform: 'reset' }
                ]
            },
            happy: {
                frames: [
                    { duration: 150, easing: 'linear', transform: 'reset' },
                    { duration: 100, easing: 'overshoot', transform: { type: 'translate', y: -10 } },
                    { duration: 80, easing: 'linear', transform: { type: 'translate', y: -5 } },
                    { duration: 80, easing: 'linear', transform: { type: 'translate', y: -7 } },
                    { duration: 600, easing: 'linear', transform: { type: 'translate', y: -3 } },
                    { duration: 300, easing: 'ease-in-out', transform: 'reset' }
                ]
            },
            sad: {
                frames: [
                    { duration: 250, easing: 'linear', transform: 'reset' },
                    { duration: 400, easing: 'ease-out', transform: { type: 'translate', y: 8, rotate: { left: -10, right: 10 } } },
                    { duration: 200, easing: 'linear', transform: { type: 'translate', y: 10, x: 1, rotate: { left: -12, right: 12 } } },
                    { duration: 200, easing: 'linear', transform: { type: 'translate', y: 9, x: -1, rotate: { left: -10, right: 10 } } },
                    { duration: 400, easing: 'ease-in-out', transform: 'reset' }
                ]
            },
            nod: {
                frames: [
                    { duration: 150, easing: 'ease-in-out', transform: 'reset' },
                    { duration: 100, easing: 'ease-out', transform: { type: 'translate', y: 15 } },
                    { duration: 80, easing: 'ease-in', transform: { type: 'translate', y: 5 } },
                    { duration: 100, easing: 'ease-out', transform: { type: 'translate', y: 12 } },
                    { duration: 150, easing: 'ease-in-out', transform: 'reset' }
                ]
            },
            shake: {
                frames: [
                    { duration: 80, easing: 'linear', transform: 'reset' },
                    { duration: 50, easing: 'ease-out', transform: { type: 'translate', x: -10 } },
                    { duration: 50, easing: 'ease-out', transform: { type: 'translate', x: 10 } },
                    { duration: 50, easing: 'ease-out', transform: { type: 'translate', x: -8 } },
                    { duration: 50, easing: 'ease-out', transform: { type: 'translate', x: 8 } },
                    { duration: 120, easing: 'ease-in-out', transform: 'reset' }
                ]
            },
            skeptic: {
                frames: [
                    { duration: 200, easing: 'linear', transform: 'reset' },
                    { duration: 180, easing: 'ease-out', transform: { type: 'squash-asymmetric', left: { squash: 1.0 }, right: { squash: 0.7, rotate: -10 } } },
                    { duration: 800, easing: 'linear', transform: { type: 'squash-asymmetric', left: { rotate: 5 }, right: { squash: 0.6, rotate: -12 } } },
                    { duration: 300, easing: 'ease-in-out', transform: 'reset' }
                ]
            },
            love: {
                frames: [
                    { duration: 150, easing: 'linear', transform: 'reset' },
                    { duration: 150, easing: 'overshoot', transform: { type: 'translate', scale: { w: 1.2, h: 1.2 } } },
                    { duration: 120, easing: 'ease-out', transform: { type: 'translate', scale: { w: 1.05, h: 1.05 } } },
                    { duration: 120, easing: 'ease-in', transform: { type: 'translate', scale: { w: 1.15, h: 1.15 } } },
                    { duration: 120, easing: 'ease-out', transform: { type: 'translate', scale: { w: 1.05, h: 1.05 } } },
                    { duration: 400, easing: 'linear', transform: 'reset' }
                ]
            },
            angry: {
                frames: [
                    { duration: 100, easing: 'linear', transform: 'reset' },
                    { duration: 80, easing: 'ease-out', transform: { type: 'translate', x: 3 } },
                    { duration: 60, easing: 'linear', transform: { type: 'translate', x: -3 } },
                    { duration: 60, easing: 'linear', transform: { type: 'translate', x: 3 } },
                    { duration: 60, easing: 'linear', transform: { type: 'translate', x: -3 } },
                    { duration: 600, easing: 'linear', transform: { type: 'translate', scale: { w: 1.05, h: 1.0 } } },
                    { duration: 300, easing: 'ease-in-out', transform: 'reset' }
                ]
            },
            sleepy: {
                frames: [
                    { duration: 400, easing: 'linear', transform: 'reset' },
                    { duration: 500, easing: 'ease-out', transform: { type: 'squash', target: 'both', factor: 0.6 } },
                    { duration: 300, easing: 'ease-out', transform: { type: 'squash', target: 'both', factor: 0.4 } },
                    { duration: 400, easing: 'ease-out', transform: { type: 'squash', target: 'both', factor: 0.1 } },
                    { duration: 600, easing: 'ease-in', transform: { type: 'squash', target: 'both', factor: 0.3 } },
                    { duration: 800, easing: 'ease-out', transform: { type: 'squash', target: 'both', factor: 0.1 } }
                ]
            },
            confused: {
                frames: [
                    { duration: 150, easing: 'linear', transform: 'reset' },
                    { duration: 180, easing: 'ease-out', transform: { type: 'tilt', rotate: 10 } },
                    { duration: 180, easing: 'ease-out', transform: { type: 'tilt', rotate: -10 } },
                    { duration: 400, easing: 'linear', transform: 'reset' }
                ]
            },
            excited: {
                frames: [
                    { duration: 80, easing: 'linear', transform: 'reset' },
                    { duration: 60, easing: 'overshoot', transform: { type: 'translate', y: -12 } },
                    { duration: 50, easing: 'linear', transform: { type: 'translate', y: 8 } },
                    { duration: 50, easing: 'linear', transform: { type: 'translate', y: -10 } },
                    { duration: 50, easing: 'linear', transform: { type: 'translate', y: 6 } },
                    { duration: 400, easing: 'ease-out', transform: 'reset' }
                ]
            },
            idle: {
                frames: [
                    { duration: 800, easing: 'ease-in-out', transform: 'reset' },
                    { duration: 600, easing: 'ease-in-out', transform: { type: 'translate', scale: { w: 1.02, h: 1.02 } } },
                    { duration: 800, easing: 'ease-in-out', transform: 'reset' },
                    { duration: 600, easing: 'ease-in-out', transform: { type: 'translate', scale: { w: 1.02, h: 1.02 } } },
                    { duration: 60, easing: 'ease-out', transform: { type: 'squash', target: 'both', factor: 0.1 } },
                    { duration: 40, easing: 'linear', transform: { type: 'squash', target: 'both', factor: 0.1 } },
                    { duration: 100, easing: 'overshoot', transform: 'reset' },
                    { duration: 600, easing: 'ease-in-out', transform: 'reset' }
                ]
            },
            curious: {
                frames: [
                    { duration: 200, easing: 'linear', transform: 'reset' },
                    { duration: 300, easing: 'ease-in-out', transform: { type: 'translate', x: -15, rotate: { left: -5, right: -5 } } },
                    { duration: 200, easing: 'ease-in-out', transform: 'reset' },
                    { duration: 300, easing: 'ease-in-out', transform: { type: 'translate', x: 15, rotate: { left: 5, right: 5 } } },
                    { duration: 200, easing: 'ease-in-out', transform: 'reset' }
                ]
            },
            laugh: {
                frames: [
                    { duration: 100, easing: 'linear', transform: 'reset' },
                    { duration: 80, easing: 'ease-out', transform: { type: 'translate', y: -8 } },
                    { duration: 60, easing: 'linear', transform: { type: 'translate', y: -3 } },
                    { duration: 60, easing: 'linear', transform: { type: 'translate', y: -10 } },
                    { duration: 60, easing: 'linear', transform: { type: 'translate', y: -4 } },
                    { duration: 300, easing: 'ease-out', transform: 'reset' }
                ]
            },
            bounce: {
                frames: [
                    { duration: 100, easing: 'ease-in', transform: { type: 'translate', y: 0, scale: { w: 1.1, h: 0.8 } } },
                    { duration: 80, easing: 'overshoot', transform: { type: 'translate', y: -20, scale: { w: 0.9, h: 1.1 } } },
                    { duration: 100, easing: 'ease-in', transform: 'reset' },
                    { duration: 80, easing: 'overshoot', transform: { type: 'translate', y: -5 } },
                    { duration: 150, easing: 'ease-in-out', transform: 'reset' }
                ]
            },
            dizzy: {
                frames: [
                    { duration: 100, easing: 'linear', transform: 'reset' },
                    { duration: 150, easing: 'linear', transform: { type: 'rotate-absolute', angle: 45 } },
                    { duration: 150, easing: 'linear', transform: { type: 'rotate-absolute', angle: 90 } },
                    { duration: 150, easing: 'linear', transform: { type: 'rotate-absolute', angle: 135 } },
                    { duration: 150, easing: 'linear', transform: { type: 'rotate-absolute', angle: 180 } },
                    { duration: 300, easing: 'ease-out', transform: 'reset' }
                ]
            }
        };

        // Get template or create a simple default
        const template = animationTemplates[type] || {
            frames: [
                { duration: 500, easing: 'ease-in-out', transform: 'reset' }
            ]
        };

        // Capture base shapes
        const baseShapes = currentFrame.shapes.map(s => s.clone(true));
        const centerX = state.GRID_WIDTH / 2;

        saveUndo();
        const startIndex = state.frames.length;

        // Helper to apply transformations based on template
        const applyTransform = (shape, base, transform, isRight, idx) => {
            if (transform === 'reset') return; // No changes

            if (transform.type === 'squash') {
                const factor = Math.pow(transform.factor, config.intensity);
                const shouldApply = transform.target === 'both' ||
                    (transform.target === 'right' && isRight) ||
                    (transform.target === 'left' && !isRight);

                if (shouldApply) {
                    const newHeight = base.height * factor;
                    shape.height = newHeight;
                    shape.y = base.y + (base.height - newHeight) / 2;
                }
            }
            else if (transform.type === 'squash-asymmetric') {
                // Different transformations for left and right eyes
                const eyeTransform = isRight ? transform.right : transform.left;
                if (eyeTransform) {
                    if (eyeTransform.squash !== undefined) {
                        const factor = Math.pow(eyeTransform.squash, config.intensity);
                        const newHeight = base.height * factor;
                        shape.height = newHeight;
                        shape.y = base.y + (base.height - newHeight) / 2;
                    }
                    if (eyeTransform.rotate !== undefined) {
                        shape.rotation += eyeTransform.rotate * config.intensity;
                    }
                }
            }
            else if (transform.type === 'translate') {
                if (transform.x !== undefined) shape.x += transform.x * config.intensity;
                if (transform.y !== undefined) shape.y += transform.y * config.intensity;

                if (transform.scale) {
                    if (transform.scale.w) {
                        const newWidth = base.width * transform.scale.w;
                        shape.width = newWidth;
                        shape.x -= (newWidth - base.width) / 2;
                    }
                    if (transform.scale.h) {
                        const newHeight = base.height * transform.scale.h;
                        shape.height = newHeight;
                        shape.y -= (newHeight - base.height) / 2;
                    }
                }

                if (transform.rotate) {
                    if (typeof transform.rotate === 'object') {
                        const rotValue = isRight ? transform.rotate.right : transform.rotate.left;
                        shape.rotation += rotValue * config.intensity;
                    } else {
                        shape.rotation += transform.rotate * config.intensity;
                    }
                }
            }
            else if (transform.type === 'tilt') {
                // Rotation for both eyes (confused head tilt)
                shape.rotation += transform.rotate * config.intensity;
            }
            else if (transform.type === 'rotate-absolute') {
                // Absolute rotation (for dizzy spin)
                shape.rotation = transform.angle * config.intensity;
            }
        };

        // Generate frames from template
        template.frames.forEach(frameConfig => {
            const frame = new Frame();
            frame.duration = Math.round(frameConfig.duration / config.speed);
            frame.easing = frameConfig.easing || 'linear';

            frame.shapes = baseShapes.map((base, idx) => {
                const s = base.clone(true);

                // Skip animation for images (keep them static)
                if (s.type === 'image') return s;

                const shapeCenterX = s.x + s.width / 2;
                const isRight = shapeCenterX > centerX;

                applyTransform(s, base, frameConfig.transform, isRight);
                return s;
            });

            state.frames.push(frame);
        });

        state.currentFrameIndex = startIndex;

        // Refresh UI
        if (typeof renderTimeline === 'function') renderTimeline();
        if (state.actions.renderEditor) state.actions.renderEditor();
        if (state.actions.renderPreview) state.actions.renderPreview();
        updateLayersPanel();
        updateFrameInfo();

        const speedInfo = config.speed !== 1.0 ? ` (${config.speed}x speed)` : '';
        const intensityInfo = config.intensity !== 1.0 ? ` (${config.intensity}x intensity)` : '';
        showToast(`Anim: ${type.toUpperCase()} Appended${speedInfo}${intensityInfo} ✨ (Total: ${state.frames.length} frames)`);

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
