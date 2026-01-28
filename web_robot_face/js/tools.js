import { state } from './state.js';
import { getPixelIndex, getCoordsFromEvent, rgbToHex } from './utils.js';
// renderer imports removed to prevent circular dependency
import { updateFrameInfo, updateToolButtons, updateLayersPanel, showToast, updateInteractionEditor } from './ui.js';
import { renderTimeline } from './timeline.js';
import { Shape } from './models.js';
import { undo, redo } from './history.js';

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

    // Normalized Paths (0-100 coordinate space) - High Fidelity
    const paths = {
        // Angry: Sharp Crescent (High contrast curves)
        angry: "M0,60 Q50,15 100,60 Q50,95 0,60 Z",

        // Alert: Wide Egg (Tapered bottom)
        alert: "M50,5 C80,5 100,35 100,65 C100,90 80,100 50,100 C20,100 0,90 0,65 C0,35 20,5 50,5 Z",

        // Sad/Neutral: Friendly Squircle (Soft corners)
        sad: "M10,15 Q50,5 90,15 Q100,50 90,85 Q50,95 10,85 Q0,50 10,15 Z",

        // Evil: Thin Fox Eye (Very Sharp)
        evil: "M0,50 Q50,25 100,50 Q50,75 0,50 Z",

        // Bean: Perfect Half-Round
        bean: "M2,50 L98,50 Q98,100 50,100 Q2,100 2,50 Z",

        // Slit: Clean Rounded Bar
        slit: "M5,42 L95,42 Q100,42 100,50 Q100,58 95,58 L5,58 Q0,58 0,50 Q0,42 5,42 Z"
    };

    const createEye = (isRight) => {
        const pathData = paths[type] || paths.sad;
        const x = isRight ? centerX + padding : centerX - padding - w;
        const y = centerY - (h / 2);

        const s = new Shape('path', x, y, w, h, color);
        s.pathData = pathData;

        // Rotation Logic for "Expression"
        if (type === 'angry') {
            const rot = 25;
            s.rotation = isRight ? -rot : rot;
            // s.y += 10; 
        } else if (type === 'evil') {
            const rot = 15;
            s.rotation = isRight ? -rot : rot;
        } else if (type === 'sad') {
            s.rotation = isRight ? 10 : -10;
        }

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
    updateLayersPanel();
    renderTimeline();
    showToast(`Preset: ${type.toUpperCase()} Applied`);
}

export function duplicateShape() {
    if (!state.selectedShape) {
        alert('Select a shape first!'); // showToast?
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
