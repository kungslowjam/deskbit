// Constants
let GRID_WIDTH = 466;
let GRID_HEIGHT = 466;

// State
let projectStates = [];
let activeStateId = null;
let frames = [];
let currentFrameIndex = 0;
let isDrawing = false;
let isPlaying = false;
let currentTool = 'pen';
let currentColor = '#00ffff';
let brushSize = 1;
let isSymmetryEnabled = false;
let lastCoords = null; // Track latest mouse coords for sync drawing
let copiedFrameData = null;


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

function lerp(start, end, t) {
    if (start === undefined || end === undefined) return start ?? end;
    return start + (end - start) * t;
}

function lerpColor(c1, c2, t) {
    if (!c1 || !c2) return c1 || c2;
    try {
        const r1 = parseInt(c1.substring(1, 3), 16);
        const g1 = parseInt(c1.substring(3, 5), 16);
        const b1 = parseInt(c1.substring(5, 7), 16);
        const r2 = parseInt(c2.substring(1, 3), 16);
        const g2 = parseInt(c2.substring(3, 5), 16);
        const b2 = parseInt(c2.substring(5, 7), 16);
        const r = Math.round(r1 + (r2 - r1) * t);
        const g = Math.round(g1 + (g2 - g1) * t);
        const b = Math.round(b1 + (b2 - b1) * t);
        return `#${r.toString(16).padStart(2, '0')}${g.toString(16).padStart(2, '0')}${b.toString(16).padStart(2, '0')}`;
    } catch (e) { return c1; }
}
function updateToolButtons() {
    document.querySelectorAll('.tool-btn').forEach(btn => {
        btn.classList.remove('bg-white/20', 'text-accent');
        if (btn.id === `tool-${currentTool}`) {
            btn.classList.add('bg-white/20', 'text-accent');
        } else if (btn.id === `tool-${currentTool.replace('-obj', '')}-obj`) {
            // Match for both simple and -obj tools
            btn.classList.add('bg-white/20', 'text-accent');
        }
    });
}

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
        this.interaction = null; // { trigger: 'click', targetStateId: 'happy' }

        // Style Properties (Advanced)
        this.strokeWidth = parseInt(document.getElementById('stroke-width-slider')?.value || 0);
        this.strokeColor = document.getElementById('stroke-color-picker')?.value || '#ffffff';
        this.cornerRadius = parseInt(document.getElementById('corner-radius-slider')?.value || 0);

        // Path Property (New)
        this.pathData = null; // SVG path string
        this.nodes = [];      // Array of {x, y, type} for editing
        this.isEditingPositions = false;
    }

    // Convert SVG path to nodes (simplified)
    parsePath() {
        if (!this.pathData) return;
        this.nodes = [];
        const commands = this.pathData.match(/[a-zA-Z][^a-zA-Z]*/g);
        if (!commands) return;

        commands.forEach(cmd => {
            const type = cmd[0];
            const args = cmd.slice(1).trim().split(/[\s,]+/).map(parseFloat);
            if (type === 'M' || type === 'L') {
                this.nodes.push({ type, x: args[0], y: args[1] });
            } else if (type === 'Q') {
                this.nodes.push({ type, cx: args[0], cy: args[1], x: args[2], y: args[3] });
            } else if (type === 'C') {
                this.nodes.push({ type, x1: args[0], y1: args[1], x2: args[2], y2: args[3], x: args[4], y: args[5] });
            } else if (type === 'Z' || type === 'z') {
                this.nodes.push({ type: 'Z' });
            }
        });
    }

    // Convert nodes back to pathData
    updatePathData() {
        this.pathData = this.nodes.map(n => {
            if (n.type === 'M' || n.type === 'L') return `${n.type}${n.x},${n.y}`;
            if (n.type === 'Q') return `Q${n.cx},${n.cy} ${n.x},${n.y}`;
            if (n.type === 'C') return `C${n.x1},${n.y1} ${n.x2},${n.y2} ${n.x},${n.y}`;
            if (n.type === 'Z') return 'Z';
            return '';
        }).join(' ');
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
        } else if (this.type === 'triangle') {
            // Point in Triangle check (Barycentric)
            const x1 = this.x + this.width / 2, y1 = this.y;
            const x2 = this.x, y2 = this.y + this.height;
            const x3 = this.x + this.width, y3 = this.y + this.height;
            const area = 0.5 * (-y2 * x3 + y1 * (-x2 + x3) + x1 * (y2 - y3) + x2 * y3);
            const s = 1 / (2 * area) * (y1 * x3 - x1 * y3 + (y3 - y1) * px + (x1 - x3) * py);
            const t = 1 / (2 * area) * (x1 * y2 - y1 * x2 + (y1 - y2) * px + (x2 - x1) * py);
            return s > 0 && t > 0 && (1 - s - t) > 0;
        } else if (this.type === 'path' && this.pathData) {
            // Rough check (bounding box)
            return px >= this.x && px <= this.x + this.width &&
                py >= this.y && py <= this.y + this.height;
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
        if (this.interaction) {
            newShape.interaction = { ...this.interaction };
        }

        // Copy Styles
        newShape.strokeWidth = this.strokeWidth;
        newShape.strokeColor = this.strokeColor;
        newShape.cornerRadius = this.cornerRadius;
        newShape.pathData = this.pathData;
        newShape.isMirrored = this.isMirrored;

        return newShape;
    }

    draw(context) {
        context.save();
        context.globalAlpha = this.opacity;
        context.globalCompositeOperation = this.blendMode;

        context.fillStyle = this.color;
        context.strokeStyle = this.strokeColor;
        context.lineWidth = this.strokeWidth;

        // Apply rotation around the center of the shape
        if (this.rotation !== 0) {
            context.translate(this.x + this.width / 2, this.y + this.height / 2);
            context.rotate((this.rotation * Math.PI) / 180);
            context.translate(-(this.x + this.width / 2), -(this.y + this.height / 2));
        }

        if (this.type === 'rect') {
            if (this.cornerRadius > 0) {
                this.drawRoundedRect(context, this.x, this.y, this.width, this.height, this.cornerRadius);
            } else {
                context.fillRect(this.x, this.y, this.width, this.height);
                if (this.strokeWidth > 0) context.strokeRect(this.x, this.y, this.width, this.height);
            }
        } else if (this.type === 'ellipse') {
            context.beginPath();
            context.ellipse(this.x + this.width / 2, this.y + this.height / 2,
                Math.abs(this.width / 2), Math.abs(this.height / 2), 0, 0, Math.PI * 2);
            context.fill();
            if (this.strokeWidth > 0) context.stroke();
        } else if (this.type === 'triangle') {
            context.beginPath();
            context.moveTo(this.x + this.width / 2, this.y);
            context.lineTo(this.x, this.y + this.height);
            context.lineTo(this.x + this.width, this.y + this.height);
            context.closePath();
            context.fill();
            if (this.strokeWidth > 0) context.stroke();
        } else if (this.type === 'path' && this.pathData) {
            context.save();
            const p = new Path2D(this.pathData);

            // Positioning and Mirroring
            if (this.isMirrored) {
                context.translate(this.x + this.width, this.y);
                context.scale(-1, 1);
            } else {
                context.translate(this.x, this.y);
            }

            const scaleX = this.width / 100; // Base library dimensions 100x60
            const scaleY = this.height / 60;
            context.scale(scaleX, scaleY);

            context.fill(p);
            if (this.strokeWidth > 0) context.stroke(p);
            context.restore();
        } else if (this.type === 'line' && this.lineEnd) {
            context.strokeStyle = this.color; // Line uses main color as stroke
            context.lineWidth = Math.max(2, this.strokeWidth); // Minimum 2px for lines
            context.beginPath();
            context.moveTo(this.x, this.y);
            context.lineTo(this.lineEnd.x, this.lineEnd.y);
            context.stroke();
        }
        context.restore();
    }

    drawRoundedRect(ctx, x, y, width, height, radius) {
        radius = Math.min(radius, Math.abs(width) / 2, Math.abs(height) / 2);
        ctx.beginPath();
        ctx.moveTo(x + radius, y);
        ctx.lineTo(x + width - radius, y);
        ctx.quadraticCurveTo(x + width, y, x + width, y + radius);
        ctx.lineTo(x + width, y + height - radius);
        ctx.quadraticCurveTo(x + width, y + height, x + width - radius, y + height);
        ctx.lineTo(x + radius, y + height);
        ctx.quadraticCurveTo(x, y + height, x, y + height - radius);
        ctx.lineTo(x, y + radius);
        ctx.quadraticCurveTo(x, y, x + radius, y);
        ctx.closePath();
        ctx.fill();
        if (this.strokeWidth > 0) ctx.stroke();
    }
}

// Frame Class
class Frame {
    constructor() {
        this.pixels = new Array(GRID_WIDTH * GRID_HEIGHT).fill(null);
        this.shapes = [];
        this.duration = 100;
        this.id = Date.now() + Math.random();
        this.cacheCanvas = document.createElement('canvas');
        this.cacheCanvas.width = GRID_WIDTH;
        this.cacheCanvas.height = GRID_HEIGHT;
        this.cacheCtx = this.cacheCanvas.getContext('2d');
        this.isCacheDirty = true;
    }

    updateCache() {
        const ctx = this.cacheCtx;
        ctx.clearRect(0, 0, GRID_WIDTH, GRID_HEIGHT);

        const imgData = ctx.createImageData(GRID_WIDTH, GRID_HEIGHT);
        const data = imgData.data;

        for (let i = 0; i < this.pixels.length; i++) {
            const color = this.pixels[i];
            if (color) {
                // Inline hex to rgb for speed
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
        this.isCacheDirty = false;
    }
}

// Global setColor function for palette swatches
function setColor(color) {
    currentColor = color;
    const picker = document.getElementById('color-picker');
    if (picker) picker.value = color;

    // Update preset highlighting
    document.querySelectorAll('.color-preset').forEach(swatch => {
        swatch.classList.remove('active');
        // Match both exact and computed background color
        const swatchColor = rgbToHex(swatch.style.backgroundColor);
        if (swatchColor === color.toLowerCase()) {
            swatch.classList.add('active');
        }
    });

    // Update secondary active indicator (if text or shape is selected)
    if (selectedShape) {
        selectedShape.color = color;
        renderEditor();
        renderPreview();
    }
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
            updateToolButtons();
        });
    });

    // Timeline Controls
    const addBtn = document.getElementById('add-frame-btn');
    const dupBtn = document.getElementById('dup-frame-btn');
    const delBtn = document.getElementById('del-frame-btn');
    const copyBtn = document.getElementById('copy-frame-btn');
    const pasteBtn = document.getElementById('paste-frame-btn');
    const reverseBtn = document.getElementById('reverse-frames-btn');

    if (addBtn) addBtn.onclick = addFrame;
    if (dupBtn) dupBtn.onclick = duplicateFrame;
    if (delBtn) delBtn.onclick = deleteFrame;
    if (copyBtn) copyBtn.onclick = copyFrame;
    if (pasteBtn) pasteBtn.onclick = pasteFrame;
    if (reverseBtn) reverseBtn.onclick = reverseFrames;

    // Easing Mode Changes
    const easingModeSelect = document.getElementById('easing-mode');
    if (easingModeSelect) {
        easingModeSelect.addEventListener('change', (e) => {
            const container = document.getElementById('curve-preview-container');
            if (e.target.value === 'custom') {
                container?.classList.remove('hidden');
            } else {
                container?.classList.add('hidden');
            }
            drawCurvePreview();
        });
    }

    // Bezier Sliders
    ['bezier-p1x', 'bezier-p1y', 'bezier-p2x', 'bezier-p2y'].forEach(id => {
        document.getElementById(id)?.addEventListener('input', drawCurvePreview);
    });

    // Playback Controls
    const playBtn = document.getElementById('play-btn');
    const stopBtn = document.getElementById('stop-btn');
    const exportCBtn = document.getElementById('export-c-btn');
    const importJsonBtn = document.getElementById('import-btn');
    const exportJsonBtn = document.getElementById('export-btn');

    if (playBtn) playBtn.onclick = startPlay;
    if (stopBtn) stopBtn.onclick = stopPlay;
    if (exportCBtn) exportCBtn.onclick = exportToC;
    if (importJsonBtn) importJsonBtn.onclick = importFromJSON;
    if (exportJsonBtn) exportJsonBtn.onclick = exportToJSON;
    const exportNativeBtn = document.getElementById('export-lvgl-native-btn');
    if (exportNativeBtn) exportNativeBtn.onclick = exportToLVGLNative;

    // Color Picker - FIXED!
    const colorPicker = document.getElementById('color-picker');
    if (colorPicker) {
        colorPicker.addEventListener('input', (e) => {
            setColor(e.target.value);
        });
    }

    // Brush Size
    const brushSizeSlider = document.getElementById('brush-size-slider');
    const brushSizeValue = document.getElementById('brush-size-val');
    if (brushSizeSlider) {
        brushSizeSlider.addEventListener('input', (e) => {
            brushSize = parseInt(e.target.value);
            if (brushSizeValue) brushSizeValue.textContent = brushSize + 'px';
        });
    }

    // Symmetry Toggle
    const symmetryToggle = document.getElementById('symmetry-toggle');
    if (symmetryToggle) {
        symmetryToggle.addEventListener('change', (e) => {
            isSymmetryEnabled = e.target.checked;
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
        frameDurationInput.addEventListener('input', (e) => {
            const frame = frames[currentFrameIndex];
            if (frame) {
                frame.duration = parseInt(e.target.value) || 100;
                renderTimeline();
                updateFrameInfo();
            }
        });
        // Also handle change for final validation
        frameDurationInput.addEventListener('change', () => updateFrameInfo());
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

    // Send to Robot
    document.getElementById('send-btn')?.addEventListener('click', sendToRobot);

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
            renderTimeline();
            renderEditor();
            updateFrameInfo();
        };

        // Resize Handle
        const handle = document.createElement('div');
        handle.className = 'resize-handle';
        handle.dataset.index = index;

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
    if (frames.length > 0) {
        duplicateFrame(); // Standard animator behavior: add means copy current
        return;
    }
    frames.push(new Frame());
    currentFrameIndex = frames.length - 1;
    selectedFrames = [currentFrameIndex];
    renderTimeline();
    renderEditor();
    updateFrameInfo();
}

function duplicateFrame() {
    if (frames.length === 0) {
        addFrame();
        return;
    }

    const srcFrame = frames[currentFrameIndex];
    if (!srcFrame) return;

    const newFrame = new Frame();
    restoreFrame(newFrame, serializeFrame(srcFrame));

    frames.splice(currentFrameIndex + 1, 0, newFrame);
    currentFrameIndex++;
    selectedFrames = [currentFrameIndex];

    renderTimeline();
    renderEditor();
    updateFrameInfo();
}

function copyFrame() {
    if (frames[currentFrameIndex]) {
        copiedFrameData = serializeFrame(frames[currentFrameIndex]);
        // Visual feedback
        const btn = document.getElementById('copy-frame-btn');
        if (btn) {
            btn.classList.add('text-accent');
            setTimeout(() => btn.classList.remove('text-accent'), 500);
        }
    }
}

function pasteFrame() {
    if (copiedFrameData) {
        saveUndo();
        const newFrame = new Frame();
        restoreFrame(newFrame, copiedFrameData);
        frames.splice(currentFrameIndex + 1, 0, newFrame);
        currentFrameIndex++;
        renderTimeline();
        renderEditor();
        updateFrameInfo();
    }
}

function reverseFrames() {
    if (frames.length > 1) {
        saveUndo();
        frames.reverse();
        currentFrameIndex = 0;
        renderTimeline();
        renderEditor();
        updateFrameInfo();
    }
}

function deleteFrame() {
    if (frames.length <= 1) {
        frames[0].pixels.fill(null);
        frames[0].shapes = [];
        renderEditor();
        renderTimeline();
        updateFrameInfo();
        return;
    }
    frames.splice(currentFrameIndex, 1);
    if (currentFrameIndex >= frames.length) {
        currentFrameIndex = frames.length - 1;
    }
    renderTimeline();
    renderEditor();
    updateFrameInfo();
}

// Drawing
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

// History State Management
function serializeFrame(frame) {
    return JSON.stringify({
        pixels: frame.pixels,
        shapes: frame.shapes,
        duration: frame.duration
    });
}

function restoreFrame(frame, jsonState) {
    const data = JSON.parse(jsonState);
    frame.pixels = data.pixels;
    frame.duration = data.duration;

    // Re-instantiate Shapes to keep methods prototype
    frame.shapes = data.shapes.map(s => {
        let shape;
        if (s.type === 'text') {
            shape = new TextShape(s.x, s.y, s.text, s.fontSize, s.color);
        } else {
            shape = new Shape(s.type, s.x, s.y, s.width, s.height, s.color);
        }

        // Restore Essential Props
        shape.id = s.id || (Date.now() + Math.random());
        shape.rotation = s.rotation || 0;
        shape.opacity = s.opacity !== undefined ? s.opacity : 1;
        shape.blendMode = s.blendMode || 'source-over';

        // Restore Advanced Props
        if (s.lineEnd) shape.lineEnd = { ...s.lineEnd };
        if (s.interaction) shape.interaction = { ...s.interaction };

        // Restore Styles & Path Data
        shape.strokeWidth = s.strokeWidth || 0;
        shape.strokeColor = s.strokeColor || '#ffffff';
        shape.cornerRadius = s.cornerRadius || 0;
        shape.pathData = s.pathData || null;
        shape.isMirrored = s.isMirrored || false;

        return shape;
    });
    frame.isCacheDirty = true;
}

function saveUndo() {
    const frame = frames[currentFrameIndex];
    if (!frame) return;

    // Clear redo stack on new action
    redoStack = [];

    // Push current state state
    undoStack.push(serializeFrame(frame));

    // Limit stack size
    if (undoStack.length > 50) undoStack.shift();

    updateLayerUI(); // Sync UI
}

function undo() {
    if (undoStack.length === 0) return;

    const frame = frames[currentFrameIndex];
    if (!frame) return;

    // Save current state to Redo Stack before reverting
    redoStack.push(serializeFrame(frame));

    // Pop and Restore
    const previousState = undoStack.pop();
    restoreFrame(frame, previousState);

    renderEditor();
    renderTimeline(); // Update duration display if changed
    updateLayerUI();
}

function updateLayerUI() {
    // Wrapper to safely call updateLayersPanel if it exists
    if (typeof updateLayersPanel === 'function') {
        updateLayersPanel();
    }
}

function paint(x, y) {
    const frame = frames[currentFrameIndex];
    if (!frame) return;

    if (currentTool === 'pen' || currentTool === 'brush') {
        drawBrush(x, y, currentColor, brushSize);
        if (isSymmetryEnabled) {
            drawBrush(GRID_WIDTH - x, y, currentColor, brushSize);
        }
    } else if (currentTool === 'eraser') {
        drawBrush(x, y, null, brushSize);
        if (isSymmetryEnabled) {
            drawBrush(GRID_WIDTH - x, y, null, brushSize);
        }
    } else if (currentTool === 'fill') {
        floodFill(x, y, currentColor);
        if (isSymmetryEnabled) {
            floodFill(GRID_WIDTH - x, y, currentColor);
        }
    }
    renderEditor(); // This will also call renderPreview
}

function drawBrush(cx, cy, color, size) {
    const frame = frames[currentFrameIndex];
    if (!frame) return;

    const half = Math.floor(size / 2);
    const radiusSq = (size / 2) ** 2;

    for (let dy = -half; dy <= half; dy++) {
        for (let dx = -half; dx <= half; dx++) {
            // Circular brush logic
            if (size > 2 && (dx * dx + dy * dy) > radiusSq) continue;

            const x = Math.round(cx + dx);
            const y = Math.round(cy + dy);
            const idx = getPixelIndex(x, y);
            if (idx !== -1) {
                frame.pixels[idx] = color;
                drawPixel(x, y, color);
            }
        }
    }
}

function drawPixel(x, y, color) {
    if (!color) {
        ctx.clearRect(x, y, 1, 1);
        return;
    }
    ctx.fillStyle = color;
    ctx.fillRect(x, y, 1, 1);

    // Also mark cache as dirty but we don't full updateCache here for performance during drag
    // Instead we will update before renderEditor is finished if needed, 
    // but paint calls renderEditor which currently redraws everything.
    // To make it super fast, we should just let paint draw on screen and update the buffer.
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

    // 1. Draw onion skin (previous frame)
    if (typeof onionSkinEnabled !== 'undefined' && onionSkinEnabled && currentFrameIndex > 0) {
        const prevFrame = frames[currentFrameIndex - 1];
        if (prevFrame) {
            ctx.globalAlpha = onionSkinOpacity || 0.3;
            if (prevFrame.isCacheDirty) prevFrame.updateCache();
            ctx.drawImage(prevFrame.cacheCanvas, 0, 0);
            if (prevFrame.shapes) {
                prevFrame.shapes.forEach(shape => {
                    ctx.globalAlpha = (onionSkinOpacity || 0.3) * 0.5;
                    shape.draw(ctx);
                });
            }
            ctx.globalAlpha = 1.0;
        }
    }

    // 2. Draw current frame pixels
    if (frame.isCacheDirty) frame.updateCache();
    ctx.drawImage(frame.cacheCanvas, 0, 0);

    // 3. Draw shapes
    if (frame.shapes && frame.shapes.length > 0) {
        frame.shapes.forEach(shape => {
            shape.draw(ctx);

            // Draw Vector Nodes if editing
            if (shape === selectedShape && shape.type === 'path' && shape.isEditingPositions) {
                ctx.save();
                shape.nodes.forEach((node, i) => {
                    if (node.type === 'Z') return;
                    let nx, ny;
                    if (shape.isMirrored) {
                        nx = (shape.x + shape.width) - (node.x * (shape.width / 100));
                    } else {
                        nx = shape.x + (node.x * (shape.width / 100));
                    }
                    ny = shape.y + (node.y * (shape.height / 60));
                    ctx.fillStyle = '#ffffff';
                    ctx.beginPath();
                    ctx.arc(nx, ny, 6, 0, Math.PI * 2);
                    ctx.fill();
                    ctx.fillStyle = '#00ffff';
                    ctx.beginPath();
                    ctx.arc(nx, ny, 4, 0, Math.PI * 2);
                    ctx.fill();
                });
                ctx.restore();
            }
        });
    }

    // 4. Draw selection box
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

        // Resize handles
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

    // 5. Update interaction & preview
    if (typeof updateInteractionEditor === 'function') updateInteractionEditor();
    renderPreview();
    updateFrameInfo();
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
    const handleSize = 12; // Increased hit area
    const half = handleSize / 2;

    const handles = {
        'tl': { x: bounds.x, y: bounds.y },
        'tr': { x: bounds.x + bounds.width, y: bounds.y },
        'bl': { x: bounds.x, y: bounds.y + bounds.height },
        'br': { x: bounds.x + bounds.width, y: bounds.y + bounds.height }
    };

    for (const [key, h] of Object.entries(handles)) {
        if (px >= h.x - half && px <= h.x + half && py >= h.y - half && py <= h.y + half) {
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
    let isEditingNode = false;
    let activeNodeIndex = -1;

    canvas.ondblclick = (e) => {
        const coords = getCoordsFromEvent(e);
        const frame = frames[currentFrameIndex];
        if (!frame) return;

        for (let i = frame.shapes.length - 1; i >= 0; i--) {
            const shape = frame.shapes[i];
            if (shape.containsPoint(coords.x, coords.y) && shape.type === 'path') {
                selectedShape = shape;
                shape.isEditingPositions = !shape.isEditingPositions;
                if (shape.isEditingPositions) shape.parsePath();
                renderEditor();
                showToast(shape.isEditingPositions ? "Vector Edit Mode: Drag points" : "Selection Mode");
                return;
            }
        }
    };

    canvas.onmousedown = (e) => {
        if (!frames[currentFrameIndex]) return;

        const coords = getCoordsFromEvent(e);
        tempStartX = coords.x;
        tempStartY = coords.y;

        // Select Tool
        if (currentTool === 'select') {
            const frame = frames[currentFrameIndex];

            if (selectedShape && selectedShape.isEditingPositions) {
                // Check Node Hits
                const nodeSize = 6;
                const hitIdx = selectedShape.nodes.findIndex(n => {
                    if (n.type === 'Z') return false;
                    const nx = selectedShape.x + (n.x * (selectedShape.width / 100));
                    const ny = selectedShape.y + (n.y * (selectedShape.height / 60));
                    return Math.abs(coords.x - nx) < nodeSize && Math.abs(coords.y - ny) < nodeSize;
                });

                if (hitIdx > -1) {
                    isEditingNode = true;
                    activeNodeIndex = hitIdx;
                    saveUndo();
                    return;
                }
            }

            // 1. Check Resize Handles first...
            if (selectedShape) {
                const handle = getResizeHandle(coords.x, coords.y, selectedShape);
                if (handle) {
                    isResizingShape = true;
                    activeHandle = handle;
                    tempStartX = coords.x;
                    tempStartY = coords.y;
                    saveUndo();
                    return;
                }
            }

            // 2. Check Shape Hit (Top to Bottom)
            let clickedShape = null;
            for (let i = frame.shapes.length - 1; i >= 0; i--) {
                const shape = frame.shapes[i];
                if (shape.containsPoint(coords.x, coords.y)) {
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
                updateLayersPanel();
            } else {
                selectedShape = null;
                renderEditor();
                updateLayersPanel();
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
        lastCoords = coords; // Save for preview sync


        // Resize Shape
        if (isResizingShape && selectedShape) {
            const dx = coords.x - tempStartX;
            const dy = coords.y - tempStartY;

            if (selectedShape.type === 'line' && selectedShape.lineEnd) {
                // Direct endpoint manipulation for lines
                if (activeHandle === 'tl') {
                    selectedShape.x = coords.x;
                    selectedShape.y = coords.y;
                } else if (activeHandle === 'br') {
                    selectedShape.lineEnd.x = coords.x;
                    selectedShape.lineEnd.y = coords.y;
                }
                // For a line, tr and bl could move start/end too or behave as bounding box
                // Let's treat tl as start, br as end for now.
            } else {
                // Bounding box resize
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
            }

            renderEditor();
            return;
        }

        // Vector Node Drag
        if (isEditingNode && selectedShape && activeNodeIndex > -1) {
            const node = selectedShape.nodes[activeNodeIndex];
            // Update node relative to 100x60 base
            node.x = (coords.x - selectedShape.x) / (selectedShape.width / 100);
            node.y = (coords.y - selectedShape.y) / (selectedShape.height / 60);
            selectedShape.updatePathData();
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
            } else if (currentTool === 'triangle-obj') {
                ctx.beginPath();
                ctx.moveTo(startX + (coords.x - startX) / 2, startY);
                ctx.lineTo(startX, coords.y);
                ctx.lineTo(coords.x, coords.y);
                ctx.closePath();
                ctx.stroke();
            }
            ctx.restore();
            // Sync preview with current editor state (including the ghost shape)
            renderPreview();
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

        if (isEditingNode) {
            isEditingNode = false;
            activeNodeIndex = -1;
            renderTimeline();
            return;
        }

        if (isResizingShape) {
            isResizingShape = false;
            activeHandle = null;
            renderTimeline();
            updateLayersPanel(); // Ensure panel syncs
            return;
        }

        if (isDraggingShape) {
            isDraggingShape = false;
            renderTimeline();
            updateLayersPanel(); // Ensure panel syncs
            return;
        }

        if (isDrawingShape) {
            const frame = frames[currentFrameIndex];

            // Pixel Shapes (Direct Draw)
            if (['line', 'rect', 'circle'].includes(currentTool)) {
                if (currentTool === 'line') {
                    drawLineOnFrame(startX, startY, coords.x, coords.y, currentColor);
                    if (isSymmetryEnabled) {
                        drawLineOnFrame(GRID_WIDTH - startX, startY, GRID_WIDTH - coords.x, coords.y, currentColor);
                    }
                }
                else if (currentTool === 'rect') {
                    drawRectOnFrame(startX, startY, coords.x, coords.y, currentColor);
                    if (isSymmetryEnabled) {
                        drawRectOnFrame(GRID_WIDTH - startX, startY, GRID_WIDTH - coords.x, coords.y, currentColor);
                    }
                }
                else if (currentTool === 'circle') {
                    drawCircleOnFrame(startX, startY, coords.x, coords.y, currentColor);
                    if (isSymmetryEnabled) {
                        drawCircleOnFrame(GRID_WIDTH - startX, startY, GRID_WIDTH - coords.x, coords.y, currentColor);
                    }
                }
            }
            // Vector Objects
            else {
                // Normalize dimensions (handle dragging backwards)
                const x = Math.min(startX, coords.x);
                const y = Math.min(startY, coords.y);
                const w = Math.abs(coords.x - startX);
                const h = Math.abs(coords.y - startY);

                let newShape = null;

                if (currentTool === 'rect-obj') {
                    // Prevent tiny accidental clicks
                    if (w > 2 || h > 2) {
                        newShape = new Shape('rect', x, y, w, h, currentColor);
                    }
                } else if (currentTool === 'ellipse-obj') {
                    if (w > 2 || h > 2) {
                        newShape = new Shape('ellipse', x, y, w, h, currentColor);
                    }
                } else if (currentTool === 'line-obj') {
                    newShape = new Shape('line', startX, startY, 0, 0, currentColor);
                    newShape.lineEnd = { x: coords.x, y: coords.y };
                    // Recalculate bounds for line
                    newShape.width = Math.abs(coords.x - startX);
                    newShape.height = Math.abs(coords.y - startY);
                    newShape.x = Math.min(startX, coords.x);
                    newShape.y = Math.min(startY, coords.y);
                } else if (currentTool === 'triangle-obj') {
                    if (w > 2 || h > 2) {
                        newShape = new Shape('triangle', x, y, w, h, currentColor);
                    }
                } else if (currentTool === 'text-obj') {
                    // Text is special: Click to place, then prompt
                    const text = prompt("Enter text:", "TEXT");
                    if (text) {
                        newShape = new TextShape(coords.x, coords.y, text, 20, currentColor);
                    }
                }

                if (newShape) {
                    saveUndo(); // Save state before adding
                    frame.shapes.push(newShape);

                    if (isSymmetryEnabled) {
                        const mirrored = newShape.clone();
                        mirrored.id = Date.now() + Math.random();
                        if (newShape.type === 'line' && newShape.lineEnd) {
                            mirrored.x = GRID_WIDTH - newShape.x;
                            mirrored.lineEnd.x = GRID_WIDTH - newShape.lineEnd.x;
                        } else {
                            mirrored.x = GRID_WIDTH - newShape.x - newShape.width;
                        }
                        frame.shapes.push(mirrored);
                    }

                    selectedShape = newShape; // Auto-select new shape
                    currentTool = 'select'; // Switch to select to move/edit
                    updateToolButtons();
                    renderEditor();
                    updateLayersPanel();
                }
            }

            renderEditor();
            renderTimeline();
        }

        isDrawing = false;
        isDrawingShape = false;
    };

    // Double click to edit text
    canvas.addEventListener('dblclick', (e) => {
        const coords = getCoordsFromEvent(e);
        const frame = frames[currentFrameIndex];
        if (!frame || !frame.shapes) return;

        // Check shapes in reverse order (top to bottom)
        for (let i = frame.shapes.length - 1; i >= 0; i--) {
            const shape = frame.shapes[i];
            if (shape.containsPoint(coords.x, coords.y)) {
                if (shape.type === 'text') {
                    const newText = prompt("Edit text:", shape.text);
                    if (newText !== null) {
                        saveUndo();
                        shape.text = newText;
                        renderEditor();
                        updateLayersPanel();
                    }
                }
                break; // Handled top-most shape
            }
        }
    });

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
    const previewContainer = document.getElementById('preview-container');

    if (wrapper) {
        if (isRound) wrapper.classList.add('canvas-round');
        else wrapper.classList.remove('canvas-round');
    }

    if (previewContainer) {
        if (isRound) {
            previewContainer.classList.add('canvas-round');
        } else {
            previewContainer.classList.remove('canvas-round');
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
        const loopMode = document.getElementById('loop-mode')?.value || 'infinite';

        // Loop Logic
        let currentTime = 0;

        if (loopMode === 'once') {
            if (elapsed >= totalDuration) {
                stopPlay();
                return;
            }
            currentTime = elapsed;
        } else if (loopMode === 'pingpong') {
            const cycle = 2 * totalDuration;
            const phase = elapsed % cycle;
            if (phase < totalDuration) {
                currentTime = phase; // Forward
            } else {
                currentTime = (2 * totalDuration) - phase; // Backward
            }
        } else {
            // Infinite or multipliers (basic loop)
            currentTime = elapsed % totalDuration;
        }

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

            // Highlight active block - OPTIMIZED: only update if changed
            const timelineArea = document.getElementById('timeline-area');
            if (timelineArea) {
                const prevActive = timelineArea.querySelector('.frame-block.active');
                if (prevActive) prevActive.classList.remove('active');

                const currentBlock = timelineArea.querySelector(`.frame-block[data-index="${currentFrameIndex}"]`);
                if (currentBlock) currentBlock.classList.add('active');
            }
        }

        // 2. Render Preview (With Interpolation & Easing)
        const frame = frames[currentFrameIndex];
        const nextFrameIndex = (currentFrameIndex + 1) % frames.length;
        const nextFrame = frames[nextFrameIndex];
        const frameDur = parseInt(frame.duration || 100);
        let t = Math.min(1, Math.max(0, localTime / frameDur)); // 0.0 to 1.0 progress

        // Apply Easing
        const easingMode = document.getElementById('easing-mode')?.value || 'linear';
        t = applyEasing(t, easingMode);

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

        // Update timer text directly
        updatePlayheadTime(currentTime);

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

// Helper for Easing Functions
function applyEasing(t, type) {
    switch (type) {
        case 'ease-in': return t * t;
        case 'ease-out': return t * (2 - t);
        case 'ease-in-out': return t < .5 ? 2 * t * t : -1 + (4 - 2 * t) * t;
        case 'overshoot': return 2.70158 * t * t * t - 1.70158 * t * t;
        case 'bounce':
            if (t < (1 / 2.75)) return 7.5625 * t * t;
            else if (t < (2 / 2.75)) return 7.5625 * (t -= (1.5 / 2.75)) * t + 0.75;
            else if (t < (2.5 / 2.75)) return 7.5625 * (t -= (2.25 / 2.75)) * t + 0.9375;
            else return 7.5625 * (t -= (2.625 / 2.75)) * t + 0.984375;
        case 'spring':
            return 1 - Math.pow(Math.E, -5 * t) * Math.cos(10 * t);
        case 'custom':
            const p1x = parseFloat(document.getElementById('bezier-p1x')?.value || 0.42);
            const p1y = parseFloat(document.getElementById('bezier-p1y')?.value || 0);
            const p2x = parseFloat(document.getElementById('bezier-p2x')?.value || 0.58);
            const p2y = parseFloat(document.getElementById('bezier-p2y')?.value || 1);
            return getCubicBezier(t, p1x, p1y, p2x, p2y);
        default: return t; // Linear
    }
}

function getCubicBezier(t, p1x, p1y, p2x, p2y) {
    // Basic cubic bezier: (1-t)^3*P0 + 3*(1-t)^2*t*P1 + 3*(1-t)*t^2*P2 + t^3*P3
    // P0 is (0,0), P3 is (1,1)
    // We need Y given X=t. This is a simplification where we assume t is roughly X.
    // For a true cubic bezier as a function of time, we solve for t' such that X(t') = t.
    // For this UI, t is already the normalized time [0, 1].
    const cx = 3 * p1x;
    const bx = 3 * (p2x - p1x) - cx;
    const ax = 1 - cx - bx;
    const cy = 3 * p1y;
    const by = 3 * (p2y - p1y) - cy;
    const ay = 1 - cy - by;

    const sampleCurveX = (t) => ((ax * t + bx) * t + cx) * t;
    const sampleCurveY = (t) => ((ay * t + by) * t + cy) * t;
    const sampleCurveDerivativeX = (t) => (3.0 * ax * t + 2.0 * bx) * t + cx;

    // Solve for t' using Newton's method
    let x = t, t2 = t, i;
    for (i = 0; i < 8; i++) {
        x = sampleCurveX(t2) - t;
        if (Math.abs(x) < 1e-3) break;
        const d = sampleCurveDerivativeX(t2);
        if (Math.abs(d) < 1e-3) break;
        t2 = t2 - x / d;
    }
    return sampleCurveY(t2);
}

function drawCurvePreview() {
    const canvas = document.getElementById('curve-canvas');
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const w = canvas.width;
    const h = canvas.height;
    const padding = 20;

    ctx.clearRect(0, 0, w, h);

    // Draw grid
    ctx.strokeStyle = 'rgba(255,255,255,0.05)';
    ctx.lineWidth = 1;
    ctx.beginPath();
    for (let i = 1; i < 4; i++) {
        const x = padding + (w - 2 * padding) * (i / 4);
        const y = padding + (h - 2 * padding) * (i / 4);
        ctx.moveTo(x, padding); ctx.lineTo(x, h - padding);
        ctx.moveTo(padding, y); ctx.lineTo(w - padding, y);
    }
    ctx.stroke();

    const p1x = parseFloat(document.getElementById('bezier-p1x')?.value || 0.42);
    const p1y = parseFloat(document.getElementById('bezier-p1y')?.value || 0);
    const p2x = parseFloat(document.getElementById('bezier-p2x')?.value || 0.58);
    const p2y = parseFloat(document.getElementById('bezier-p2y')?.value || 1);

    const mapX = (x) => padding + x * (w - 2 * padding);
    const mapY = (y) => h - (padding + y * (h - 2 * padding));

    // Draw active curve
    ctx.strokeStyle = 'var(--accent)';
    ctx.lineWidth = 3;
    ctx.shadowBlur = 10;
    ctx.shadowColor = 'rgba(0, 210, 255, 0.5)';
    ctx.beginPath();
    ctx.moveTo(mapX(0), mapY(0));

    const mode = document.getElementById('easing-mode')?.value || 'linear';
    for (let i = 0; i <= 100; i++) {
        const t = i / 100;
        const val = applyEasing(t, mode);
        ctx.lineTo(mapX(t), mapY(val));
    }
    ctx.stroke();
    ctx.shadowBlur = 0;

    // Draw handles if custom
    if (mode === 'custom') {
        ctx.setLineDash([2, 2]);
        ctx.strokeStyle = 'rgba(255,255,255,0.2)';
        ctx.beginPath();
        ctx.moveTo(mapX(0), mapY(0)); ctx.lineTo(mapX(p1x), mapY(p1y));
        ctx.moveTo(mapX(1), mapY(1)); ctx.lineTo(mapX(p2x), mapY(p2y));
        ctx.stroke();
        ctx.setLineDash([]);

        ctx.fillStyle = '#ff0055';
        ctx.beginPath(); ctx.arc(mapX(p1x), mapY(p1y), 4, 0, Math.PI * 2); ctx.fill();
        ctx.fillStyle = '#00ff66';
        ctx.beginPath(); ctx.arc(mapX(p2x), mapY(p2y), 4, 0, Math.PI * 2); ctx.fill();
    }
}

// Helper for Linear Interpolation
function lerp(start, end, t) {
    if (start === undefined || end === undefined || isNaN(start) || isNaN(end)) {
        return start !== undefined ? start : 0;
    }
    return start + (end - start) * t;
}

function renderPreview(frameIndex, interpolation = null) {
    if (!previewCanvas || !previewCtx) return;

    // Use current frame by default
    if (frameIndex === undefined) frameIndex = currentFrameIndex;
    const frame = frames[frameIndex];
    if (!frame) return;

    const pCtx = previewCtx;
    pCtx.save();

    // 1. Clear with Background
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
            data[idx] = r; data[idx + 1] = g; data[idx + 2] = b; data[idx + 3] = 255;
        }
    }
    pCtx.putImageData(imgData, 0, 0);

    // 3. Render Shapes (Interpolated)
    if (frame.shapes) {
        // Collect all unique IDs from both frames to handle appearing/disappearing objects
        const nextFrame = interpolation ? interpolation.nextFrame : null;
        const t = interpolation ? interpolation.t : 0;

        const allIds = new Set([
            ...frame.shapes.map(s => s.id),
            ...(nextFrame ? nextFrame.shapes.map(s => s.id) : [])
        ]);

        allIds.forEach(id => {
            const s1 = frame.shapes.find(s => s.id === id);
            const s2 = nextFrame ? nextFrame.shapes.find(s => s.id === id) : null;

            let props = null;

            if (s1 && s2 && s1.type === s2.type) {
                // Interpolate
                props = {
                    type: s1.type,
                    x: lerp(s1.x, s2.x, t),
                    y: lerp(s1.y, s2.y, t),
                    width: lerp(s1.width, s2.width, t),
                    height: lerp(s1.height, s2.height, t),
                    rotation: lerp(s1.rotation || 0, s2.rotation || 0, t),
                    color: s1.color,
                    strokeColor: s1.strokeColor || s1.color,
                    strokeWidth: lerp(s1.strokeWidth || 0, s2.strokeWidth || 0, t),
                    cornerRadius: lerp(s1.cornerRadius || 0, s2.cornerRadius || 0, t),
                    opacity: lerp(s1.opacity !== undefined ? s1.opacity : 1, s2.opacity !== undefined ? s2.opacity : 1, t),
                    blendMode: s1.blendMode,
                    text: s1.text,
                    fontSize: s1.fontSize,
                    pathData: s1.pathData,
                    isMirrored: s1.isMirrored,
                    lineEnd: (s1.type === 'line' && s1.lineEnd && s2.lineEnd) ? {
                        x: lerp(s1.lineEnd.x, s2.lineEnd.x, t),
                        y: lerp(s1.lineEnd.y, s2.lineEnd.y, t)
                    } : null
                };
            } else if (s1) {
                // Disappearing (fade out if interpolating)
                props = { ...s1, opacity: s1.opacity * (1 - t) };
            } else if (s2 && t > 0) {
                // Appearing (fade in)
                props = { ...s2, opacity: s2.opacity * t };
            }

            if (props) {
                pCtx.save();
                pCtx.globalAlpha = props.opacity !== undefined ? props.opacity : 1;
                pCtx.globalCompositeOperation = props.blendMode || 'source-over';
                pCtx.fillStyle = props.color || '#ffffff';
                pCtx.strokeStyle = props.strokeColor || props.color || '#ffffff';
                pCtx.lineWidth = props.strokeWidth || 0;

                // Apply rotation
                if (props.rotation) {
                    const cx = props.x + (props.width || 0) / 2;
                    const cy = props.y + (props.height || 0) / 2;
                    pCtx.translate(cx, cy);
                    pCtx.rotate((props.rotation * Math.PI) / 180);
                    pCtx.translate(-cx, -cy);
                }

                if (props.type === 'rect') {
                    pCtx.fillRect(props.x, props.y, props.width, props.height);
                } else if (props.type === 'ellipse') {
                    pCtx.beginPath();
                    pCtx.ellipse(props.x + props.width / 2, props.y + props.height / 2,
                        Math.abs(props.width / 2), Math.abs(props.height / 2), 0, 0, Math.PI * 2);
                    pCtx.fill();
                } else if (props.type === 'triangle') {
                    pCtx.beginPath();
                    pCtx.moveTo(props.x + props.width / 2, props.y);
                    pCtx.lineTo(props.x, props.y + props.height);
                    pCtx.lineTo(props.x + props.width, props.y + props.height);
                    pCtx.closePath();
                    pCtx.fill();
                } else if (props.type === 'path' && props.pathData) {
                    pCtx.save();
                    const p = new Path2D(props.pathData);
                    if (props.isMirrored) {
                        pCtx.translate(props.x + props.width, props.y);
                        pCtx.scale(-1, 1);
                    } else {
                        pCtx.translate(props.x, props.y);
                    }
                    pCtx.scale(props.width / 100, props.height / 60);
                    pCtx.fill(p);
                    if (props.strokeWidth > 0) {
                        pCtx.lineWidth = props.strokeWidth / (props.width / 100);
                        pCtx.stroke(p);
                    }
                    pCtx.restore();
                } else if (props.type === 'line' && props.lineEnd) {
                    pCtx.beginPath(); pCtx.moveTo(props.x, props.y); pCtx.lineTo(props.lineEnd.x, props.lineEnd.y); pCtx.stroke();
                } else if (props.type === 'text') {
                    pCtx.font = `${props.fontSize || 16}px Inter, sans-serif`;
                    pCtx.fillText(props.text || '', props.x, props.y + (props.fontSize || 16));
                }
                pCtx.restore();
            }
        });
    }

    // 4. Ghost Shape (Sync drawing)
    const isDrawing = document.body.classList.contains('drawing-tool-active'); // We should check a variable instead
    // Since we are in script.js, let's just check the existing boolean
    if (typeof isDrawingShape !== 'undefined' && isDrawingShape && lastCoords) {
        pCtx.strokeStyle = currentColor;
        pCtx.fillStyle = currentColor;
        pCtx.globalAlpha = 0.5;

        if (currentTool.includes('line')) {
            pCtx.beginPath(); pCtx.moveTo(startX, startY); pCtx.lineTo(lastCoords.x, lastCoords.y); pCtx.stroke();
        } else if (currentTool.includes('rect')) {
            pCtx.strokeRect(startX, startY, lastCoords.x - startX, lastCoords.y - startY);
        } else if (currentTool.includes('circle') || currentTool.includes('ellipse')) {
            const w = lastCoords.x - startX; const h = lastCoords.y - startY;
            pCtx.beginPath(); pCtx.ellipse(startX + w / 2, startY + h / 2, Math.abs(w / 2), Math.abs(h / 2), 0, 0, Math.PI * 2); pCtx.stroke();
        }
    }

    pCtx.restore();
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

// ======== EXPORT TO C (BITMAP BAKING) ========
async function exportToC() {
    let name = document.getElementById('anim-name')?.value || 'my_anim';
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
    const bakeCanvas = document.createElement('canvas');
    bakeCanvas.width = GRID_WIDTH;
    bakeCanvas.height = GRID_HEIGHT;
    const bakeCtx = bakeCanvas.getContext('2d');

    for (let idx = 0; idx < frames.length; idx++) {
        const frame = frames[idx];
        const frameName = `${name}_f${idx}`;
        dscNames.push(frameName);

        bakeCtx.fillStyle = '#000000';
        bakeCtx.fillRect(0, 0, GRID_WIDTH, GRID_HEIGHT);

        const pImgData = bakeCtx.createImageData(GRID_WIDTH, GRID_HEIGHT);
        const pData = pImgData.data;
        for (let i = 0; i < frame.pixels.length; i++) {
            const color = frame.pixels[i];
            if (color) {
                const r = parseInt(color.slice(1, 3), 16);
                const g = parseInt(color.slice(3, 5), 16);
                const b = parseInt(color.slice(5, 7), 16);
                const idx = i * 4;
                pData[idx] = r; pData[idx + 1] = g; pData[idx + 2] = b; pData[idx + 3] = 255;
            }
        }
        bakeCtx.putImageData(pImgData, 0, 0);

        if (frame.shapes && frame.shapes.length > 0) {
            bakeCtx.globalAlpha = 1.0;
            bakeCtx.globalCompositeOperation = 'source-over';
            frame.shapes.forEach(shape => shape.draw(bakeCtx));
        }

        const finalImgData = bakeCtx.getImageData(0, 0, GRID_WIDTH, GRID_HEIGHT);
        const rgba = finalImgData.data;

        cContent += `const LV_ATTRIBUTE_MEM_ALIGN uint8_t ${frameName}_map[] = {\n`;
        let line = "    ";
        let byteCount = 0;

        for (let i = 0; i < rgba.length; i += 4) {
            const r = rgba[i];
            const g = rgba[i + 1];
            const b = rgba[i + 2];
            const r5 = (r >> 3) & 0x1F;
            const g6 = (g >> 2) & 0x3F;
            const b5 = (b >> 3) & 0x1F;
            const rgb565 = (r5 << 11) | (g6 << 5) | b5;
            const lowByte = rgb565 & 0xFF;
            const highByte = (rgb565 >> 8) & 0xFF;
            line += `0x${lowByte.toString(16).padStart(2, '0')}, 0x${highByte.toString(16).padStart(2, '0')}, `;
            byteCount += 2;
            if (byteCount >= 24) {
                cContent += line + "\n";
                line = "    ";
                byteCount = 0;
            }
        }
        if (line.trim() !== "") cContent += line + "\n";
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

    cContent += `const lv_img_dsc_t* ${name}_frames[] = {\n`;
    dscNames.forEach(f => cContent += `    &${f},\n`);
    cContent += `};\n\n`;
    cContent += `const uint8_t ${name}_frame_count = ${frames.length};\n`;

    const hContent = `#ifndef ${name.toUpperCase()}_H\n#define ${name.toUpperCase()}_H\n#include "lvgl/lvgl.h"\nextern const lv_img_dsc_t* ${name}_frames[];\nextern const uint8_t ${name}_frame_count;\n#endif\n`;

    showToast('Generating Bitmap files...');
    await saveFile(cContent, `${name}.c`, 'text/x-c');
    await new Promise(r => setTimeout(r, 500));
    await saveFile(hContent, `${name}.h`, 'text/x-c');
    showToast(`✅ Exported ${name}.c (Bitmap)`);
}

// ======== NATIVE LVGL EXPORT (STATE MACHINE SUPPORT) ========
async function exportToLVGLNative() {
    let baseName = document.getElementById('anim-name')?.value || 'my_anim';
    baseName = baseName.replace(/[^a-zA-Z0-9_]/g, '_').toLowerCase();

    // Cache current frames to active state
    if (activeStateId) {
        const current = projectStates.find(s => s.id === activeStateId);
        if (current) current.frames = [...frames];
    }

    let cContent = `/**
 * @file ${baseName}_native.c
 * @brief Auto-generated Native LVGL state machine from Robot Face Studio
 */
#include "lvgl/lvgl.h"

// Forward declarations
static lv_obj_t * face_container = NULL;
void ${baseName}_switch_state(const char * state_id);

// Interaction callback
static void interaction_cb(lv_event_t * e) {
    const char * next_state = (const char *)lv_event_get_user_data(e);
    if (next_state) {
        ${baseName}_switch_state(next_state);
    }
}

// Helper to set bg opacity for animations
static void _lv_obj_set_bg_opa_anim(void * obj, int32_t v) {
    lv_obj_set_style_bg_opa((lv_obj_t*)obj, v, 0);
}

`;

    // Generate specific creation functions for each state
    projectStates.forEach(state => {
        cContent += `// --- State: ${state.name} ---\n`;
        cContent += `static void create_state_${state.id}(lv_obj_t * parent) {\n`;

        const stateFrames = state.frames;
        if (!stateFrames || stateFrames.length === 0) {
            cContent += `}\n\n`;
            return;
        }

        // Identify unique shapes in THIS state
        const stateShapeIds = new Set();
        stateFrames.forEach(f => f.shapes.forEach(s => stateShapeIds.add(s.id)));

        const idToVar = {};
        let shapeIdx = 0;
        stateShapeIds.forEach(id => {
            idToVar[id] = `obj_${shapeIdx++}`;
            cContent += `    lv_obj_t * ${idToVar[id]} = NULL;\n`;
        });

        stateShapeIds.forEach(id => {
            const varName = idToVar[id];
            let firstFrameIdx = -1;
            for (let i = 0; i < stateFrames.length; i++) {
                if (stateFrames[i].shapes.find(s => s.id === id)) {
                    firstFrameIdx = i; break;
                }
            }
            if (firstFrameIdx === -1) return;
            const s = stateFrames[firstFrameIdx].shapes.find(sh => sh.id === id);

            if (s.type === 'text') {
                cContent += `    ${varName} = lv_label_create(parent);\n`;
                cContent += `    lv_label_set_text(${varName}, "${s.text.replace(/"/g, '\\"')}");\n`;
            } else {
                cContent += `    ${varName} = lv_obj_create(parent);\n`;
                cContent += `    lv_obj_set_size(${varName}, ${Math.round(s.width)}, ${Math.round(s.height)});\n`;

                if (s.type === 'ellipse') {
                    cContent += `    lv_obj_set_style_radius(${varName}, LV_RADIUS_CIRCLE, 0);\n`;
                } else if (s.cornerRadius > 0) {
                    cContent += `    lv_obj_set_style_radius(${varName}, ${Math.round(s.cornerRadius)}, 0);\n`;
                } else {
                    cContent += `    lv_obj_set_style_radius(${varName}, 0, 0);\n`;
                }

                cContent += `    lv_obj_set_style_bg_color(${varName}, lv_color_hex(0x${s.color.replace('#', '')}), 0);\n`;

                if (s.strokeWidth > 0) {
                    cContent += `    lv_obj_set_style_border_width(${varName}, ${Math.round(s.strokeWidth)}, 0);\n`;
                    cContent += `    lv_obj_set_style_border_color(${varName}, lv_color_hex(0x${s.strokeColor.replace('#', '')}), 0);\n`;
                } else {
                    cContent += `    lv_obj_set_style_border_width(${varName}, 0, 0);\n`;
                }
            }

            cContent += `    lv_obj_set_pos(${varName}, ${Math.round(s.x)}, ${Math.round(s.y)});\n`;
            cContent += `    lv_obj_set_style_bg_opa(${varName}, ${Math.round(s.opacity * 255)}, 0);\n`;
            if (firstFrameIdx > 0) cContent += `    lv_obj_add_flag(${varName}, LV_OBJ_FLAG_HIDDEN);\n`;

            if (s.interaction && s.interaction.trigger === 'click') {
                cContent += `    lv_obj_add_flag(${varName}, LV_OBJ_FLAG_CLICKABLE);\n`;
                cContent += `    lv_obj_add_event_cb(${varName}, interaction_cb, LV_EVENT_CLICKED, (void*)"${s.interaction.targetStateId}");\n`;
            }
        });

        // Animations for this state
        const easingMap = { 'linear': 'lv_anim_path_linear', 'ease-in': 'lv_anim_path_ease_in', 'ease-out': 'lv_anim_path_ease_out', 'ease-in-out': 'lv_anim_path_ease_in_out', 'overshoot': 'lv_anim_path_overshoot', 'bounce': 'lv_anim_path_bounce' };
        const globalEasing = document.getElementById('easing-mode')?.value || 'linear';
        const lvglEasing = easingMap[globalEasing] || 'lv_anim_path_linear';

        let timeAccum = 0;
        for (let i = 0; i < stateFrames.length - 1; i++) {
            const f1 = stateFrames[i];
            const f2 = stateFrames[i + 1];
            f1.shapes.forEach(s1 => {
                const s2 = f2.shapes.find(s => s.id === s1.id);
                if (s2) {
                    const varName = idToVar[s1.id];
                    const props = [
                        { k: 'x', cb: 'lv_obj_set_x' },
                        { k: 'y', cb: 'lv_obj_set_y' },
                        { k: 'width', cb: 'lv_obj_set_width' },
                        { k: 'height', cb: 'lv_obj_set_height' },
                        { k: 'opacity', cb: '_lv_obj_set_bg_opa_anim', s: 255 },
                        { k: 'strokeWidth', cb: 'lv_obj_set_style_border_width' }
                    ];
                    props.forEach(p => {
                        const v1 = p.s ? s1[p.k] * p.s : s1[p.k];
                        const v2 = p.s ? s2[p.k] * p.s : s2[p.k];
                        if (Math.abs(v1 - v2) > 0.1) {
                            cContent += `    { lv_anim_t a; lv_anim_init(&a); lv_anim_set_var(&a, ${varName}); lv_anim_set_values(&a, ${Math.round(v1)}, ${Math.round(v2)}); lv_anim_set_time(&a, ${f1.duration}); lv_anim_set_delay(&a, ${timeAccum}); lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)${p.cb}); lv_anim_set_path_cb(&a, ${lvglEasing}); lv_anim_start(&a); }\n`;
                        }
                    });
                }
            });
            timeAccum += f1.duration;
        }
        cContent += `}\n\n`;
    });

    // Central Controller
    cContent += `void ${baseName}_switch_state(const char * state_id) {\n`;
    cContent += `    if (!face_container) return;\n`;
    cContent += `    lv_obj_clean(face_container);\n`;
    projectStates.forEach((state, i) => {
        const cond = i === 0 ? `if` : `else if`;
        cContent += `    ${cond} (strcmp(state_id, "${state.id}") == 0) create_state_${state.id}(face_container);\n`;
    });
    cContent += `}\n\n`;

    cContent += `void init_${baseName}(lv_obj_t * parent) {\n`;
    cContent += `    face_container = lv_obj_create(parent);\n`;
    cContent += `    lv_obj_set_size(face_container, LV_PCT(100), LV_PCT(100));\n`;
    cContent += `    lv_obj_set_style_bg_opa(face_container, 0, 0);\n`;
    cContent += `    lv_obj_set_style_border_width(face_container, 0, 0);\n`;
    cContent += `    ${baseName}_switch_state("${activeStateId || projectStates[0].id}");\n`;
    cContent += `}\n`;

    await saveFile(cContent, `${baseName}_native.c`, 'text/x-c');
    showToast(`✅ Exported State Machine: ${baseName}_native.c`);
}

// ======== SEND TO ROBOT (BRIDGE) ========
async function sendToRobot() {
    let name = document.getElementById('anim-name')?.value || 'my_anim';
    // Sanitize
    name = name.replace(/[^a-zA-Z0-9_]/g, '_').toLowerCase();

    // Save current frames to active state
    if (activeStateId) {
        const current = projectStates.find(s => s.id === activeStateId);
        if (current) current.frames = [...frames];
    }

    const data = {
        name: name,
        width: GRID_WIDTH,
        height: GRID_HEIGHT,
        fps: parseInt(document.getElementById('fps-range')?.value || 12),
        activeStateId: activeStateId,
        states: projectStates.map(state => ({
            id: state.id,
            name: state.name,
            frames: state.frames.map(f => ({
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
                    id: s.id,
                    interaction: s.interaction
                })) : []
            }))
        }))
    };

    showToast('⚡ Sending to Robot Project...');

    // Smart URL: If running on server, use relative path. If file://, use localhost:8000
    const API_BASE = window.location.protocol === 'file:' ? 'http://localhost:8000' : '';

    try {
        const response = await fetch(`${API_BASE}/save-anim`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(data)
        });

        if (response.ok) {
            const result = await response.json();
            showToast('✅ Saved! Run "idf.py build" now.');
            console.log(result);
        } else {
            showToast('❌ Error: Start "run_bridge.bat" first!');
        }
    } catch (e) {
        console.error(e);
        showToast('❌ Failed! Is bridge server running?');
    }
}

// ======== DELETE FROM ROBOT ========
async function deleteFromRobot() {
    let name = document.getElementById('anim-name')?.value;
    if (!name) return;

    // Sanitize
    name = name.replace(/[^a-zA-Z0-9_]/g, '_').toLowerCase();

    if (!confirm(`Confirm delete '${name}' from Robot Project?`)) {
        return;
    }

    showToast(`🗑️ Deleting ${name}...`);

    const API_BASE = window.location.protocol === 'file:' ? 'http://localhost:8000' : '';

    try {
        const response = await fetch(`${API_BASE}/delete-anim`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ name: name })
        });

        if (response.ok) {
            const result = await response.json();
            showToast(`✅ Deleted ${name}! Run "idf.py build" to update.`);
            console.log(result);
        } else {
            showToast('❌ Error: Start "run_bridge.bat" first!');
        }
    } catch (e) {
        console.error(e);
        showToast('❌ Failed! Is bridge server running?');
    }
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

async function exportToJSON() {
    let name = document.getElementById('anim-name')?.value || 'animation';
    name = name.replace(/[^a-zA-Z0-9_-]/g, '_');

    // Save current frames to the active state before exporting
    if (activeStateId) {
        const current = projectStates.find(s => s.id === activeStateId);
        if (current) current.frames = [...frames];
    }

    const data = {
        version: "1.1",
        width: GRID_WIDTH,
        height: GRID_HEIGHT,
        fps: parseInt(document.getElementById('fps-range')?.value || 12),
        easingMode: document.getElementById('easing-mode')?.value || 'linear',
        activeStateId: activeStateId,
        states: projectStates.map(state => ({
            id: state.id,
            name: state.name,
            frames: state.frames.map((frame, index) => ({
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
                    id: s.id,
                    interaction: s.interaction,
                    strokeWidth: s.strokeWidth,
                    strokeColor: s.strokeColor,
                    cornerRadius: s.cornerRadius,
                    pathData: s.pathData,
                    isMirrored: s.isMirrored
                }))
            }))
        }))
    };

    const jsonContent = JSON.stringify(data, null, 2);
    await saveFile(jsonContent, `${name}.json`, 'application/json');
}

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

            // 1. Restore Global Settings
            if (data.width && data.height) {
                resizeCanvas(data.width, data.height);
            }

            if (data.fps) {
                const fpsRange = document.getElementById('fps-range');
                const fpsValue = document.getElementById('fps-value');
                if (fpsRange) fpsRange.value = data.fps;
                if (fpsValue) fpsValue.textContent = `${data.fps} FPS`;
            }

            if (data.easingMode) {
                const easingSelect = document.getElementById('easing-mode');
                if (easingSelect) {
                    easingSelect.value = data.easingMode;
                    // Trigger change event to update UI visibility
                    easingSelect.dispatchEvent(new Event('change'));
                }
            }

            // Restore Animation Name if saved
            const animNameInput = document.getElementById('anim-name');
            if (animNameInput && data.name) { // 'name' might not be at root in v1.1 export structure used, but let's check
                // actually exportToJSON doesn't save name at root, it saves activeStateId. 
                // Whatever, let's keep going.
            }


            if (data.states && Array.isArray(data.states)) {
                // v1.1 format with states
                projectStates = data.states.map(stateData => ({
                    id: stateData.id,
                    name: stateData.name,
                    frames: stateData.frames.map(frameData => {
                        const frame = new Frame();
                        frame.duration = frameData.duration || 100;
                        if (frameData.pixels) {
                            frameData.pixels.forEach(p => {
                                if (p && p.i !== undefined && p.c) frame.pixels[p.i] = p.c;
                            });
                        }
                        if (frameData.shapes) {
                            frame.shapes = frameData.shapes.map(s => {
                                const shape = new Shape(s.type, s.x, s.y, s.width, s.height, s.color);
                                shape.opacity = s.opacity ?? 1;
                                shape.blendMode = s.blendMode || 'source-over';
                                shape.rotation = s.rotation || 0;
                                shape.id = s.id || (Date.now() + Math.random());
                                if (s.lineEnd) shape.lineEnd = { ...s.lineEnd };
                                if (s.interaction) shape.interaction = { ...s.interaction };
                                shape.strokeWidth = s.strokeWidth || 0;
                                shape.strokeColor = s.strokeColor || '#ffffff';
                                shape.cornerRadius = s.cornerRadius || 0;
                                if (s.pathData) shape.pathData = s.pathData;
                                if (s.isMirrored) shape.isMirrored = s.isMirrored;

                                // Parse path nodes if it's a path type
                                if (shape.type === 'path') shape.parsePath();
                                return shape;
                            });
                        }
                        return frame;
                    })
                }));
                activeStateId = data.activeStateId || projectStates[0].id;
            } else if (data.frames) {
                // Legacy v1.0 format
                const framesList = data.frames.map(frameData => {
                    const frame = new Frame();
                    frame.duration = frameData.duration || 100;
                    if (frameData.pixels) {
                        frameData.pixels.forEach(p => {
                            if (p && p.i !== undefined && p.c) frame.pixels[p.i] = p.c;
                        });
                    }
                    if (frameData.shapes) {
                        frame.shapes = frameData.shapes.map(s => {
                            const shape = new Shape(s.type, s.x, s.y, s.width, s.height, s.color);
                            shape.opacity = s.opacity ?? 1;
                            shape.rotation = s.rotation || 0;
                            shape.id = s.id || (Date.now() + Math.random());
                            if (s.lineEnd) shape.lineEnd = { ...s.lineEnd };
                            if (s.pathData) {
                                shape.pathData = s.pathData;
                                shape.parsePath();
                            }
                            if (s.isMirrored) shape.isMirrored = s.isMirrored;
                            return shape;
                        });
                    }
                    return frame;
                });
                projectStates = [{ id: 'idle', name: 'Idle', frames: framesList }];
                activeStateId = 'idle';
            }

            // Manually set frames locally from the active state so ui updates correct
            const activeState = projectStates.find(s => s.id === activeStateId);
            if (activeState) {
                frames = activeState.frames;
                currentFrameIndex = 0;
            }

            // Force refresh
            if (typeof updateStatesPanel === 'function') updateStatesPanel();
            if (typeof drawCurvePreview === 'function') drawCurvePreview();

            renderTimeline();
            renderEditor();
            updateFrameInfo(); // Update duration inputs and stuff

            showToast(`Loaded ${projectStates.length} states`);
        } catch (err) {
            console.error(err);
            showToast('Failed to load file');
        }
    };

    input.click();
}

// Update Frame Info Panel
function updateFrameInfo() {
    if (!frames || frames.length === 0) return;
    const frame = frames[currentFrameIndex];
    if (!frame) return;

    // 1. Update Index Display
    const indexEl = document.getElementById('frame-index');
    if (indexEl) indexEl.textContent = currentFrameIndex + 1;

    // 2. Update Duration Input (Force value)
    const durationInput = document.getElementById('frame-duration');
    if (durationInput) {
        durationInput.value = frame.duration;
        // Visual cue that it's updated
        durationInput.style.color = '#00d2ff';
    }

    // 3. Update Easing/Interpolation if it's stored per frame (optional, for now use global)
    // const easingInput = document.getElementById('easing-mode');
    // if (easingInput && frame.easing) easingInput.value = frame.easing;

    // 4. Update Total Time
    const totalMs = frames.reduce((sum, f) => sum + (parseInt(f.duration) || 100), 0);
    const totalTimeEl = document.getElementById('total-time');
    if (totalTimeEl) totalTimeEl.textContent = (totalMs / 1000).toFixed(2) + 's';

    // 5. Update Playhead position and time display
    if (!isPlaying) {
        let timeMs = 0;
        for (let i = 0; i < currentFrameIndex; i++) {
            timeMs += parseInt(frames[i].duration) || 100;
        }
        updatePlayheadTime(timeMs);
    }

    // 6. Sync Layers count
    updateLayersPanel();
}

function updatePlayheadTime(timeMs) {
    const playheadTimeEl = document.getElementById('playhead-time');
    if (playheadTimeEl) {
        const mins = Math.floor(timeMs / 60000);
        const secs = Math.floor((timeMs % 60000) / 1000);
        const ms = Math.floor(timeMs % 1000);
        playheadTimeEl.textContent = `${mins.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}.${ms.toString().padStart(3, '0')}`;
    }
}

// Onion Skinning
let onionSkinEnabled = false;
let onionSkinOpacity = 0.3;

function toggleOnionSkin(enabled) {
    onionSkinEnabled = enabled;
    renderEditor();
}



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
            if (prevFrame.isCacheDirty) prevFrame.updateCache();
            ctx.drawImage(prevFrame.cacheCanvas, 0, 0);

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
    if (frame.isCacheDirty) frame.updateCache();
    ctx.drawImage(frame.cacheCanvas, 0, 0);

    // Draw shapes
    if (frame.shapes && frame.shapes.length > 0) {
        frame.shapes.forEach(shape => {
            shape.draw(ctx);

            // Draw Vector Nodes if editing
            if (shape === selectedShape && shape.type === 'path' && shape.isEditingPositions) {
                ctx.fillStyle = '#00d2ff';
                ctx.strokeStyle = '#ffffff';
                ctx.lineWidth = 1;

                shape.nodes.forEach((node, i) => {
                    if (node.type === 'Z') return;

                    // Convert normalized 100x60 Library coords to actual shape units
                    // (Actually nodes are already absolute relative to shape origin)
                    const nx = shape.x + (node.x * (shape.width / 100));
                    const ny = shape.y + (node.y * (shape.height / 60));

                    ctx.beginPath();
                    ctx.arc(nx, ny, 4, 0, Math.PI * 2);
                    ctx.fill();
                    ctx.stroke();
                });
            }
        });
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



// =========================================
// NEW TOOLS - Redo, Duplicate, Delete, Scale, Align, Layers, Text
// =========================================

// Redo Stack
let redoStack = [];

// Redo Stack (Already initialized above)

function redo() {
    if (redoStack.length === 0) return;

    const frame = frames[currentFrameIndex];
    if (!frame) return;

    // Save current state to Undo Stack
    undoStack.push(serializeFrame(frame));

    // Restore from Redo Stack
    const nextState = redoStack.pop();
    restoreFrame(frame, nextState);

    renderEditor();
    renderTimeline();
    updateLayerUI();
}

function duplicateShape() {
    if (!selectedShape) {
        alert('Select a shape first!');
        return;
    }
    const newShape = selectedShape.clone();
    newShape.x += 20;
    newShape.y += 20;
    frames[currentFrameIndex].shapes.push(newShape);
    selectedShape = newShape;
    renderEditor();
    renderPreview();
    updateLayersPanel();
}

function deleteShape() {
    if (!selectedShape) {
        alert('Select a shape first!');
        return;
    }
    saveUndo();
    const shapes = frames[currentFrameIndex].shapes;
    const idx = shapes.findIndex(s => s.id === selectedShape.id);
    if (idx !== -1) {
        shapes.splice(idx, 1);
        selectedShape = null;
        renderEditor();
        renderPreview();
        updateLayersPanel();
    }
}

function scaleShape(factor) {
    if (!selectedShape) {
        alert('Select a shape first!');
        return;
    }
    saveUndo();
    selectedShape.width *= factor;
    selectedShape.height *= factor;
    renderEditor();
    renderPreview();
}

// Align Functions
function alignShapes(direction) {
    if (!selectedShape) {
        alert('Select a shape first!');
        return;
    }
    saveUndo();
    const bounds = selectedShape.getBounds();
    switch (direction) {
        case 'left':
            selectedShape.x = 0;
            break;
        case 'center-h':
            selectedShape.x = (GRID_WIDTH - bounds.width) / 2;
            break;
        case 'right':
            selectedShape.x = GRID_WIDTH - bounds.width;
            break;
        case 'top':
            selectedShape.y = 0;
            break;
        case 'center-v':
            selectedShape.y = (GRID_HEIGHT - bounds.height) / 2;
            break;
        case 'bottom':
            selectedShape.y = GRID_HEIGHT - bounds.height;
            break;
    }
    renderEditor();
    renderPreview();
}

// Layer Management
function updateLayersPanel() {
    const listEl = document.getElementById('layers-list');
    if (!listEl) return;

    const shapes = frames[currentFrameIndex]?.shapes || [];

    if (shapes.length === 0) {
        listEl.innerHTML = '<div class="layer-item"><span class="layer-icon">○</span><span class="layer-name">No shapes</span></div>';
        return;
    }

    listEl.innerHTML = shapes.map((shape, i) => {
        const icon = shape.type === 'ellipse' ? '○' :
            shape.type === 'rect' ? '▢' :
                shape.type === 'line' ? '╱' :
                    shape.type === 'text' ? 'T' : '?';
        const isActive = selectedShape && selectedShape.id === shape.id;
        return `<div class="layer-item ${isActive ? 'active' : ''}" data-index="${i}">
            <span class="layer-icon">${icon}</span>
            <span class="layer-name">${shape.type} ${i + 1}</span>
            <span class="layer-visibility">👁</span>
        </div>`;
    }).reverse().join('');

    // Add click handlers
    listEl.querySelectorAll('.layer-item').forEach(item => {
        item.onclick = () => {
            const idx = parseInt(item.dataset.index);
            selectedShape = frames[currentFrameIndex].shapes[idx];
            renderEditor();
            updateLayersPanel();
        };
    });
}

function moveLayerUp() {
    if (!selectedShape) return;
    const shapes = frames[currentFrameIndex].shapes;
    const idx = shapes.findIndex(s => s.id === selectedShape.id);
    if (idx < shapes.length - 1) {
        [shapes[idx], shapes[idx + 1]] = [shapes[idx + 1], shapes[idx]];
        renderEditor();
        renderPreview();
        updateLayersPanel();
    }
}

function moveLayerDown() {
    if (!selectedShape) return;
    const shapes = frames[currentFrameIndex].shapes;
    const idx = shapes.findIndex(s => s.id === selectedShape.id);
    if (idx > 0) {
        [shapes[idx], shapes[idx - 1]] = [shapes[idx - 1], shapes[idx]];
        renderEditor();
        renderPreview();
        updateLayersPanel();
    }
}

// Text Shape (for LVGL lv_label)
class TextShape extends Shape {
    constructor(x, y, text, fontSize, color) {
        super('text', x, y, 100, 30, color);
        this.text = text || 'Text';
        this.fontSize = fontSize || 16;
    }

    draw(context) {
        context.save();
        context.globalAlpha = this.opacity;
        context.globalCompositeOperation = this.blendMode;
        context.fillStyle = this.color;
        context.font = `${this.fontSize}px Inter, sans-serif`;
        context.fillText(this.text, this.x, this.y + this.fontSize);

        // Measure for bounds
        const metrics = context.measureText(this.text);
        this.width = metrics.width;
        this.height = this.fontSize * 1.2;

        context.restore();
    }

    containsPoint(px, py) {
        return px >= this.x && px <= this.x + this.width &&
            py >= this.y && py <= this.y + this.height;
    }

    clone() {
        const newShape = new TextShape(this.x, this.y, this.text, this.fontSize, this.color);
        newShape.rotation = this.rotation;
        newShape.opacity = this.opacity;
        newShape.blendMode = this.blendMode;
        newShape.id = Date.now() + Math.random();
        newShape.width = this.width;
        newShape.height = this.height;
        return newShape;
    }
}

// Initialize new tool buttons
document.addEventListener('DOMContentLoaded', () => {
    // Redo
    document.getElementById('action-redo')?.addEventListener('click', redo);

    // Shape Actions
    document.getElementById('action-duplicate')?.addEventListener('click', duplicateShape);
    document.getElementById('action-delete-shape')?.addEventListener('click', deleteShape);
    document.getElementById('action-scale-up')?.addEventListener('click', () => scaleShape(1.1));
    document.getElementById('action-scale-down')?.addEventListener('click', () => scaleShape(0.9));

    // Align Tools
    document.getElementById('align-left')?.addEventListener('click', () => alignShapes('left'));
    document.getElementById('align-center-h')?.addEventListener('click', () => alignShapes('center-h'));
    document.getElementById('align-right')?.addEventListener('click', () => alignShapes('right'));
    document.getElementById('align-top')?.addEventListener('click', () => alignShapes('top'));
    document.getElementById('align-center-v')?.addEventListener('click', () => alignShapes('center-v'));
    document.getElementById('align-bottom')?.addEventListener('click', () => alignShapes('bottom'));

    // Layer Actions
    document.getElementById('layer-up-btn')?.addEventListener('click', moveLayerUp);
    document.getElementById('layer-down-btn')?.addEventListener('click', moveLayerDown);
    document.getElementById('layer-del-btn')?.addEventListener('click', deleteShape);

    // Text Tool
    document.getElementById('tool-text-obj')?.addEventListener('click', () => {
        currentTool = 'text-obj';
        updateToolButtons();
    });

    // Keyboard shortcuts
    document.addEventListener('keydown', (e) => {
        if (e.ctrlKey && e.key === 'y') {
            e.preventDefault();
            redo();
        }
        if (e.ctrlKey && e.key === 'd') {
            e.preventDefault();
            duplicateShape();
        }
        if (e.key === 'Delete' && selectedShape) {
            deleteShape();
        }
        if (e.key === 't' && !e.ctrlKey) {
            currentTool = 'text-obj';
            updateToolButtons();
        }
    });
});

function updateToolButtons() {
    document.querySelectorAll('.tool-btn').forEach(btn => btn.classList.remove('active'));
    const toolId = 'tool-' + currentTool;
    document.getElementById(toolId)?.classList.add('active');
}


// ======== INTERACTION & STATE MACHINE ========

function updateStatesPanel() {
    const listEl = document.getElementById('states-list');
    if (!listEl) return;

    if (projectStates.length === 0) {
        listEl.innerHTML = `
            <div class="text-[10px] text-zinc-600 font-bold text-center py-4 bg-black/20 rounded-lg border border-white/5 border-dashed">
                No states defined
            </div>`;
        return;
    }

    listEl.innerHTML = projectStates.map(state => {
        const isActive = activeStateId === state.id;
        return `
            <div class="flex items-center gap-2 p-2 rounded-lg cursor-pointer transition-all ${isActive ? 'bg-accent/20 border border-accent/20' : 'bg-white/5 hover:bg-white/10'}" 
                 onclick="switchState('${state.id}')">
                <div class="w-1.5 h-1.5 rounded-full ${isActive ? 'bg-accent animate-pulse' : 'bg-zinc-600'}"></div>
                <span class="text-[11px] font-bold flex-1 ${isActive ? 'text-accent' : 'text-zinc-400'}">${state.name}</span>
                <button onclick="event.stopPropagation(); deleteState('${state.id}')" class="p-1.5 text-zinc-600 hover:text-red-500 transition-colors">
                    <i data-lucide="x" class="w-3 h-3"></i>
                </button>
            </div>
        `;
    }).join('');

    if (window.lucide) lucide.createIcons();

    // Update the next-state dropdowns in the interaction editor
    const nextStateSelect = document.getElementById('trigger-next-state');
    if (nextStateSelect) {
        nextStateSelect.innerHTML = projectStates.map(s =>
            `<option value="${s.id}">${s.name}</option>`
        ).join('');
    }
}

function addState() {
    const name = prompt("Enter state name (e.g. Happy, Sad, Idle):", "New State");
    if (!name) return;

    const id = name.toLowerCase().replace(/[^a-z0-9]/g, '_');

    // Check if ID exists
    if (projectStates.find(s => s.id === id)) {
        alert("A state with this name already exists.");
        return;
    }

    const newState = {
        id: id,
        name: name,
        frames: [new Frame()]
    };

    projectStates.push(newState);

    if (!activeStateId) {
        switchState(id);
    } else {
        updateStatesPanel();
    }
}

function switchState(stateId) {
    if (activeStateId === stateId) return;

    // 1. Save current frames to existing state
    if (activeStateId) {
        const current = projectStates.find(s => s.id === activeStateId);
        if (current) current.frames = [...frames];
    }

    // 2. Load new state
    const next = projectStates.find(s => s.id === stateId);
    if (!next) return;

    activeStateId = stateId;
    frames = next.frames;
    currentFrameIndex = 0;

    // Update Animation Name Input
    const animNameInput = document.getElementById('anim-name');
    if (animNameInput) animNameInput.value = next.name;

    renderTimeline();
    renderEditor();
    updateStatesPanel();
    showToast(`Switched to: ${next.name}`);
}

function deleteState(stateId) {
    if (projectStates.length <= 1) {
        alert("You must have at least one state.");
        return;
    }

    if (!confirm(`Are you sure you want to delete the state '${stateId}'?`)) return;

    const idx = projectStates.findIndex(s => s.id === stateId);
    if (idx > -1) {
        projectStates.splice(idx, 1);
        if (activeStateId === stateId) {
            switchState(projectStates[0].id);
        } else {
            updateStatesPanel();
        }
    }
}

// Interaction & Styles Setup
function updateInteractionEditor() {
    const editor = document.getElementById('interaction-editor');
    const shapeExtras = document.getElementById('shape-extras');

    if (currentTool === 'select' && selectedShape) {
        // 1. Interaction Editor
        if (editor) editor.classList.remove('hidden');

        const triggerSelect = document.getElementById('trigger-type');
        const nextStateSelect = document.getElementById('trigger-next-state');

        if (selectedShape.interaction) {
            if (triggerSelect) triggerSelect.value = selectedShape.interaction.trigger || 'click';
            if (nextStateSelect) nextStateSelect.value = selectedShape.interaction.targetStateId || '';
        } else {
            if (triggerSelect) triggerSelect.value = 'click';
        }

        // 2. Shape Style Extras
        if (shapeExtras) {
            shapeExtras.classList.remove('hidden');

            // Corner Radius (Rect only)
            const radProp = document.getElementById('prop-corner-radius');
            if (radProp) {
                if (selectedShape.type === 'rect') {
                    radProp.classList.remove('hidden');
                    document.getElementById('corner-radius-slider').value = selectedShape.cornerRadius || 0;
                    document.getElementById('corner-radius-val').innerText = (selectedShape.cornerRadius || 0) + 'px';
                } else {
                    radProp.classList.add('hidden');
                }
            }

            // Sync Stroke UI
            document.getElementById('stroke-width-slider').value = selectedShape.strokeWidth || 0;
            document.getElementById('stroke-width-val').innerText = (selectedShape.strokeWidth || 0) + 'px';
            document.getElementById('stroke-color-picker').value = selectedShape.strokeColor || '#ffffff';
        }
    } else {
        if (editor) editor.classList.add('hidden');
        if (shapeExtras) shapeExtras.classList.add('hidden');
    }
}

function saveInteraction() {
    if (!selectedShape) return;

    const trigger = document.getElementById('trigger-type')?.value;
    const target = document.getElementById('trigger-next-state')?.value;

    selectedShape.interaction = {
        trigger: trigger,
        targetStateId: target
    };

    showToast(`Interaction saved for ${selectedShape.type}`);
    renderEditor();
}

// Preset Logic
function applyEyePreset(type) {
    const frame = frames[currentFrameIndex];
    if (!frame) return;

    saveUndo();

    // High-fidelity paths matched to the reference image
    const eyePaths = {
        angry: "M10,20 C30,15 70,5 95,25 C80,60 20,60 10,20 Z",
        alert: "M20,15 C50,0 100,20 80,55 C40,70 0,50 20,15 Z",
        sad: "M5,25 C30,10 70,10 95,25 C95,55 5,55 5,25 Z",
        evil: "M5,35 Q50,0 95,35 Q50,70 5,35 Z",
        bean: "M5,15 L95,15 Q95,60 50,60 Q5,60 5,15 Z",
        slit: "M5,25 L95,25 C95,45 5,45 5,25 Z" // Simplified Rect-like path
    };

    const path = eyePaths[type];
    const eyeWidth = 120;
    const eyeHeight = 72;
    const padding = 50;

    // Create Left Eye
    const leftEye = new Shape('path', (GRID_WIDTH / 2) - eyeWidth - padding, (GRID_HEIGHT / 2) - (eyeHeight / 2), eyeWidth, eyeHeight, currentColor);
    leftEye.pathData = path;

    // Create Right Eye (Mirroring)
    const rightEye = new Shape('path', (GRID_WIDTH / 2) + padding, (GRID_HEIGHT / 2) - (eyeHeight / 2), eyeWidth, eyeHeight, currentColor);

    // For asymmetric paths, we need to flip the path data for the right eye
    // Simplest way is to keep a 'mirrored' flag or transformation, 
    // but for now, we'll just use the same and rely on the user to flip if they want specialized mirroring,
    // OR we can manually flip the path (complex). Let's just draw it.
    rightEye.pathData = path;
    rightEye.isMirrored = true; // We'll add support for this in drawing

    frame.shapes.push(leftEye);
    frame.shapes.push(rightEye);

    selectedShape = rightEye;
    currentTool = 'select';
    updateToolButtons();
    renderEditor();
    updateLayersPanel();
    showToast(`Applied ${type} eye preset`);
}

// Initialize and hook events
window.addEventListener('DOMContentLoaded', () => {
    document.getElementById('state-add-btn')?.addEventListener('click', addState);
    document.getElementById('save-interaction-btn')?.addEventListener('click', saveInteraction);

    // Style Sliders
    document.getElementById('stroke-width-slider')?.addEventListener('input', (e) => {
        const val = parseInt(e.target.value);
        document.getElementById('stroke-width-val').innerText = val + 'px';
        if (selectedShape) {
            selectedShape.strokeWidth = val;
            renderEditor();
        }
    });

    document.getElementById('stroke-color-picker')?.addEventListener('input', (e) => {
        if (selectedShape) {
            selectedShape.strokeColor = e.target.value;
            renderEditor();
        }
    });

    document.getElementById('corner-radius-slider')?.addEventListener('input', (e) => {
        const val = parseInt(e.target.value);
        document.getElementById('corner-radius-val').innerText = val + 'px';
        if (selectedShape && selectedShape.type === 'rect') {
            selectedShape.cornerRadius = val;
            renderEditor();
        }
    });

    document.getElementById('tool-triangle-obj')?.addEventListener('click', () => {
        currentTool = 'triangle-obj';
        updateToolButtons();
    });

    document.getElementById('action-symmetry')?.addEventListener('click', (e) => {
        isSymmetryEnabled = !isSymmetryEnabled;
        const btn = e.currentTarget;
        if (isSymmetryEnabled) {
            btn.classList.add('bg-accent/20', 'text-accent');
            btn.querySelector('i').classList.remove('text-zinc-500');
            showToast("Horizontal Symmetry ON");
        } else {
            btn.classList.remove('bg-accent/20', 'text-accent');
            btn.querySelector('i').classList.add('text-zinc-500');
            showToast("Symmetry OFF");
        }
    });

    // Initialize with a default 'Idle' state if empty
    setTimeout(() => {
        if (projectStates.length === 0) {
            projectStates = [
                {
                    id: 'idle',
                    name: 'Idle',
                    frames: frames
                }
            ];
            activeStateId = 'idle';
            updateStatesPanel();
        }
    }, 100);
});

// ======== EYE PRESETS SYSTEM ========
window.applyEyePreset = function (type) {
    const frame = frames[currentFrameIndex];
    if (!frame) return;

    saveUndo();

    const eyePaths = {
        angry: "M10,20 C30,15 70,5 95,25 C80,60 20,60 10,20 Z",
        alert: "M20,15 C50,0 100,20 80,55 C40,70 0,50 20,15 Z",
        sad: "M5,25 C30,10 70,10 95,25 C95,55 5,55 5,25 Z",
        evil: "M5,35 Q50,0 95,35 Q50,70 5,35 Z",
        bean: "M5,15 L95,15 Q95,60 50,60 Q5,60 5,15 Z",
        slit: "M5,25 L95,25 C95,45 5,45 5,25 Z"
    };

    const path = eyePaths[type];
    const eyeWidth = 200;
    const eyeHeight = 130;
    const padding = 15;

    // Remove existing shapes if user wants to replace (optional, but let's just add)
    // Actually, adding is safer. User can clear if they want.

    // Create Left Eye
    const leftEye = new Shape('path', (GRID_WIDTH / 2) - eyeWidth - padding, (GRID_HEIGHT / 2) - (eyeHeight / 2), eyeWidth, eyeHeight, currentColor);
    leftEye.pathData = path;

    // Create Right Eye
    const rightEye = new Shape('path', (GRID_WIDTH / 2) + padding, (GRID_HEIGHT / 2) - (eyeHeight / 2), eyeWidth, eyeHeight, currentColor);
    rightEye.pathData = path;
    rightEye.isMirrored = true;

    frame.shapes.push(leftEye);
    frame.shapes.push(rightEye);

    selectedShape = rightEye;
    currentTool = 'select';
    updateToolButtons();
    renderEditor();
    updateLayersPanel();
    showToast(`Applied ${type} eyes`);
};
