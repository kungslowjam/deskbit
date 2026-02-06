import { state } from './state.js';
import { getPixelIndex, getCoordsFromEvent, rgbToHex } from './utils.js';
// renderer imports removed to prevent circular dependency
import { updateFrameInfo, updateToolButtons, updateLayersPanel, showToast, updateInteractionEditor } from './ui.js';
import { renderTimeline } from './timeline.js';
import { Shape, Frame } from './models.js';
import { undo, redo, saveUndo } from './history.js';
import { EYE_PATHS, EYE_ROTATIONS, ANIMATION_TEMPLATES } from './data/presets.js';

let canvas = null;

export function initTools(c) {
    canvas = c;
    state.EYE_PATHS = EYE_PATHS; // Share with state for timeline renderer
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
            // Ensure state integrity
            if (!state.selectedShapes) state.selectedShapes = [];

            // Sync legacy single-select if needed
            if (state.selectedShape && state.selectedShapes.length === 0) {
                state.selectedShapes = [state.selectedShape];
            } else if (state.selectedShape && !state.selectedShapes.some(s => s.id === state.selectedShape.id)) {
                // If primary is set but missing from list, add it or reset list?
                // Let's assume list is source of truth, but if empty, take primary.
                state.selectedShapes.push(state.selectedShape);
            }

            const mx = coords.x;
            const my = coords.y;

            // Check Resize Handles first (Only for primary selection 'selectedShape')
            if (state.selectedShape && state.selectedShapes.length === 1) { // Only resize if single selection
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
                // Check from top (last drawn) to bottom
                for (let i = frame.shapes.length - 1; i >= 0; i--) {
                    if (frame.shapes[i].containsPoint(coords.x, coords.y)) {
                        clickedShape = frame.shapes[i];
                        break;
                    }
                }
            }

            // Multi-Select Logic
            if (clickedShape) {
                if (e.ctrlKey || e.shiftKey) {
                    // Toggle Selection works differently for group add
                    const existingIdx = state.selectedShapes.findIndex(s => s.id === clickedShape.id);
                    if (existingIdx !== -1) {
                        // Already selected -> Deselect
                        state.selectedShapes.splice(existingIdx, 1);
                    } else {
                        // Add to selection
                        state.selectedShapes.push(clickedShape);
                    }
                    // Update Primary (last one usually)
                    state.selectedShape = state.selectedShapes.length > 0 ? state.selectedShapes[state.selectedShapes.length - 1] : null;
                } else {
                    // Normal Click
                    // If clicking a shape that is ALREADY part of the selection, allow dragging the whole group
                    // BUT only if we don't click a new one.
                    const isAlreadySelected = state.selectedShapes.some(s => s.id === clickedShape.id);

                    if (!isAlreadySelected) {
                        // Clicked a new unselected shape -> Clear and Select New
                        state.selectedShapes = [clickedShape];
                        state.selectedShape = clickedShape;
                    } else {
                        // Clicked an already selected shape -> Keep selection (allows drag), but update primary?
                        state.selectedShape = clickedShape;
                    }
                }
            } else {
                // Clicked Empty Space
                if (!e.ctrlKey && !e.shiftKey) {
                    state.selectedShapes = [];
                    state.selectedShape = null;
                }
            }

            state.isDraggingShape = !!state.selectedShape;

            // Prepare Drag Offsets for ALL selected shapes
            // Note: state.selectedShapes contains REFERENCES to objects in state.frames
            if (state.isDraggingShape) {
                state.dragOffsets = state.selectedShapes.map(s => ({
                    id: s.id,
                    dx: coords.x - s.x,
                    dy: coords.y - s.y
                }));
            }

            updateInteractionEditor();
            updateLayersPanel(); // Refresh selection UI
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

        // Group Dragging Logic
        if (state.isDraggingShape && state.dragOffsets && state.dragOffsets.length > 0) {
            state.dragOffsets.forEach(offset => {
                // Find the live shape object (it might have changed if we supported deeper updates, but refs should hold)
                // Just lookup by ID to be safe if refs were lost (unlikely here but good practice)
                const s = state.selectedShapes.find(shp => shp.id === offset.id);
                if (s) {
                    s.x = coords.x - offset.dx;
                    s.y = coords.y - offset.dy;
                }
            });
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
        state.dragOffsets = null; // Clear dragging offsets

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
                state.selectedShapes = [newShape]; // Auto-select new shape
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

    const createEye = (isRight) => {
        const pathData = EYE_PATHS[type] || EYE_PATHS.sad;
        const x = isRight ? centerX + padding : centerX - padding - w;
        const y = centerY - (h / 2);

        const s = new Shape('path', x, y, w, h, color);
        s.pathData = pathData;

        // Dynamic Rotation
        const rot = EYE_ROTATIONS[type] || { left: 0, right: 0 };
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

        // Get template or create a simple default
        const template = ANIMATION_TEMPLATES[type] || {
            frames: [
                { duration: 500, easing: 'ease-in-out', transform: 'reset' }
            ]
        };

        // Capture base shapes
        const baseShapes = currentFrame.shapes.map(s => s.clone(true));
        const centerX = state.GRID_WIDTH / 2;

        saveUndo();
        const startIndex = state.currentFrameIndex + 1; // Start applying from AFTER the current frame (NLE style)

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

            // Path Swapping (New Feature for Dizzy Animation)
            if (transform.path) {
                // If path is specified, replace the shape's path data
                // This allows frame-by-frame animation of the shape itself
                const newPath = EYE_PATHS[transform.path] || transform.path;
                if (newPath) {
                    shape.pathData = newPath;
                    // Reset cache to ensure redraw
                    if (shape.isCacheDirty !== undefined) shape.isCacheDirty = true;
                }
            }
        };

        // Generate frames from template
        template.frames.forEach((frameConfig, i) => {
            const frameIndex = startIndex + i;
            let frame;

            // Check if we can merge into existing frame
            if (frameIndex < state.frames.length) {
                // MERGE MODE: Modify existing frame
                frame = state.frames[frameIndex];

                // If the frame duration is significantly different, we might want to adjust it?
                // For now, let's keep the existing frame duration unless it's a single-frame static hold.
                // But animation presets assume specific timing. 
                // Let's force update duration if it's the "start" of a new merged sequence?
                // No, that breaks other tracks. Complex NLE issue.
                // Compromise: We map the template frame 1:1 to existing frames regardless of duration for now, 
                // OR we presume the user wants the animation timing more than the old timing.
                // Let's UPDATE the duration to match the preset if we are overwriting.
                frame.duration = Math.round(frameConfig.duration / config.speed);
                frame.easing = frameConfig.easing || 'linear';
                frame.label = (frame.label ? frame.label + '+' : '') + type.toUpperCase(); // Append label

                // Re-map shapes based on the NEW base (which is the PREVIOUS frame's state for continuity)
                // Actually, for merging, we should take the shapes from the CURRENT existing frame 
                // and just modify the target ones.

                // Find shapes to modify in this existing frame
                frame.shapes = frame.shapes.map(s => {
                    const shapeCenterX = s.x + s.width / 2;
                    const isRight = shapeCenterX > centerX;
                    let shouldAnimate = false;

                    if (state.selectedShapes && state.selectedShapes.length > 0) {
                        shouldAnimate = state.selectedShapes.some(sel => sel.id === s.id);
                    } else if (state.selectedShape) {
                        shouldAnimate = (s.id === state.selectedShape.id);
                    } else {
                        const isKnownEye = Object.values(EYE_PATHS).includes(s.pathData);
                        if (isKnownEye) shouldAnimate = true;
                    }

                    if (shouldAnimate) {
                        // Apply transform to this shape
                        // We need a 'base' to transform FROM. 
                        // Ideally, we transform relative to the shape's state at the START of the animation sequence
                        // to avoid accumulating errors or "walking" away.
                        // But existing logic creates absolute frames from a single base.

                        // Let's use the 'baseShapes' captured at start as the reference "Neutral Pose".
                        // Find the corresponding base shape
                        const originalBase = baseShapes.find(b => b.id === s.id) || baseShapes[baseShapes.findIndex(b => b.type === s.type)]; // heuristic fallback

                        if (originalBase) {
                            // Reset to base + Apply Transform
                            const temp = originalBase.clone(true);
                            applyTransform(temp, originalBase, frameConfig.transform, isRight);

                            // Copy properties back to s
                            s.x = temp.x;
                            s.y = temp.y;
                            s.width = temp.width;
                            s.height = temp.height;
                            s.rotation = temp.rotation;
                        } else {
                            // Just apply relative to itself if no base found (shim)
                            applyTransform(s, s.clone(), frameConfig.transform, isRight);
                        }
                    }
                    return s;
                });

            } else {
                // APPEND MODE: Create new frame (Classic behavior)
                frame = new Frame();
                frame.duration = Math.round(frameConfig.duration / config.speed);
                frame.easing = frameConfig.easing || 'linear';
                frame.label = type.toUpperCase();

                frame.shapes = baseShapes.map((base, idx) => {
                    const s = base.clone(true);

                    // Skip animation for images (keep them static)
                    if (s.type === 'image') return s;

                    const shapeCenterX = s.x + s.width / 2;
                    const isRight = shapeCenterX > centerX;

                    // Smart Detection Logic
                    let shouldAnimate = false;

                    if (state.selectedShapes && state.selectedShapes.length > 0) {
                        shouldAnimate = state.selectedShapes.some(sel => sel.id === s.id);
                    } else if (state.selectedShape) {
                        shouldAnimate = (s.id === state.selectedShape.id);
                    } else {
                        // 2. Global Mode: Smartly detect "Eyes"
                        const isKnownEye = Object.values(EYE_PATHS).includes(s.pathData);
                        if (isKnownEye) {
                            shouldAnimate = true;
                        }
                    }

                    if (shouldAnimate) {
                        applyTransform(s, base, frameConfig.transform, isRight);
                    }

                    return s;
                });
                state.frames.push(frame);
            }
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
