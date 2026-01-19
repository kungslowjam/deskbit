// Constants
let GRID_WIDTH = 466;
let GRID_HEIGHT = 466;

// State
let frames = [];
let currentFrameIndex = 0;
let isDrawing = false;
let isPlaying = false;
let currentTool = 'pen';
let currentColor = '#00ffff';

// Multi-Select & Clipboard
let selectedFrames = [];
let clipboard = [];
let clipboardMode = null;

// File Management
let localFiles = {};
let currentFileName = 'default_anim';

// DOM Elements
let canvas, ctx, previewCanvas, previewCtx, timelineTrack;

// Shape Object System
let selectedShape = null;

// Zoom & Pan System
let zoomLevel = 1;
const MIN_ZOOM = 0.25;
const MAX_ZOOM = 8;

// Timeline Globals
const PIXELS_PER_SEC = 100;

// Shape Class
class Shape {
    constructor(type, x, y, w, h, color) {
        this.id = Date.now() + Math.random();
        this.type = type;
        this.x = x;
        this.y = y;
        this.width = w;
        this.height = h;
        this.color = color;
        this.rotation = 0;
        this.opacity = parseFloat(document.getElementById('opacity-slider')?.value || 1);
        this.blendMode = document.getElementById('blend-mode-select')?.value || 'source-over';
        this.lineEnd = null;
    }

    containsPoint(px, py) {
        if (this.type === 'rect') {
            return px >= this.x && px <= this.x + this.width &&
                py >= this.y && py <= this.y + this.height;
        } else if (this.type === 'ellipse') {
            const cx = this.x + this.width / 2;
            const cy = this.y + this.height / 2;
            const rx = this.width / 2;
            const ry = this.height / 2;
            return Math.pow((px - cx) / rx, 2) + Math.pow((py - cy) / ry, 2) <= 1;
        } else if (this.type === 'line' && this.lineEnd) {
            // Distance to line segment
            const A = px - this.x;
            const B = py - this.y;
            const C = this.lineEnd.x - this.x;
            const D = this.lineEnd.y - this.y;
            const dot = A * C + B * D;
            const lenSq = C * C + D * D;
            let param = lenSq !== 0 ? dot / lenSq : -1;
            param = Math.max(0, Math.min(1, param));
            const xx = this.x + param * C;
            const yy = this.y + param * D;
            const dist = Math.sqrt((px - xx) ** 2 + (py - yy) ** 2);
            return dist < 5; // 5 pixel tolerance
        }
        return false;
    }

    getBounds() {
        if (this.type === 'line' && this.lineEnd) {
            return {
                x: Math.min(this.x, this.lineEnd.x),
                y: Math.min(this.y, this.lineEnd.y),
                width: Math.abs(this.lineEnd.x - this.x),
                height: Math.abs(this.lineEnd.y - this.y)
            };
        }
        return { x: this.x, y: this.y, width: this.width, height: this.height };
    }

    clone(preserveId = false) {
        const newShape = new Shape(this.type, this.x, this.y, this.width, this.height, this.color);
        newShape.rotation = this.rotation;
        newShape.opacity = this.opacity;
        newShape.blendMode = this.blendMode;
        if (this.lineEnd) {
            newShape.lineEnd = { ...this.lineEnd };
        }
        if (preserveId) {
            newShape.id = this.id; // Keep ID for tweening
        }
        return newShape;
    }

    draw(context) {
        context.save();
        context.globalAlpha = this.opacity;
        context.globalCompositeOperation = this.blendMode;

        context.fillStyle = this.color;
        context.strokeStyle = this.color;
        context.lineWidth = 2;

        if (this.type === 'rect') {
            context.fillRect(this.x, this.y, this.width, this.height);
        } else if (this.type === 'ellipse') {
            context.beginPath();
            context.ellipse(this.x + this.width / 2, this.y + this.height / 2,
                this.width / 2, this.height / 2, 0, 0, Math.PI * 2);
            context.fill();
        } else if (this.type === 'line' && this.lineEnd) {
            context.beginPath();
            context.moveTo(this.x, this.y);
            context.lineTo(this.lineEnd.x, this.lineEnd.y);
            context.stroke();
        }
        context.restore();
    }
}

// Frame Class
class Frame {
    constructor() {
        this.pixels = new Array(GRID_WIDTH * GRID_HEIGHT).fill(null);
        this.shapes = [];
        this.duration = 100;
        this.id = Date.now() + Math.random();
    }
}

// Global setColor function for palette swatches
function setColor(color) {
    currentColor = color;
    const picker = document.getElementById('color-picker');
    if (picker) picker.value = color;

    // Update palette swatch highlighting
    document.querySelectorAll('.palette-swatch').forEach(swatch => {
        swatch.classList.remove('active');
        if (swatch.style.backgroundColor === color ||
            rgbToHex(swatch.style.backgroundColor) === color.toLowerCase()) {
            swatch.classList.add('active');
        }
    });
}

// Helper to convert rgb to hex
function rgbToHex(rgb) {
    if (rgb.startsWith('#')) return rgb;
    const result = rgb.match(/\d+/g);
    if (!result || result.length < 3) return rgb;
    return '#' + result.slice(0, 3).map(x => parseInt(x).toString(16).padStart(2, '0')).join('');
}

// Initialize
function init() {
    canvas = document.getElementById('editor-canvas');
    ctx = canvas.getContext('2d');
    previewCanvas = document.getElementById('preview-canvas');
    previewCtx = previewCanvas.getContext('2d');
    timelineTrack = document.getElementById('main-track');

    document.getElementById('canvas-width').value = GRID_WIDTH;
    document.getElementById('canvas-height').value = GRID_HEIGHT;

    // Toolbar Events
    document.querySelectorAll('.tool-btn').forEach(btn => {
        btn.addEventListener('click', (e) => {
            const btnId = e.currentTarget.id;

            if (btnId.startsWith('action-')) {
                const action = btnId.replace('action-', '');
                if (action === 'clear') clearCanvas();
                else if (action === 'invert') invertColors();
                else if (action === 'undo') undo();
                return;
            }

            const toolName = btnId.replace('tool-', '');
            currentTool = toolName;

            document.querySelectorAll('.tool-btn').forEach(b => {
                if (!b.id.startsWith('action-')) b.classList.remove('active');
            });
            e.currentTarget.classList.add('active');
        });
    });

    // Timeline Controls
    const addBtn = document.getElementById('add-frame-btn');
    const dupBtn = document.getElementById('dup-frame-btn');
    const delBtn = document.getElementById('del-frame-btn');

    if (addBtn) addBtn.onclick = () => { console.log('Add Frame Clicked'); addFrame(); };
    if (dupBtn) dupBtn.onclick = () => { console.log('Dup Frame Clicked'); duplicateFrame(); };
    if (delBtn) delBtn.onclick = () => { console.log('Del Frame Clicked'); deleteFrame(); };

    // Playback Controls - FIXED!
    const playBtn = document.getElementById('play-btn');
    const stopBtn = document.getElementById('stop-btn');
    const exportCBtn = document.getElementById('export-c-btn');
    const exportJsonBtn = document.getElementById('export-btn');

    if (playBtn) playBtn.onclick = startPlay;
    if (stopBtn) stopBtn.onclick = stopPlay;
    if (exportCBtn) exportCBtn.onclick = exportToC;
    if (exportJsonBtn) exportJsonBtn.onclick = exportToJSON;

    // Color Picker - FIXED!
    const colorPicker = document.getElementById('color-picker');
    if (colorPicker) {
        colorPicker.addEventListener('input', (e) => {
            setColor(e.target.value);
        });
    }

    // Brush Size - FIXED!
    const brushSizeSlider = document.getElementById('brush-size');
    const brushSizeValue = document.getElementById('brush-size-value');
    if (brushSizeSlider) {
        brushSizeSlider.addEventListener('input', (e) => {
            brushSize = parseInt(e.target.value);
            if (brushSizeValue) brushSizeValue.textContent = brushSize;
        });
    }

    // FPS / Speed Control - FIXED!
    const fpsRange = document.getElementById('fps-range');
    const fpsValue = document.getElementById('fps-value');
    if (fpsRange) {
        fpsRange.addEventListener('input', (e) => {
            const fps = parseInt(e.target.value);
            if (fpsValue) fpsValue.textContent = `${fps} FPS`;
            // Update frame durations based on FPS if needed
        });
    }

    // Canvas Size Controls - FIXED!
    const canvasWidthInput = document.getElementById('canvas-width');
    const canvasHeightInput = document.getElementById('canvas-height');

    if (canvasWidthInput) {
        canvasWidthInput.addEventListener('change', (e) => {
            resizeCanvas(parseInt(e.target.value), GRID_HEIGHT);
        });
    }
    if (canvasHeightInput) {
        canvasHeightInput.addEventListener('change', (e) => {
            resizeCanvas(GRID_WIDTH, parseInt(e.target.value));
        });
    }

    // Zoom Controls - FIXED!
    const zoomInBtn = document.getElementById('zoom-in-btn');
    const zoomOutBtn = document.getElementById('zoom-out-btn');
    const zoomResetBtn = document.getElementById('zoom-reset-btn');
    const zoomFitBtn = document.getElementById('zoom-fit-btn');

    if (zoomInBtn) zoomInBtn.onclick = () => setZoom(zoomLevel * 1.25);
    if (zoomOutBtn) zoomOutBtn.onclick = () => setZoom(zoomLevel * 0.8);
    if (zoomResetBtn) zoomResetBtn.onclick = () => setZoom(1);
    if (zoomFitBtn) zoomFitBtn.onclick = fitZoom;

    // Opacity Slider - FIXED! (For selected shape)
    const opacitySlider = document.getElementById('opacity-slider');
    if (opacitySlider) {
        opacitySlider.addEventListener('input', (e) => {
            if (selectedShape) {
                selectedShape.opacity = parseFloat(e.target.value);
                renderEditor();
                renderTimeline();
            }
        });
    }

    // Blend Mode Select - FIXED!
    const blendModeSelect = document.getElementById('blend-mode-select');
    if (blendModeSelect) {
        blendModeSelect.addEventListener('change', (e) => {
            if (selectedShape) {
                selectedShape.blendMode = e.target.value;
                renderEditor();
                renderTimeline();
            }
        });
    }

    // Frame Duration - FIXED!
    const frameDurationInput = document.getElementById('frame-duration');
    if (frameDurationInput) {
        frameDurationInput.addEventListener('change', (e) => {
            const frame = frames[currentFrameIndex];
            if (frame) {
                frame.duration = parseInt(e.target.value) || 100;
                renderTimeline();
                updateFrameInfo();
            }
        });
    }

    // Import JSON - FIXED!
    const importBtn = document.getElementById('import-btn');
    if (importBtn) importBtn.onclick = importFromJSON;

    // Onion Skinning Toggle - FIXED!
    const onionSkinToggle = document.getElementById('onion-skin-toggle');
    if (onionSkinToggle) {
        onionSkinToggle.addEventListener('change', (e) => {
            toggleOnionSkin(e.target.checked);
        });
    }

    if (GRID_WIDTH === 466 && GRID_HEIGHT === 466) {
        document.getElementById('round-toggle').checked = true;
        toggleRound(true);
    }

    document.getElementById('round-toggle').addEventListener('change', (e) => {
        toggleRound(e.target.checked);
    });

    // Animation Name Input - Sync with current file
    const animNameInput = document.getElementById('anim-name');
    if (animNameInput) {
        animNameInput.addEventListener('change', (e) => {
            let newName = e.target.value.replace(/[^a-zA-Z0-9_-]/g, '_');
            if (!newName) newName = 'my_anim';
            e.target.value = newName;
            currentFileName = newName;
            // Update local files
            if (localFiles[currentFileName]) {
                delete localFiles[currentFileName];
            }
            localFiles[newName] = frames;
        });
    }

    initRuler();

    canvas.width = GRID_WIDTH;
    canvas.height = GRID_HEIGHT;
    previewCanvas.width = GRID_WIDTH;
    previewCanvas.height = GRID_HEIGHT;

    // Create default frame
    frames = [new Frame()];
    currentFrameIndex = 0;
    currentFileName = animNameInput?.value || 'my_anim';
    localFiles[currentFileName] = frames;

    console.log("✅ Initialized with", frames.length, "frame(s)");

    renderTimeline();
    renderEditor();
    renderPreview(); // Initial preview render
    setupEventListeners();
    updateFrameInfo();

    // Initial zoom fit
    setTimeout(fitZoom, 100);
}

// Initialize Timeline Ruler
function initRuler() {
    const ruler = document.getElementById('timeline-ruler');
    if (!ruler) return;

    const ticksContainer = document.createElement('div');
    ticksContainer.className = 'ruler-ticks';
    ruler.innerHTML = '';
    ruler.appendChild(ticksContainer);

    const playhead = document.createElement('div');
    playhead.id = 'playhead';
    playhead.className = 'playhead';
    playhead.innerHTML = '<div class="playhead-marker"></div>';
    ruler.appendChild(playhead);

    const totalSeconds = 60;
    for (let i = 0; i <= totalSeconds; i++) {
        const tick = document.createElement('div');
        tick.className = 'ruler-tick';
        tick.style.left = `${i * PIXELS_PER_SEC}px`;
        tick.innerText = `${i}s`;
        ticksContainer.appendChild(tick);

        for (let j = 1; j < 10; j++) {
            const subtick = document.createElement('div');
            subtick.className = 'ruler-subtick';
            subtick.style.left = `${(i * PIXELS_PER_SEC) + (j * (PIXELS_PER_SEC / 10))}px`;
            ticksContainer.appendChild(subtick);
        }
    }
}

// Render Timeline
function renderTimeline() {
    if (!timelineTrack) {
        console.error("❌ Timeline track not found!");
        return;
    }

    timelineTrack.innerHTML = '';
    let currentX = 0;

    frames.forEach((frame, index) => {
        const width = Math.max(80, (frame.duration / 1000) * PIXELS_PER_SEC); // Min 80px wide

        const block = document.createElement('div');
        block.className = `frame-block ${index === currentFrameIndex ? 'active' : ''}`;
        block.style.left = `${currentX}px`;
        block.style.width = `${width}px`;
        block.dataset.index = index;

        // Create thumbnail canvas
        const thumbCanvas = document.createElement('canvas');
        thumbCanvas.width = GRID_WIDTH;
        thumbCanvas.height = GRID_HEIGHT;
        thumbCanvas.className = 'frame-thumbnail';
        const thumbCtx = thumbCanvas.getContext('2d');

        // Render frame to thumbnail
        thumbCtx.fillStyle = '#000';
        thumbCtx.fillRect(0, 0, GRID_WIDTH, GRID_HEIGHT);

        // Draw pixels
        const imgData = thumbCtx.createImageData(GRID_WIDTH, GRID_HEIGHT);
        const data = imgData.data;
        for (let i = 0; i < frame.pixels.length; i++) {
            const color = frame.pixels[i];
            if (color) {
                const r = parseInt(color.slice(1, 3), 16);
                const g = parseInt(color.slice(3, 5), 16);
                const b = parseInt(color.slice(5, 7), 16);
                const idx = i * 4;
                data[idx] = r;
                data[idx + 1] = g;
                data[idx + 2] = b;
                data[idx + 3] = 255;
            }
        }
        thumbCtx.putImageData(imgData, 0, 0);

        // Draw shapes on thumbnail
        if (frame.shapes && frame.shapes.length > 0) {
            frame.shapes.forEach(shape => shape.draw(thumbCtx));
        }

        block.appendChild(thumbCanvas);

        // Frame info overlay
        const infoDiv = document.createElement('div');
        infoDiv.className = 'frame-info';
        infoDiv.innerHTML = `
            <span class="frame-number">#${index + 1}</span>
            <span class="frame-duration">${frame.duration}ms</span>
        `;
        block.appendChild(infoDiv);

        block.onclick = (e) => {
            // Don't select if clicking on resize handle
            if (e.target.classList.contains('resize-handle')) return;

            e.stopPropagation();
            currentFrameIndex = index;
            selectedFrames = [index];
            renderTimeline();
            renderEditor();
        };

        // Resize Handle
        const handle = document.createElement('div');
        handle.className = 'resize-handle';
        handle.dataset.index = index;
        handle.innerHTML = '⋮⋮'; // Visual indicator

        // Resize drag handlers
        handle.onmousedown = (e) => {
            e.preventDefault();
            e.stopPropagation();
            startFrameResize(index, e);
        };

        block.appendChild(handle);
        timelineTrack.appendChild(block);
        currentX += width;
    });

    console.log("✅ Timeline rendered with", frames.length, "frames");
}

// Frame Resize System
let isResizingFrame = false;
let resizeFrameIndex = -1;
let resizeStartX = 0;
let resizeStartDuration = 0;

function startFrameResize(frameIndex, e) {
    isResizingFrame = true;
    resizeFrameIndex = frameIndex;
    resizeStartX = e.clientX;
    resizeStartDuration = frames[frameIndex].duration;

    document.body.style.cursor = 'ew-resize';
    document.addEventListener('mousemove', doFrameResize);
    document.addEventListener('mouseup', endFrameResize);
}

function doFrameResize(e) {
    if (!isResizingFrame || resizeFrameIndex < 0) return;

    const deltaX = e.clientX - resizeStartX;

    // Calculate new duration: PIXELS_PER_SEC pixels = 1000ms
    // So 1 pixel = 1000/PIXELS_PER_SEC ms = 10ms
    const msPerPixel = 1000 / PIXELS_PER_SEC;
    const deltaDuration = deltaX * msPerPixel;

    let newDuration = resizeStartDuration + deltaDuration;

    // Clamp to reasonable range (10ms - 10000ms)
    newDuration = Math.max(10, Math.min(10000, Math.round(newDuration)));

    // Update frame data
    frames[resizeFrameIndex].duration = newDuration;

    // Update UI immediately
    const block = timelineTrack.querySelector(`[data-index="${resizeFrameIndex}"]`);
    if (block) {
        const newWidth = (newDuration / 1000) * PIXELS_PER_SEC;
        block.style.width = `${newWidth}px`;
        block.title = `Frame ${resizeFrameIndex + 1} (${newDuration}ms)`;
    }

    // Update following frames' positions
    let cumulativeX = 0;
    for (let i = 0; i < frames.length; i++) {
        const frameBlock = timelineTrack.querySelector(`[data-index="${i}"]`);
        if (frameBlock) {
            frameBlock.style.left = `${cumulativeX}px`;
            cumulativeX += (frames[i].duration / 1000) * PIXELS_PER_SEC;
        }
    }
}

function endFrameResize(e) {
    if (!isResizingFrame) return;

    isResizingFrame = false;
    resizeFrameIndex = -1;
    document.body.style.cursor = '';

    document.removeEventListener('mousemove', doFrameResize);
    document.removeEventListener('mouseup', endFrameResize);

    // Full re-render to ensure consistency
    renderTimeline();
}

// Timeline Actions
function addFrame() {
    frames.push(new Frame());
    currentFrameIndex = frames.length - 1;
    selectedFrames = [currentFrameIndex];
    renderTimeline();
    renderEditor();
}

function duplicateFrame() {
    if (frames.length === 0) {
        addFrame();
        return;
    }

    const srcFrame = frames[currentFrameIndex];
    if (!srcFrame) return;

    const newFrame = new Frame();
    newFrame.duration = srcFrame.duration;

    // Copy Pixels
    newFrame.pixels = [...srcFrame.pixels];

    // Copy Shapes (Maintain IDs for morphing!)
    if (srcFrame.shapes) {
        newFrame.shapes = srcFrame.shapes.map(s => s.clone(true));
    }

    frames.splice(currentFrameIndex + 1, 0, newFrame);
    currentFrameIndex++;
    selectedFrames = [currentFrameIndex];

    renderTimeline();
    renderEditor();
}

function deleteFrame() {
    if (frames.length <= 1) {
        frames[0].pixels.fill(null);
        frames[0].shapes = [];
        renderEditor();
        renderTimeline();
        return;
    }
    frames.splice(currentFrameIndex, 1);
    if (currentFrameIndex >= frames.length) {
        currentFrameIndex = frames.length - 1;
    }
    renderTimeline();
    renderEditor();
}

// Drawing
let brushSize = 1;
let startX = 0, startY = 0;
let undoStack = [];

function getPixelIndex(x, y) {
    if (x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) return -1;
    return y * GRID_WIDTH + x;
}

function getCoordsFromEvent(e) {
    const rect = canvas.getBoundingClientRect();
    const scaleX = canvas.width / rect.width;
    const scaleY = canvas.height / rect.height;

    let x = (e.clientX - rect.left) * scaleX;
    let y = (e.clientY - rect.top) * scaleY;

    x = Math.max(0, Math.min(GRID_WIDTH - 1, Math.floor(x)));
    y = Math.max(0, Math.min(GRID_HEIGHT - 1, Math.floor(y)));

    return { x, y };
}

function saveUndo() {
    const frame = frames[currentFrameIndex];
    if (!frame) return;
    undoStack.push([...frame.pixels]);
    if (undoStack.length > 20) undoStack.shift();
}

function undo() {
    if (undoStack.length === 0) return;
    const frame = frames[currentFrameIndex];
    if (!frame) return;
    frame.pixels = undoStack.pop();
    renderEditor();
}

function paint(x, y) {
    const frame = frames[currentFrameIndex];
    if (!frame) return;

    if (currentTool === 'pen' || currentTool === 'brush') {
        drawBrush(x, y, currentColor, brushSize);
    } else if (currentTool === 'eraser') {
        drawBrush(x, y, null, brushSize);
    } else if (currentTool === 'fill') {
        floodFill(x, y, currentColor);
        renderEditor();
    }
}

function drawBrush(cx, cy, color, size) {
    const frame = frames[currentFrameIndex];
    if (!frame) return;
    const half = Math.floor(size / 2);
    for (let dy = -half; dy <= half; dy++) {
        for (let dx = -half; dx <= half; dx++) {
            const x = cx + dx;
            const y = cy + dy;
            const idx = getPixelIndex(x, y);
            if (idx !== -1) {
                frame.pixels[idx] = color;
                drawPixel(x, y, color);
            }
        }
    }
}

function drawPixel(x, y, color) {
    ctx.fillStyle = color || '#000000';
    ctx.fillRect(x, y, 1, 1);
}

function floodFill(startX, startY, color) {
    const frame = frames[currentFrameIndex];
    const startIdx = getPixelIndex(startX, startY);
    const targetColor = frame.pixels[startIdx];
    if (targetColor === color) return;

    const stack = [[startX, startY]];
    while (stack.length) {
        const [x, y] = stack.pop();
        const idx = getPixelIndex(x, y);
        if (idx !== -1 && frame.pixels[idx] === targetColor) {
            frame.pixels[idx] = color;
            stack.push([x + 1, y]);
            stack.push([x - 1, y]);
            stack.push([x, y + 1]);
            stack.push([x, y - 1]);
        }
    }
}

// Rendering
function renderEditor() {
    ctx.fillStyle = '#000000';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    const frame = frames[currentFrameIndex];
    if (!frame) return;

    const imgData = ctx.createImageData(canvas.width, canvas.height);
    const data = imgData.data;

    for (let i = 0; i < frame.pixels.length; i++) {
        const color = frame.pixels[i];
        if (color) {
            const r = parseInt(color.slice(1, 3), 16);
            const g = parseInt(color.slice(3, 5), 16);
            const b = parseInt(color.slice(5, 7), 16);
            const idx = i * 4;
            data[idx] = r;
            data[idx + 1] = g;
            data[idx + 2] = b;
            data[idx + 3] = 255;
        }
    }
    ctx.putImageData(imgData, 0, 0);

    if (frame.shapes && frame.shapes.length > 0) {
        frame.shapes.forEach(shape => shape.draw(ctx));
    }

    // Draw selection box if shape is selected
    if (selectedShape) {
        const bounds = selectedShape.getBounds();
        const padding = 4;

        ctx.strokeStyle = '#00d2ff';
        ctx.lineWidth = 2;
        ctx.setLineDash([5, 3]);
        ctx.strokeRect(
            bounds.x - padding,
            bounds.y - padding,
            bounds.width + padding * 2,
            bounds.height + padding * 2
        );
        ctx.setLineDash([]);

        // Draw resize handles (corners)
        const handleSize = 8;
        ctx.fillStyle = '#00d2ff';

        const corners = [
            { x: bounds.x - padding, y: bounds.y - padding },
            { x: bounds.x + bounds.width + padding - handleSize, y: bounds.y - padding },
            { x: bounds.x - padding, y: bounds.y + bounds.height + padding - handleSize },
            { x: bounds.x + bounds.width + padding - handleSize, y: bounds.y + bounds.height + padding - handleSize }
        ];

        corners.forEach(corner => {
            ctx.fillRect(corner.x, corner.y, handleSize, handleSize);
        });
    }

    // Update preview canvas
    renderPreview();
}

// Canvas Actions
function clearCanvas() {
    const frame = frames[currentFrameIndex];
    if (frame) {
        saveUndo();
        frame.pixels.fill(null);
        frame.shapes = [];
        renderEditor();
        renderTimeline();
    }
}

function invertColors() {
    const frame = frames[currentFrameIndex];
    if (!frame) return;
    saveUndo();

    for (let i = 0; i < frame.pixels.length; i++) {
        if (frame.pixels[i]) {
            const hex = frame.pixels[i].replace('#', '');
            const r = (255 - parseInt(hex.substr(0, 2), 16)).toString(16).padStart(2, '0');
            const g = (255 - parseInt(hex.substr(2, 2), 16)).toString(16).padStart(2, '0');
            const b = (255 - parseInt(hex.substr(4, 2), 16)).toString(16).padStart(2, '0');
            frame.pixels[i] = '#' + r + g + b;
        }
    }
    renderEditor();
    renderTimeline();
}

function cloneShape(shape) {
    const newShape = new Shape(shape.type, shape.x, shape.y, shape.width, shape.height, shape.color);
    newShape.rotation = shape.rotation;
    if (shape.lineEnd) newShape.lineEnd = { ...shape.lineEnd };
    return newShape;
}

// Clipboard for shapes
let shapeClipboard = null;

// Helper to check if clicking on a resize handle
function getResizeHandle(px, py, shape) {
    if (!shape) return null;
    const bounds = shape.getBounds();
    const padding = 4;
    const size = 8;

    const handles = {
        'tl': { x: bounds.x - padding, y: bounds.y - padding },
        'tr': { x: bounds.x + bounds.width + padding - size, y: bounds.y - padding },
        'bl': { x: bounds.x - padding, y: bounds.y + bounds.height + padding - size },
        'br': { x: bounds.x + bounds.width + padding - size, y: bounds.y + bounds.height + padding - size }
    };

    for (const [key, h] of Object.entries(handles)) {
        if (px >= h.x && px <= h.x + size && py >= h.y && py <= h.y + size) {
            return key;
        }
    }
    return null;
}

// Setup Event Listeners
function setupEventListeners() {
    let tempStartX = 0, tempStartY = 0;
    let isDrawingShape = false;
    let isDraggingShape = false;
    let isResizingShape = false;
    let activeHandle = null;
    let dragOffsetX = 0, dragOffsetY = 0;
    let initialShapeState = null;

    canvas.onmousedown = (e) => {
        if (!frames[currentFrameIndex]) return;

        const coords = getCoordsFromEvent(e);
        tempStartX = coords.x;
        tempStartY = coords.y;

        // Select Tool
        if (currentTool === 'select') {
            const frame = frames[currentFrameIndex];

            // 1. Check Resize Handles first (if a shape is already selected)
            if (selectedShape) {
                const handle = getResizeHandle(coords.x, coords.y, selectedShape);
                if (handle) {
                    isResizingShape = true;
                    activeHandle = handle;
                    initialShapeState = { ...selectedShape }; // Clone props
                    saveUndo();
                    return;
                }
            }

            // 2. Check Shape Hit
            let clickedShape = null;
            // Check shapes in reverse order (top to bottom)
            for (let i = frame.shapes.length - 1; i >= 0; i--) {
                const shape = frame.shapes[i];
                if (shape.containsPoint && shape.containsPoint(coords.x, coords.y)) {
                    clickedShape = shape;
                    break;
                }
            }

            if (clickedShape) {
                selectedShape = clickedShape;
                isDraggingShape = true;
                dragOffsetX = coords.x - selectedShape.x;
                dragOffsetY = coords.y - selectedShape.y;
                saveUndo();
                renderEditor();
            } else {
                // Clicked empty space -> Deselect
                selectedShape = null;
                renderEditor();
            }
            return;
        }

        // Pixel drawing tools
        if (['pen', 'brush', 'eraser', 'fill', 'eyedropper'].includes(currentTool)) {
            isDrawing = true;
            startX = coords.x;
            startY = coords.y;
            saveUndo();
            paint(coords.x, coords.y);
        }
        // Shape tools (Pixel & Object)
        else {
            isDrawingShape = true;
            startX = coords.x;
            startY = coords.y;
            if (['line', 'rect', 'circle'].includes(currentTool)) {
                saveUndo();
            }
        }
    };

    canvas.onmousemove = (e) => {
        if (!frames[currentFrameIndex]) return;
        const coords = getCoordsFromEvent(e);

        // Resize Shape
        if (isResizingShape && selectedShape) {
            const dx = coords.x - tempStartX; // Delta from start click is tricky, better use absolute
            // Actually better to calculate based on handle role

            // Ideally we need start props + mouse delta
            // For simplicity, let's just adjust edges based on mouse pos

            if (activeHandle === 'br') {
                selectedShape.width = Math.max(1, coords.x - selectedShape.x);
                selectedShape.height = Math.max(1, coords.y - selectedShape.y);
            } else if (activeHandle === 'bl') {
                const oldRight = selectedShape.x + selectedShape.width;
                selectedShape.x = Math.min(coords.x, oldRight - 1);
                selectedShape.width = oldRight - selectedShape.x;
                selectedShape.height = Math.max(1, coords.y - selectedShape.y);
            } else if (activeHandle === 'tr') {
                const oldBottom = selectedShape.y + selectedShape.height;
                selectedShape.y = Math.min(coords.y, oldBottom - 1);
                selectedShape.height = oldBottom - selectedShape.y;
                selectedShape.width = Math.max(1, coords.x - selectedShape.x);
            } else if (activeHandle === 'tl') {
                const oldRight = selectedShape.x + selectedShape.width;
                const oldBottom = selectedShape.y + selectedShape.height;
                selectedShape.x = Math.min(coords.x, oldRight - 1);
                selectedShape.y = Math.min(coords.y, oldBottom - 1);
                selectedShape.width = oldRight - selectedShape.x;
                selectedShape.height = oldBottom - selectedShape.y;
            }

            // For Line Object
            if (selectedShape.type === 'line') {
                // Simplistic line resizing (move end point or start point?)
                // Currently handles logic is bounding-box based. 
                // For lines, standard handles might feel weird.
                // Let's stick to bounding box or add specific endpoints logic later.
                // For now, bounding box resize will stretch the line.
            }

            renderEditor();
            return;
        }

        // Move Shape
        if (isDraggingShape && selectedShape) {
            selectedShape.x = coords.x - dragOffsetX;
            selectedShape.y = coords.y - dragOffsetY;
            renderEditor();
            return;
        }

        // Pixel drawing
        if (isDrawing && ['pen', 'brush', 'eraser'].includes(currentTool)) {
            paint(coords.x, coords.y);
        }
        // Shape Preview
        else if (isDrawingShape) {
            renderEditor();
            ctx.save();
            ctx.strokeStyle = currentColor;
            ctx.fillStyle = currentColor;
            ctx.lineWidth = 2;
            ctx.globalAlpha = 0.5;

            if (currentTool.includes('line')) {
                ctx.beginPath();
                ctx.moveTo(startX, startY);
                ctx.lineTo(coords.x, coords.y);
                ctx.stroke();
            } else if (currentTool.includes('rect')) {
                ctx.strokeRect(startX, startY, coords.x - startX, coords.y - startY);
            } else if (currentTool.includes('circle') || currentTool.includes('ellipse')) {
                const w = Math.abs(coords.x - startX);
                const h = Math.abs(coords.y - startY);
                const cx = startX + (coords.x - startX) / 2;
                const cy = startY + (coords.y - startY) / 2;
                ctx.beginPath();
                ctx.ellipse(cx, cy, w / 2, h / 2, 0, 0, Math.PI * 2);
                ctx.stroke();
            }
            ctx.restore();
        }

        // Cursor updates for handles
        if (currentTool === 'select' && selectedShape && !isDraggingShape && !isResizingShape) {
            const handle = getResizeHandle(coords.x, coords.y, selectedShape);
            if (handle) {
                canvas.style.cursor = (handle === 'tl' || handle === 'br') ? 'nwse-resize' : 'nesw-resize';
            } else if (selectedShape.containsPoint(coords.x, coords.y)) {
                canvas.style.cursor = 'move';
            } else {
                canvas.style.cursor = 'default';
            }
        } else if (!isDraggingShape && !isResizingShape) {
            // Reset cursor for other tools
            canvas.style.cursor = 'crosshair';
        }
    };

    canvas.onmouseup = (e) => {
        if (!frames[currentFrameIndex]) return;
        const coords = getCoordsFromEvent(e);

        if (isResizingShape) {
            isResizingShape = false;
            activeHandle = null;
            renderTimeline();
            return;
        }

        if (isDraggingShape) {
            isDraggingShape = false;
            renderTimeline();
            return;
        }

        if (isDrawingShape) {
            if (['line', 'rect', 'circle'].includes(currentTool)) {
                if (currentTool === 'line') drawLineOnFrame(startX, startY, coords.x, coords.y, currentColor);
                else if (currentTool === 'rect') drawRectOnFrame(startX, startY, coords.x, coords.y, currentColor);
                else if (currentTool === 'circle') drawCircleOnFrame(startX, startY, coords.x, coords.y, currentColor);
            } else {
                // Object shapes
                const frame = frames[currentFrameIndex];
                if (currentTool === 'rect-obj') {
                    frame.shapes.push(new Shape('rect', startX, startY, coords.x - startX, coords.y - startY, currentColor));
                } else if (currentTool === 'ellipse-obj') {
                    // Normalize dimensions
                    const x = Math.min(startX, coords.x);
                    const y = Math.min(startY, coords.y);
                    const w = Math.abs(coords.x - startX);
                    const h = Math.abs(coords.y - startY);
                    frame.shapes.push(new Shape('ellipse', x, y, w, h, currentColor));
                } else if (currentTool === 'line-obj') {
                    const s = new Shape('line', startX, startY, 0, 0, currentColor);
                    s.lineEnd = { x: coords.x, y: coords.y };
                    frame.shapes.push(s);
                }
            }
            renderEditor();
            renderTimeline();
        }

        isDrawing = false;
        isDrawingShape = false;
    };

    canvas.onmouseleave = () => {
        isDrawing = false;
        isDrawingShape = false;
        isDraggingShape = false;
        isResizingShape = false;
    };

    // Keyboard Shortcuts
    document.addEventListener('keydown', (e) => {
        // Copy (Ctrl+C)
        if (e.ctrlKey && e.key.toLowerCase() === 'c') {
            if (selectedShape) {
                shapeClipboard = {
                    ...selectedShape, // Copy properties
                    lineEnd: selectedShape.lineEnd ? { ...selectedShape.lineEnd } : null
                };
                showToast("Shape Copied");
                e.preventDefault();
            }
        }

        // Paste (Ctrl+V)
        if (e.ctrlKey && e.key.toLowerCase() === 'v') {
            if (shapeClipboard) {
                const frame = frames[currentFrameIndex];
                if (!frame) return;

                // Create a REAL Shape instance to restore methods (draw, getBounds, etc.)
                const newShape = new Shape(
                    shapeClipboard.type,
                    shapeClipboard.x + 10, // Offset position
                    shapeClipboard.y + 10,
                    shapeClipboard.width,
                    shapeClipboard.height,
                    shapeClipboard.color
                );

                // Restore other properties
                newShape.rotation = shapeClipboard.rotation || 0;
                newShape.opacity = shapeClipboard.opacity !== undefined ? shapeClipboard.opacity : 1;
                newShape.blendMode = shapeClipboard.blendMode || 'source-over';

                if (shapeClipboard.lineEnd) {
                    newShape.lineEnd = {
                        x: shapeClipboard.lineEnd.x + 10,
                        y: shapeClipboard.lineEnd.y + 10
                    };
                }

                frame.shapes.push(newShape);
                selectedShape = newShape; // Select new shape

                renderEditor();
                renderTimeline();
                showToast("Shape Pasted");
                e.preventDefault();
            }
        }

        // Delete
        if (e.key === 'Delete' && selectedShape) {
            const frame = frames[currentFrameIndex];
            const idx = frame.shapes.indexOf(selectedShape);
            if (idx > -1) {
                frame.shapes.splice(idx, 1);
                selectedShape = null;
                renderEditor();
                renderTimeline();
            }
        }

        if (e.ctrlKey && e.key.toLowerCase() === 'z') { undo(); e.preventDefault(); }
        if (e.key.toLowerCase() === 'p') document.getElementById('tool-pen')?.click();
        if (e.key.toLowerCase() === 'b') document.getElementById('tool-brush')?.click();
        if (e.key.toLowerCase() === 'e') document.getElementById('tool-eraser')?.click();
        if (e.key.toLowerCase() === 'g') document.getElementById('tool-fill')?.click();
        if (e.key.toLowerCase() === 'r') document.getElementById('tool-rect')?.click();
        if (e.key.toLowerCase() === 'v') document.getElementById('tool-select')?.click();
    });
}

// Shape Drawing Functions (Pixel Mode)
function drawLineOnFrame(x0, y0, x1, y1, color) {
    const frame = frames[currentFrameIndex];
    if (!frame) return;

    // Bresenham's line algorithm
    const dx = Math.abs(x1 - x0);
    const dy = Math.abs(y1 - y0);
    const sx = x0 < x1 ? 1 : -1;
    const sy = y0 < y1 ? 1 : -1;
    let err = dx - dy;

    while (true) {
        const idx = getPixelIndex(x0, y0);
        if (idx !== -1) frame.pixels[idx] = color;
        if (x0 === x1 && y0 === y1) break;
        const e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

function drawRectOnFrame(x0, y0, x1, y1, color) {
    const frame = frames[currentFrameIndex];
    if (!frame) return;

    const minX = Math.min(x0, x1);
    const maxX = Math.max(x0, x1);
    const minY = Math.min(y0, y1);
    const maxY = Math.max(y0, y1);

    // Draw outline
    for (let x = minX; x <= maxX; x++) {
        const idx1 = getPixelIndex(x, minY);
        const idx2 = getPixelIndex(x, maxY);
        if (idx1 !== -1) frame.pixels[idx1] = color;
        if (idx2 !== -1) frame.pixels[idx2] = color;
    }
    for (let y = minY; y <= maxY; y++) {
        const idx1 = getPixelIndex(minX, y);
        const idx2 = getPixelIndex(maxX, y);
        if (idx1 !== -1) frame.pixels[idx1] = color;
        if (idx2 !== -1) frame.pixels[idx2] = color;
    }
}

function drawCircleOnFrame(cx, cy, ex, ey, color) {
    const frame = frames[currentFrameIndex];
    if (!frame) return;

    const radius = Math.round(Math.sqrt((ex - cx) ** 2 + (ey - cy) ** 2));

    // Midpoint circle algorithm
    let x = radius;
    let y = 0;
    let err = 0;

    while (x >= y) {
        setPixelSafe(cx + x, cy + y, color);
        setPixelSafe(cx + y, cy + x, color);
        setPixelSafe(cx - y, cy + x, color);
        setPixelSafe(cx - x, cy + y, color);
        setPixelSafe(cx - x, cy - y, color);
        setPixelSafe(cx - y, cy - x, color);
        setPixelSafe(cx + y, cy - x, color);
        setPixelSafe(cx + x, cy - y, color);

        y++;
        if (err <= 0) {
            err += 2 * y + 1;
        }
        if (err > 0) {
            x--;
            err -= 2 * x + 1;
        }
    }
}

function setPixelSafe(x, y, color) {
    const frame = frames[currentFrameIndex];
    const idx = getPixelIndex(x, y);
    if (idx !== -1) frame.pixels[idx] = color;
}

// Toggle Round Mode
function toggleRound(isRound) {
    const wrapper = document.getElementById('canvas-wrapper');
    const preview = document.querySelector('.preview-box');

    if (wrapper) {
        if (isRound) {
            wrapper.classList.add('canvas-round');
        } else {
            wrapper.classList.remove('canvas-round');
        }
    }

    if (preview) {
        if (isRound) {
            preview.style.borderRadius = '50%';
            preview.style.overflow = 'hidden';
        } else {
            preview.style.borderRadius = 'var(--radius-sm)';
            preview.style.overflow = 'visible';
        }
    }
}

// ======== ANIMATION PLAYBACK ========
let playTimerId = null;
let playStartTime = 0;
let playStartOffset = 0; // Time offset in total animation
let totalDuration = 0;
let animFrameId = null;

function calculateTotalDuration() {
    return frames.reduce((sum, f) => sum + parseInt(f.duration || 100), 0);
}

function startPlay() {
    if (frames.length === 0) return;

    // Toggle Pause if already playing
    if (isPlaying) {
        stopPlay();
        return;
    }

    isPlaying = true;
    const playBtn = document.getElementById('play-btn');
    if (playBtn) playBtn.innerHTML = '<span class="icon">⏸</span> Pause';

    // Calculate total duration map (Ensure Number)
    totalDuration = calculateTotalDuration();
    if (totalDuration === 0) totalDuration = 1000; // Fallback

    // Determine start time based on current frame
    let startTimeMs = 0;
    for (let i = 0; i < currentFrameIndex; i++) {
        startTimeMs += parseInt(frames[i].duration || 100);
    }

    // If at end, restart
    if (currentFrameIndex >= frames.length - 1) {
        currentFrameIndex = 0;
        startTimeMs = 0;
    }

    playStartOffset = startTimeMs;
    playStartTime = performance.now();

    // Logic Loop (Frame Switching)
    const animate = (now) => {
        if (!isPlaying) return;

        const elapsed = now - playStartTime + playStartOffset;
        let currentTime = elapsed % totalDuration; // Loop

        // Find which frame we are in
        let timeAccum = 0;
        let foundIndex = 0;
        let localTime = 0; // Time within current frame

        for (let i = 0; i < frames.length; i++) {
            const duration = parseInt(frames[i].duration || 100);
            if (currentTime >= timeAccum && currentTime < timeAccum + duration) {
                foundIndex = i;
                localTime = currentTime - timeAccum;
                break;
            }
            timeAccum += duration;
        }

        // 1. Update active frame if changed (Editor View)
        if (foundIndex !== currentFrameIndex) {
            currentFrameIndex = foundIndex;
            renderEditor();

            // Highlight active block
            document.querySelectorAll('.frame-block').forEach(b => b.classList.remove('active'));
            const activeBlock = document.querySelector(`.frame-block[data-index="${currentFrameIndex}"]`);
            if (activeBlock) {
                activeBlock.classList.add('active');
            }
        }

        // 2. Render Preview (With Interpolation)
        const frame = frames[currentFrameIndex];
        const nextFrameIndex = (currentFrameIndex + 1) % frames.length;
        const nextFrame = frames[nextFrameIndex];
        const frameDur = parseInt(frame.duration || 100);
        const t = Math.min(1, Math.max(0, localTime / frameDur)); // 0.0 to 1.0 progress

        renderPreview(currentFrameIndex, {
            nextFrame: nextFrame,
            t: t
        });

        // 3. Update Playhead (Smooth)
        const pixels = (currentTime / 1000) * PIXELS_PER_SEC;
        const playhead = document.getElementById('playhead');
        if (playhead) {
            playhead.style.left = `${pixels}px`;
        }

        animFrameId = requestAnimationFrame(animate);
    };

    animFrameId = requestAnimationFrame(animate);
}

function stopPlay() {
    isPlaying = false;
    cancelAnimationFrame(animFrameId);

    const playBtn = document.getElementById('play-btn');
    if (playBtn) playBtn.innerHTML = '<span class="icon">▶</span> Play';

    renderTimeline(); // Snap UI back to static state
    renderEditor();
}

function updatePlayheadPosition() {
    // Only used when NOT playing (static snap)
    if (isPlaying) return;

    const playhead = document.getElementById('playhead');
    if (!playhead) return;

    let timeMs = 0;
    for (let i = 0; i < currentFrameIndex; i++) {
        timeMs += frames[i].duration;
    }

    const pixels = (timeMs / 1000) * PIXELS_PER_SEC;
    playhead.style.left = `${pixels}px`;
}

// Helper for Linear Interpolation
// Helper for Linear Interpolation
function lerp(start, end, t) {
    if (start === undefined || end === undefined || isNaN(start) || isNaN(end)) {
        return start !== undefined ? start : 0;
    }
    return start + (end - start) * t;
}

function renderPreview(frameIndex, interpolation = null) {
    if (frameIndex === undefined) frameIndex = currentFrameIndex;
    const frame = frames[frameIndex];
    if (!frame) return;

    previewCanvas.width = GRID_WIDTH;
    previewCanvas.height = GRID_HEIGHT;

    const pCtx = previewCtx;

    // 1. Clear text/bg
    pCtx.fillStyle = '#000000';
    pCtx.fillRect(0, 0, previewCanvas.width, previewCanvas.height);

    // 2. Render Pixels
    const imgData = pCtx.createImageData(GRID_WIDTH, GRID_HEIGHT);
    const data = imgData.data;

    for (let i = 0; i < frame.pixels.length; i++) {
        const color = frame.pixels[i];
        if (color) {
            const r = parseInt(color.slice(1, 3), 16);
            const g = parseInt(color.slice(3, 5), 16);
            const b = parseInt(color.slice(5, 7), 16);
            const idx = i * 4;
            data[idx] = r;
            data[idx + 1] = g;
            data[idx + 2] = b;
            data[idx + 3] = 255;
        }
    }
    pCtx.putImageData(imgData, 0, 0);

    // 3. Helper to draw any shape properties
    const drawShapeProps = (ctx, props) => {
        ctx.save();
        ctx.globalAlpha = props.opacity !== undefined ? props.opacity : 1;
        ctx.globalCompositeOperation = props.blendMode || 'source-over';
        ctx.fillStyle = props.color;
        ctx.strokeStyle = props.color;
        ctx.lineWidth = 2;

        if (props.type === 'rect') {
            ctx.fillRect(props.x, props.y, props.width, props.height);
        } else if (props.type === 'ellipse') {
            ctx.beginPath();
            ctx.ellipse(
                props.x + props.width / 2,
                props.y + props.height / 2,
                Math.abs(props.width / 2),
                Math.abs(props.height / 2),
                0, 0, Math.PI * 2
            );
            ctx.fill();
        } else if (props.type === 'line' && props.lineEnd) {
            ctx.beginPath();
            ctx.moveTo(props.x, props.y);
            ctx.lineTo(props.lineEnd.x, props.lineEnd.y);
            ctx.stroke();
        }
        ctx.restore();
    };

    // 4. Render Shapes (Interpolated if needed)
    if (frame.shapes && frame.shapes.length > 0) {
        // Reset context globally first just in case
        pCtx.globalAlpha = 1.0;
        pCtx.globalCompositeOperation = 'source-over';

        frame.shapes.forEach(shape => {
            let props = shape; // Default to current shape

            // Interpolation Logic
            if (interpolation && interpolation.nextFrame && interpolation.t > 0) {
                const nextShape = interpolation.nextFrame.shapes.find(s => s.id === shape.id);
                if (nextShape && nextShape.type === shape.type) {
                    // Create interpolated properties
                    props = {
                        type: shape.type,
                        x: lerp(shape.x, nextShape.x, interpolation.t),
                        y: lerp(shape.y, nextShape.y, interpolation.t),
                        width: lerp(shape.width, nextShape.width, interpolation.t),
                        height: lerp(shape.height, nextShape.height, interpolation.t),
                        color: shape.color,
                        opacity: lerp(
                            shape.opacity !== undefined ? shape.opacity : 1,
                            nextShape.opacity !== undefined ? nextShape.opacity : 1,
                            interpolation.t
                        ),
                        blendMode: shape.blendMode,
                        lineEnd: null
                    };

                    if (shape.type === 'line' && shape.lineEnd && nextShape.lineEnd) {
                        props.lineEnd = {
                            x: lerp(shape.lineEnd.x, nextShape.lineEnd.x, interpolation.t),
                            y: lerp(shape.lineEnd.y, nextShape.lineEnd.y, interpolation.t)
                        };
                    }
                }
            }

            drawShapeProps(pCtx, props);
        });
    }
}

// ======== ZOOM CONTROLS ========
function setZoom(level) {
    zoomLevel = Math.max(MIN_ZOOM, Math.min(MAX_ZOOM, level));

    const wrapper = document.getElementById('canvas-wrapper');
    if (wrapper) {
        if (zoomLevel === 1) {
            wrapper.style.transform = 'none';
        } else {
            wrapper.style.transform = `scale(${zoomLevel})`;
            wrapper.style.transformOrigin = 'center center';
        }
    }

    const zoomLevelEl = document.getElementById('zoom-level');
    if (zoomLevelEl) {
        zoomLevelEl.innerText = Math.round(zoomLevel * 100) + '%';
    }
}

function fitZoom() {
    const viewport = document.getElementById('canvas-viewport');
    if (viewport) {
        const vw = viewport.clientWidth - 40;
        const vh = viewport.clientHeight - 40;
        const scaleX = vw / GRID_WIDTH;
        const scaleY = vh / GRID_HEIGHT;
        const fitScale = Math.min(scaleX, scaleY);
        setZoom(fitScale);
    }
}

// ======== EXPORT TO C ========
async function exportToC() {
    // Get animation name from input field
    let name = document.getElementById('anim-name')?.value || 'my_anim';
    // Sanitize name (remove special characters, spaces)
    name = name.replace(/[^a-zA-Z0-9_]/g, '_').toLowerCase();
    if (!name || name.length === 0) name = 'my_anim';

    let cContent = `/**
 * @file ${name}.c
 * @brief Auto-generated animation from Robot Face Studio
 * Compatible with LVGL (RGB565 Format)
 */

#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

`;

    const dscNames = [];

    // Helper Canvas for Baking
    const bakeCanvas = document.createElement('canvas');
    bakeCanvas.width = GRID_WIDTH;
    bakeCanvas.height = GRID_HEIGHT;
    const bakeCtx = bakeCanvas.getContext('2d');

    for (let idx = 0; idx < frames.length; idx++) {
        const frame = frames[idx];
        const frameName = `${name}_f${idx}`;
        dscNames.push(frameName);

        // 1. Render Frame to Canvas (Apply all Blending/Opacity)
        bakeCtx.fillStyle = '#000000'; // Black background
        bakeCtx.fillRect(0, 0, GRID_WIDTH, GRID_HEIGHT);

        // Draw Pixels
        const pImgData = bakeCtx.createImageData(GRID_WIDTH, GRID_HEIGHT);
        const pData = pImgData.data;
        for (let i = 0; i < frame.pixels.length; i++) {
            const color = frame.pixels[i];
            if (color) {
                const r = parseInt(color.slice(1, 3), 16);
                const g = parseInt(color.slice(3, 5), 16);
                const b = parseInt(color.slice(5, 7), 16);
                const idx = i * 4;
                pData[idx] = r;
                pData[idx + 1] = g;
                pData[idx + 2] = b;
                pData[idx + 3] = 255;
            }
        }
        bakeCtx.putImageData(pImgData, 0, 0);

        // Draw Shapes (with context restore/save built-in to draw method)
        if (frame.shapes && frame.shapes.length > 0) {
            // Ensure Clean State before shapes
            bakeCtx.globalAlpha = 1.0;
            bakeCtx.globalCompositeOperation = 'source-over';

            frame.shapes.forEach(shape => shape.draw(bakeCtx));
        }

        // 2. Extract Data
        const finalImgData = bakeCtx.getImageData(0, 0, GRID_WIDTH, GRID_HEIGHT);
        const rgba = finalImgData.data;

        // 3. Convert to C Array (RGB565)
        cContent += `const LV_ATTRIBUTE_MEM_ALIGN uint8_t ${frameName}_map[] = {\n`;
        let line = "    ";
        let byteCount = 0;

        for (let i = 0; i < rgba.length; i += 4) {
            const r = rgba[i];
            const g = rgba[i + 1];
            const b = rgba[i + 2];
            // Alpha is ignored in RGB565 (unless we use alpha byte, but standard is usually no alpha for basic bitmaps)

            // RGB565 Conversion
            const r5 = (r >> 3) & 0x1F;
            const g6 = (g >> 2) & 0x3F;
            const b5 = (b >> 3) & 0x1F;

            // Little Endian
            const rgb565 = (r5 << 11) | (g6 << 5) | b5;
            const lowByte = rgb565 & 0xFF;
            const highByte = (rgb565 >> 8) & 0xFF;

            // Output bytes (Low, High for Little Endian architecture usually, LVGL depends on config but standard is often creating simple byte array)
            // Actually LVGL usually expects Little Endian: Low byte, High byte
            line += `0x${lowByte.toString(16).padStart(2, '0')}, 0x${highByte.toString(16).padStart(2, '0')}, `;
            byteCount += 2;

            if (byteCount >= 24) {
                cContent += line + "\n";
                line = "    ";
                byteCount = 0;
            }
        }

        if (line.trim() !== "") {
            cContent += line + "\n";
        }

        cContent += `};\n\n`;

        cContent += `const lv_img_dsc_t ${frameName} = {\n`;
        cContent += `    .header.always_zero = 0,\n`;
        cContent += `    .header.w = ${GRID_WIDTH},\n`;
        cContent += `    .header.h = ${GRID_HEIGHT},\n`;
        cContent += `    .data_size = ${GRID_WIDTH * GRID_HEIGHT * 2},\n`;
        cContent += `    .header.cf = LV_IMG_CF_TRUE_COLOR,\n`;
        cContent += `    .data = ${frameName}_map,\n`;
        cContent += `};\n\n`;
    }

    // Create array of descriptors
    cContent += `const lv_img_dsc_t* ${name}_frames[] = {\n`;
    dscNames.forEach(frameName => {
        cContent += `    &${frameName},\n`;
    });
    cContent += `};\n\n`;
    cContent += `const uint8_t ${name}_frame_count = ${frames.length};\n`;

    // Generate H file
    const hContent = `/**
 * @file ${name}.h
 * @brief Header for ${name}.c
 * Auto-generated from Robot Face Studio
 */

#ifndef ${name.toUpperCase()}_H
#define ${name.toUpperCase()}_H

#include "lvgl/lvgl.h"

extern const lv_img_dsc_t* ${name}_frames[];
extern const uint8_t ${name}_frame_count;

#endif // ${name.toUpperCase()}_H
`;

    // Save both files
    showToast('Generating files...');

    await saveFile(cContent, `${name}.c`, 'text/x-c');

    // Small delay to ensure first file is saved
    await new Promise(resolve => setTimeout(resolve, 500));

    await saveFile(hContent, `${name}.h`, 'text/x-c');

    showToast(`✅ Exported ${name}.c and ${name}.h`);
}

async function saveFile(content, filename, mimeType) {
    if ('showSaveFilePicker' in window) {
        try {
            const ext = filename.split('.').pop();
            const types = {
                'c': { description: 'C Source File', accept: { 'text/x-c': ['.c'] } },
                'json': { description: 'JSON File', accept: { 'application/json': ['.json'] } }
            };

            const handle = await window.showSaveFilePicker({
                suggestedName: filename,
                types: [types[ext] || { description: 'File', accept: { 'text/plain': [`.${ext}`] } }]
            });

            const writable = await handle.createWritable();
            await writable.write(content);
            await writable.close();
            showToast(`Saved: ${filename}`);
            return;
        } catch (e) {
            if (e.name === 'AbortError') return;
            console.log('File System API failed, using fallback');
        }
    }

    const dataUrl = `data:${mimeType};charset=utf-8,` + encodeURIComponent(content);
    const a = document.createElement('a');
    a.href = dataUrl;
    a.download = filename;
    a.click();
}

function showToast(msg) {
    let container = document.getElementById('toast-container');
    if (!container) {
        container = document.createElement('div');
        container.id = 'toast-container';
        container.style.cssText = 'position: fixed; bottom: 20px; right: 20px; z-index: 9999; display: flex; flex-direction: column; gap: 10px;';
        document.body.appendChild(container);
    }

    const toast = document.createElement('div');
    toast.style.cssText = 'background: rgba(0,0,0,0.8); color: white; padding: 10px 20px; border-radius: 4px; font-family: sans-serif;';
    toast.innerText = msg;

    container.appendChild(toast);

    setTimeout(() => {
        toast.style.opacity = '0';
        toast.style.transition = 'all 0.3s ease-in';
        setTimeout(() => toast.remove(), 300);
    }, 2000);
}

// ======== MISSING UTILITY FUNCTIONS ========

// Resize Canvas
function resizeCanvas(newWidth, newHeight) {
    if (newWidth < 8 || newHeight < 8 || newWidth > 512 || newHeight > 512) {
        showToast('Canvas size must be between 8 and 512');
        return;
    }

    const oldWidth = GRID_WIDTH;
    const oldHeight = GRID_HEIGHT;

    GRID_WIDTH = newWidth;
    GRID_HEIGHT = newHeight;

    // Resize all frames
    frames.forEach(frame => {
        const oldPixels = frame.pixels;
        frame.pixels = new Array(GRID_WIDTH * GRID_HEIGHT).fill(null);

        // Copy existing pixels
        for (let y = 0; y < Math.min(oldHeight, GRID_HEIGHT); y++) {
            for (let x = 0; x < Math.min(oldWidth, GRID_WIDTH); x++) {
                const oldIdx = y * oldWidth + x;
                const newIdx = y * GRID_WIDTH + x;
                frame.pixels[newIdx] = oldPixels[oldIdx];
            }
        }
    });

    // Update canvas elements
    canvas.width = GRID_WIDTH;
    canvas.height = GRID_HEIGHT;
    previewCanvas.width = GRID_WIDTH;
    previewCanvas.height = GRID_HEIGHT;

    // Update inputs
    document.getElementById('canvas-width').value = GRID_WIDTH;
    document.getElementById('canvas-height').value = GRID_HEIGHT;

    // Re-check round toggle
    if (GRID_WIDTH === 466 && GRID_HEIGHT === 466) {
        document.getElementById('round-toggle').checked = true;
        toggleRound(true);
    } else {
        document.getElementById('round-toggle').checked = false;
        toggleRound(false);
    }

    renderTimeline();
    renderEditor();
    renderPreview();
    fitZoom();

    showToast(`Canvas resized to ${GRID_WIDTH}x${GRID_HEIGHT}`);
}

// Export to JSON
async function exportToJSON() {
    // Get animation name from input field
    let name = document.getElementById('anim-name')?.value || 'animation';
    // Sanitize name
    name = name.replace(/[^a-zA-Z0-9_-]/g, '_');
    if (!name || name.length === 0) name = 'animation';

    const data = {
        version: "1.0",
        width: GRID_WIDTH,
        height: GRID_HEIGHT,
        fps: parseInt(document.getElementById('fps-range')?.value || 12),
        frames: frames.map((frame, index) => ({
            index,
            duration: frame.duration,
            pixels: frame.pixels.map((p, i) => p ? { i, c: p } : null).filter(Boolean),
            shapes: frame.shapes.map(s => ({
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
            }))
        }))
    };

    const jsonContent = JSON.stringify(data, null, 2);
    await saveFile(jsonContent, `${name}.json`, 'application/json');
}

// Import from JSON
async function importFromJSON() {
    const input = document.createElement('input');
    input.type = 'file';
    input.accept = '.json';

    input.onchange = async (e) => {
        const file = e.target.files[0];
        if (!file) return;

        try {
            const text = await file.text();
            const data = JSON.parse(text);

            if (!data.frames || !Array.isArray(data.frames)) {
                showToast('Invalid animation file');
                return;
            }

            // Apply dimensions
            if (data.width && data.height) {
                resizeCanvas(data.width, data.height);
            }

            // Load frames
            frames = [];
            data.frames.forEach(frameData => {
                const frame = new Frame();
                frame.duration = frameData.duration || 100;

                // Load pixels
                if (frameData.pixels) {
                    frameData.pixels.forEach(p => {
                        if (p && p.i !== undefined && p.c) {
                            frame.pixels[p.i] = p.c;
                        }
                    });
                }

                // Load shapes
                if (frameData.shapes) {
                    frame.shapes = frameData.shapes.map(s => {
                        const shape = new Shape(s.type, s.x, s.y, s.width, s.height, s.color);
                        shape.opacity = s.opacity !== undefined ? s.opacity : 1;
                        shape.blendMode = s.blendMode || 'source-over';
                        shape.rotation = s.rotation || 0;
                        shape.id = s.id || (Date.now() + Math.random());
                        if (s.lineEnd) shape.lineEnd = { ...s.lineEnd };
                        return shape;
                    });
                }

                frames.push(frame);
            });

            if (frames.length === 0) {
                frames = [new Frame()];
            }

            currentFrameIndex = 0;
            renderTimeline();
            renderEditor();
            renderPreview();
            updateFrameInfo();

            showToast(`Loaded ${frames.length} frames`);
        } catch (err) {
            console.error(err);
            showToast('Failed to load file');
        }
    };

    input.click();
}

// Update Frame Info Panel
function updateFrameInfo() {
    const frame = frames[currentFrameIndex];
    if (!frame) return;

    // Update frame index
    const indexEl = document.getElementById('frame-index');
    if (indexEl) indexEl.textContent = currentFrameIndex;

    // Update frame duration input
    const durationInput = document.getElementById('frame-duration');
    if (durationInput) durationInput.value = frame.duration;

    // Calculate total time
    const totalMs = frames.reduce((sum, f) => sum + (f.duration || 100), 0);
    const totalTimeEl = document.getElementById('total-time');
    if (totalTimeEl) totalTimeEl.textContent = (totalMs / 1000).toFixed(2) + 's';

    // Update playhead time if not playing
    if (!isPlaying) {
        let timeMs = 0;
        for (let i = 0; i < currentFrameIndex; i++) {
            timeMs += frames[i].duration || 100;
        }
        const playheadTimeEl = document.getElementById('playhead-time');
        if (playheadTimeEl) {
            const mins = Math.floor(timeMs / 60000);
            const secs = Math.floor((timeMs % 60000) / 1000);
            const ms = timeMs % 1000;
            playheadTimeEl.textContent = `${mins.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}.${ms.toString().padStart(3, '0')}`;
        }
    }
}

// Onion Skinning
let onionSkinEnabled = false;
let onionSkinOpacity = 0.3;

function toggleOnionSkin(enabled) {
    onionSkinEnabled = enabled;
    renderEditor();
}

// Enhanced renderEditor with Onion Skinning
const originalRenderEditor = renderEditor;

// Override renderEditor to add onion skinning
function renderEditorWithOnion() {
    ctx.fillStyle = '#000000';
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    const frame = frames[currentFrameIndex];
    if (!frame) return;

    // Draw onion skin (previous frame)
    if (onionSkinEnabled && currentFrameIndex > 0) {
        const prevFrame = frames[currentFrameIndex - 1];
        if (prevFrame) {
            ctx.globalAlpha = onionSkinOpacity;

            // Draw previous frame pixels
            const imgData = ctx.createImageData(canvas.width, canvas.height);
            const data = imgData.data;
            for (let i = 0; i < prevFrame.pixels.length; i++) {
                const color = prevFrame.pixels[i];
                if (color) {
                    const r = parseInt(color.slice(1, 3), 16);
                    const g = parseInt(color.slice(3, 5), 16);
                    const b = parseInt(color.slice(5, 7), 16);
                    const idx = i * 4;
                    // Tint red for previous frame
                    data[idx] = Math.min(255, r + 100);
                    data[idx + 1] = g * 0.5;
                    data[idx + 2] = b * 0.5;
                    data[idx + 3] = 128;
                }
            }
            ctx.putImageData(imgData, 0, 0);

            // Draw shapes
            if (prevFrame.shapes) {
                prevFrame.shapes.forEach(shape => {
                    ctx.globalAlpha = onionSkinOpacity * 0.5;
                    shape.draw(ctx);
                });
            }

            ctx.globalAlpha = 1.0;
        }
    }

    // Draw current frame pixels
    const imgData = ctx.createImageData(canvas.width, canvas.height);
    const data = imgData.data;

    for (let i = 0; i < frame.pixels.length; i++) {
        const color = frame.pixels[i];
        if (color) {
            const r = parseInt(color.slice(1, 3), 16);
            const g = parseInt(color.slice(3, 5), 16);
            const b = parseInt(color.slice(5, 7), 16);
            const idx = i * 4;
            data[idx] = r;
            data[idx + 1] = g;
            data[idx + 2] = b;
            data[idx + 3] = 255;
        }
    }
    ctx.putImageData(imgData, 0, 0);

    // Draw shapes
    if (frame.shapes && frame.shapes.length > 0) {
        frame.shapes.forEach(shape => shape.draw(ctx));
    }

    // Draw selection box if shape is selected
    if (selectedShape) {
        const bounds = selectedShape.getBounds();
        const padding = 4;

        ctx.strokeStyle = '#00d2ff';
        ctx.lineWidth = 2;
        ctx.setLineDash([5, 3]);
        ctx.strokeRect(
            bounds.x - padding,
            bounds.y - padding,
            bounds.width + padding * 2,
            bounds.height + padding * 2
        );
        ctx.setLineDash([]);

        // Draw resize handles (corners)
        const handleSize = 8;
        ctx.fillStyle = '#00d2ff';

        const corners = [
            { x: bounds.x - padding, y: bounds.y - padding },
            { x: bounds.x + bounds.width + padding - handleSize, y: bounds.y - padding },
            { x: bounds.x - padding, y: bounds.y + bounds.height + padding - handleSize },
            { x: bounds.x + bounds.width + padding - handleSize, y: bounds.y + bounds.height + padding - handleSize }
        ];

        corners.forEach(corner => {
            ctx.fillRect(corner.x, corner.y, handleSize, handleSize);
        });
    }

    // Update preview canvas
    renderPreview();

    // Update frame info
    updateFrameInfo();
}

// Start
init();

// ======== ROBUST PREVIEW RENDERER ========
// Overriding any previous broken implementation
function renderPreview(frameIndex, interpolation = null) {
    try {
        if (frameIndex === undefined) frameIndex = currentFrameIndex;
        const frame = frames[frameIndex];
        if (!frame) return;

        const pCanvas = document.getElementById('preview-canvas');
        if (!pCanvas) return;
        const pCtx = pCanvas.getContext('2d');
        if (!pCtx) return;

        pCanvas.width = GRID_WIDTH;
        pCanvas.height = GRID_HEIGHT;

        // 1. Clear text/bg
        pCtx.fillStyle = '#000000';
        pCtx.fillRect(0, 0, pCanvas.width, pCanvas.height);

        // 2. Render Pixels
        const imgData = pCtx.createImageData(GRID_WIDTH, GRID_HEIGHT);
        const data = imgData.data;

        if (frame.pixels) {
            for (let i = 0; i < frame.pixels.length; i++) {
                const color = frame.pixels[i];
                if (color) {
                    const r = parseInt(color.slice(1, 3), 16);
                    const g = parseInt(color.slice(3, 5), 16);
                    const b = parseInt(color.slice(5, 7), 16);
                    const idx = i * 4;
                    data[idx] = r;
                    data[idx + 1] = g;
                    data[idx + 2] = b;
                    data[idx + 3] = 255;
                }
            }
        }
        pCtx.putImageData(imgData, 0, 0);

        // 3. Helper to draw any shape properties
        const drawShapeProps = (ctx, props) => {
            if (!props) return;
            ctx.save();
            ctx.globalAlpha = (props.opacity !== undefined && !isNaN(props.opacity)) ? props.opacity : 1;
            ctx.globalCompositeOperation = props.blendMode || 'source-over';
            ctx.fillStyle = props.color || '#FFFFFF';
            ctx.strokeStyle = props.color || '#FFFFFF';
            ctx.lineWidth = 2;

            const x = props.x || 0;
            const y = props.y || 0;
            const w = props.width || 0;
            const h = props.height || 0;

            if (props.type === 'rect') {
                ctx.fillRect(x, y, w, h);
            } else if (props.type === 'ellipse') {
                ctx.beginPath();
                ctx.ellipse(
                    x + w / 2,
                    y + h / 2,
                    Math.abs(w / 2),
                    Math.abs(h / 2),
                    0, 0, Math.PI * 2
                );
                ctx.fill();
            } else if (props.type === 'line' && props.lineEnd) {
                ctx.beginPath();
                ctx.moveTo(x, y);
                ctx.lineTo(props.lineEnd.x || 0, props.lineEnd.y || 0);
                ctx.stroke();
            }
            ctx.restore();
        };

        // 4. Render Shapes (Interpolated if needed)
        if (frame.shapes && frame.shapes.length > 0) {
            pCtx.globalAlpha = 1.0;
            pCtx.globalCompositeOperation = 'source-over';

            frame.shapes.forEach(shape => {
                let props = shape;

                // Interpolation Logic
                if (interpolation && interpolation.nextFrame && interpolation.t > 0) {
                    const nextShape = interpolation.nextFrame.shapes ? interpolation.nextFrame.shapes.find(s => s.id === shape.id) : null;
                    if (nextShape && nextShape.type === shape.type) {
                        props = {
                            type: shape.type,
                            x: lerp(shape.x, nextShape.x, interpolation.t),
                            y: lerp(shape.y, nextShape.y, interpolation.t),
                            width: lerp(shape.width, nextShape.width, interpolation.t),
                            height: lerp(shape.height, nextShape.height, interpolation.t),
                            color: shape.color,
                            opacity: lerp(
                                shape.opacity !== undefined ? shape.opacity : 1,
                                nextShape.opacity !== undefined ? nextShape.opacity : 1,
                                interpolation.t
                            ),
                            blendMode: shape.blendMode,
                            lineEnd: null
                        };

                        // Fix line end interpolation
                        if (shape.type === 'line' && shape.lineEnd && nextShape.lineEnd) {
                            props.lineEnd = {
                                x: lerp(shape.lineEnd.x, nextShape.lineEnd.x, interpolation.t),
                                y: lerp(shape.lineEnd.y, nextShape.lineEnd.y, interpolation.t)
                            };
                        }
                    }
                }
                drawShapeProps(pCtx, props);
            });
        }
    } catch (err) {
        console.error("RenderPreview Error:", err);
    }
}


