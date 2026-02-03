import { state } from './state.js';
import { Frame } from './models.js';
// renderer imports removed to prevent circular dependency
import { updateFrameInfo, updatePlayheadTime } from './ui.js';
import { saveUndo, serializeFrame, restoreFrame } from './history.js';
import { applyEasing } from './utils.js';
import { PIXELS_PER_SEC } from './constants.js';
import { updatePlayheadPosition } from './playback.js';

// Timeline Elements
const track = document.getElementById('main-track');
const ruler = document.getElementById('timeline-ruler');
const playhead = document.getElementById('playhead');

export function initTimeline() {
    renderTimeline();
    initRuler();

    // Attach Resize Listeners for Frames
    if (track) {
        track.addEventListener('mousedown', (e) => {
            if (e.target.classList.contains('resize-handle')) {
                const frameEl = e.target.parentElement;
                state.isResizingFrame = true;
                state.resizeFrameIndex = parseInt(frameEl.dataset.index);
                state.resizeStartX = e.clientX;
                state.resizeStartDuration = state.frames[state.resizeFrameIndex].duration;
                e.preventDefault();
                e.stopPropagation();
            } else {
                const frameEl = e.target.closest('.frame-block');
                if (frameEl) {
                    state.currentFrameIndex = parseInt(frameEl.dataset.index);
                    renderTimeline();
                    if (state.actions.renderEditor) state.actions.renderEditor();
                }
            }
        });
    }

    window.addEventListener('mousemove', (e) => {
        if (state.isResizingFrame) {
            const dx = e.clientX - state.resizeStartX;
            const msDelta = (dx / PIXELS_PER_SEC) * 1000;
            let newDuration = Math.max(50, state.resizeStartDuration + msDelta);
            newDuration = Math.round(newDuration / 10) * 10; // Snap

            if (state.frames[state.resizeFrameIndex]) {
                state.frames[state.resizeFrameIndex].duration = newDuration;
                renderTimeline();
                // update info
                updateFrameInfo();
            }
        }
    });

    window.addEventListener('mouseup', () => {
        state.isResizingFrame = false;
    });
}

export function initRuler() {
    if (!ruler) return;
    const width = 2000; // Arbitrary large width or calc based on frames
    ruler.width = width;
    ruler.height = 30;
    const ctx = ruler.getContext('2d');
    ctx.fillStyle = '#222';
    ctx.fillRect(0, 0, width, 30);
    ctx.strokeStyle = '#555';
    ctx.lineWidth = 1;
    ctx.beginPath();

    // Draw Ticks every 100ms (10px at 100px/s)
    const pxPerSec = PIXELS_PER_SEC;
    const durationSec = width / pxPerSec;

    for (let t = 0; t <= durationSec; t += 0.1) {
        const x = t * pxPerSec;
        const height = (Math.round(t * 10) % 10 === 0) ? 20 : (Math.round(t * 10) % 5 === 0) ? 10 : 5;
        ctx.moveTo(x + 0.5, 30);
        ctx.lineTo(x + 0.5, 30 - height);

        if (Math.round(t * 10) % 10 === 0) {
            ctx.fillStyle = '#888';
            ctx.font = '10px sans-serif';
            ctx.fillText(Math.round(t) + 's', x + 4, 12);
        }
    }
    ctx.stroke();
}

// Timeline Rendering
export function renderTimeline() {
    if (!track) return;
    track.innerHTML = '';

    // We need a specific structure for sticky headers to work:
    // Container (Scrollable)
    //   -> Header Row (Ruler) - handled outside usually, but let's align
    //   -> Master Row
    //   -> Object Rows

    // 1. Collect all unique objects (tracks) with simplified ID mapping
    const objectTracks = new Map(); // ID -> { label, ranges: [] }

    state.frames.forEach((frame, fIdx) => {
        if (!frame.shapes) return;
        frame.shapes.forEach(shape => {
            if (!objectTracks.has(shape.id)) {
                let name = shape.type;
                if (shape.type === 'path') {
                    if (Object.values(state.EYE_PATHS || {}).includes(shape.pathData)) name = "Eye (Preset)";
                    else name = "Custom Path";
                }
                else if (shape.type === 'rect') name = "Rectangle";
                else if (shape.type === 'ellipse') name = "Ellipse";
                else if (shape.type === 'text') name = `"${shape.text}"`;
                else if (shape.type === 'image') name = "Image";

                objectTracks.set(shape.id, {
                    id: shape.id,
                    type: shape.type,
                    name: name,
                    existsInFrames: new Set()
                });
            }
            objectTracks.get(shape.id).existsInFrames.add(fIdx);
        });
    });

    // --- Helper: Convert Set to Ranges [start, duration_ms, start_px, width_px] ---
    const calculateRanges = (frameSet) => {
        const ranges = [];
        let currentStart = -1;
        let currentDuration = 0;
        let currentWidth = 0;
        let startPx = 0;

        // We need to iterate ALL frames to calculate strict pixel positions
        let pixelOffset = 0;

        state.frames.forEach((frame, i) => {
            const frameW = (frame.duration / 1000) * PIXELS_PER_SEC;

            if (frameSet.has(i)) {
                if (currentStart === -1) {
                    currentStart = i;
                    startPx = pixelOffset;
                }
                currentDuration += frame.duration;
                currentWidth += frameW;
            } else {
                if (currentStart !== -1) {
                    ranges.push({ start: currentStart, ms: currentDuration, x: startPx, w: currentWidth });
                    currentStart = -1;
                    currentDuration = 0;
                    currentWidth = 0;
                }
            }
            pixelOffset += frameW;
        });

        // Push last if active
        if (currentStart !== -1) {
            ranges.push({ start: currentStart, ms: currentDuration, x: startPx, w: currentWidth });
        }
        return ranges;
    };


    // 2. Render Master Track (Top)
    const masterContainer = document.createElement('div');
    masterContainer.className = 'timeline-row master-row';
    masterContainer.style.height = '80px';
    masterContainer.style.marginBottom = '2px';

    // Label
    const masterLabel = document.createElement('div');
    masterLabel.className = 'track-label sticky-label';
    masterLabel.innerHTML = '<span style="font-weight:900; color:#fff">MASTER</span>';
    masterLabel.style.borderLeft = '4px solid #fff'; // White indicator for Master
    masterLabel.style.backgroundColor = '#18181b'; // Match background to hide scroll
    masterContainer.appendChild(masterLabel);

    const masterTrack = document.createElement('div');
    masterTrack.className = 'track-content';
    masterContainer.appendChild(masterTrack);

    let leftOffset = 0;
    state.frames.forEach((frame, i) => {
        const width = (frame.duration / 1000) * PIXELS_PER_SEC;
        const div = document.createElement('div');
        div.className = `frame-block ${i === state.currentFrameIndex ? 'active' : ''}`;
        div.style.left = leftOffset + 'px';
        div.style.width = width + 'px';
        div.dataset.index = i;

        // Thumbnail (Lazy load or simplify for perf?)
        const thumbCanvas = document.createElement('canvas');
        thumbCanvas.className = 'frame-thumbnail';
        thumbCanvas.width = state.GRID_WIDTH;
        thumbCanvas.height = state.GRID_HEIGHT;
        const tCtx = thumbCanvas.getContext('2d');

        if (frame.isCacheDirty) frame.updateCache();
        tCtx.drawImage(frame.cacheCanvas, 0, 0);
        if (frame.shapes) {
            frame.shapes.forEach(s => s.draw(tCtx)); // Draw vector overlay
        }
        div.appendChild(thumbCanvas);

        // Frame Info
        const info = document.createElement('div');
        info.className = 'frame-info';
        let labelHtml = '';
        if (frame.label) {
            let colorClass = '#00d2ff';
            if (frame.label.includes('BLINK')) colorClass = '#fbbf24';
            labelHtml = `<div class="frame-label" style="font-size: 8px; color: ${colorClass}; margin-top: 1px; font-weight:900; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; max-width: 100%; text-transform: uppercase;">${frame.label}</div>`;
        }

        info.innerHTML = `
            <span class="frame-number">${i + 1}</span>
             ${labelHtml}
        `;
        div.appendChild(info);

        // Resize Handle
        const handle = document.createElement('div');
        handle.className = 'resize-handle';
        div.appendChild(handle);

        masterTrack.appendChild(div);
        leftOffset += width;
    });

    track.appendChild(masterContainer);

    // 3. Render Object Tracks (NLE Style)
    objectTracks.forEach((trackData, objId) => {
        const row = document.createElement('div');
        row.className = 'timeline-row';
        row.dataset.objectId = objId;

        // Label
        const label = document.createElement('div');
        label.className = 'track-label sticky-label';
        label.style.backgroundColor = '#121217'; // Dark panel bg to cover scrolling content

        // Icon logic
        let icon = 'Box';
        if (trackData.type === 'path') icon = 'Pentagon';
        if (trackData.type === 'text') icon = 'Type';
        if (trackData.type === 'image') icon = 'Image';
        if (trackData.type === 'ellipse') icon = 'Circle';

        // Active indicator (Multi-select)
        const isActive = state.selectedShapes.some(s => s.id == objId);
        if (isActive) {
            label.style.color = '#00d2ff';
            label.style.borderLeft = '4px solid #00d2ff';
            label.style.backgroundColor = '#1e1e24';
        } else {
            label.style.borderLeft = '4px solid transparent';
        }

        label.innerHTML = `
            <div style="display:flex; align-items:center; gap:6px; width:100%; overflow:hidden;">
                <i data-lucide="${icon.toLowerCase()}" style="width:10px; height:10px; flex-shrink:0 opacity:0.7"></i>
                <span title="${trackData.name}" style="flex:1; overflow:hidden; text-overflow:ellipsis;">${trackData.name}</span>
            </div>
            `;

        // Click label to select object
        label.onclick = (e) => {
            // Find the shape ref from current frame to select it
            const currentFrame = state.frames[state.currentFrameIndex];
            if (currentFrame) {
                const s = currentFrame.shapes.find(s => s.id == objId);
                if (s) {
                    if (e.ctrlKey || e.shiftKey) {
                        const idx = state.selectedShapes.findIndex(sel => sel.id == objId);
                        if (idx !== -1) {
                            state.selectedShapes.splice(idx, 1);
                        } else {
                            state.selectedShapes.push(s);
                        }
                        state.selectedShape = state.selectedShapes.length > 0 ? state.selectedShapes[state.selectedShapes.length - 1] : null;
                    } else {
                        state.selectedShapes = [s];
                        state.selectedShape = s;
                    }

                    if (state.actions.renderEditor) state.actions.renderEditor();
                    renderTimeline();
                }
            }
        };

        row.appendChild(label);

        // Track Content
        const content = document.createElement('div');
        content.className = 'track-content';

        // --- Draw Clips ---
        const ranges = calculateRanges(trackData.existsInFrames);

        ranges.forEach(range => {
            const clip = document.createElement('div');
            clip.className = 'timeline-clip';
            clip.style.left = range.x + 'px';
            clip.style.width = (range.w - 1) + 'px'; // -1 for gap

            // Interaction: Select Clip selects object
            clip.onclick = (e) => {
                e.stopPropagation();
                // Select object, move playhead to start of clip?
                state.currentFrameIndex = range.start;
                const frame = state.frames[state.currentFrameIndex];
                const shape = frame.shapes.find(s => s.id == objId);

                if (shape) {
                    if (e.ctrlKey || e.shiftKey) {
                        const idx = state.selectedShapes.findIndex(sel => sel.id == objId);
                        if (idx !== -1) state.selectedShapes.splice(idx, 1);
                        else state.selectedShapes.push(shape);
                        state.selectedShape = state.selectedShapes[state.selectedShapes.length - 1] || null;
                    } else {
                        state.selectedShapes = [shape];
                        state.selectedShape = shape;
                    }
                }

                if (state.actions.renderEditor) state.actions.renderEditor();
                if (state.actions.renderPreview) state.actions.renderPreview();
                updateFrameInfo();
                renderTimeline();
            };

            // Highlight active clip
            if (isActive && state.currentFrameIndex >= range.start && state.currentFrameIndex < (range.start + (range.ms / 100))) {
                // Approximate logic for "is current frame in this range" (ignoring variable FPS for a sec, logic is index based)
                // Actually safer:
                let isInRange = false;
                let frameCount = 0;
                let msCount = 0;
                for (let i = range.start; i < state.frames.length; i++) {
                    if (msCount >= range.ms) break;
                    if (i === state.currentFrameIndex) isInRange = true;
                    msCount += state.frames[i].duration;
                }

                if (isActive) {
                    clip.classList.add('selected');
                }
            }

            // Clip Name
            clip.innerHTML = `<span class="clip-name">${trackData.name}</span>`;

            content.appendChild(clip);
        });

        row.appendChild(content);
        track.appendChild(row);
    });

    // Re-init Icons because we injected HTML
    if (window.lucide) window.lucide.createIcons();

    // Update Playhead
    updatePlayheadPosition();
}


// Frame Operations
export function addFrame() {
    saveUndo();
    const newFrame = new Frame();
    // Copy pixels from current frame if exists
    if (state.frames.length > 0 && state.frames[state.currentFrameIndex]) {
        const prev = state.frames[state.currentFrameIndex];
        // Create blank or copy? Usually blank or copy last? 
        // Original script: creates blank.
    }
    state.frames.splice(state.currentFrameIndex + 1, 0, newFrame);
    state.currentFrameIndex++;
    renderTimeline();
    if (state.actions.renderEditor) state.actions.renderEditor();
}

export function duplicateFrame() {
    saveUndo();
    const current = state.frames[state.currentFrameIndex];
    if (!current) return;

    const clone = new Frame();
    restoreFrame(clone, serializeFrame(current)); // Copy data
    clone.id = Date.now() + Math.random();

    state.frames.splice(state.currentFrameIndex + 1, 0, clone);
    state.currentFrameIndex++;
    renderTimeline();
    if (state.actions.renderEditor) state.actions.renderEditor();
}

export function deleteFrame() {
    if (state.frames.length <= 1) return;
    saveUndo();
    state.frames.splice(state.currentFrameIndex, 1);
    if (state.currentFrameIndex >= state.frames.length) {
        state.currentFrameIndex = state.frames.length - 1;
    }
    renderTimeline();
    if (state.actions.renderEditor) state.actions.renderEditor();
}

