import { state } from './state.js';
import { Frame } from './models.js';
// renderer imports removed to prevent circular dependency
import { updateFrameInfo, updatePlayheadTime } from './ui.js';
import { saveUndo, serializeFrame, restoreFrame } from './history.js';
import { applyEasing } from './utils.js';
import { PIXELS_PER_SEC } from './constants.js';

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

export function renderTimeline() {
    if (!track) return;
    track.innerHTML = '';

    let leftOffset = 0;
    state.frames.forEach((frame, i) => {
        const width = (frame.duration / 1000) * PIXELS_PER_SEC;
        const div = document.createElement('div');
        div.className = `frame-block ${i === state.currentFrameIndex ? 'active' : ''}`;
        div.style.left = leftOffset + 'px';
        div.style.width = width + 'px';
        div.dataset.index = i;

        // Thumbnail
        const thumbCanvas = document.createElement('canvas');
        thumbCanvas.className = 'frame-thumbnail';
        thumbCanvas.width = state.GRID_WIDTH;
        thumbCanvas.height = state.GRID_HEIGHT;
        const tCtx = thumbCanvas.getContext('2d');

        // Use cached frame canvas if clean
        if (frame.isCacheDirty) frame.updateCache();
        tCtx.drawImage(frame.cacheCanvas, 0, 0);

        // Draw shapes on thumbnail
        if (frame.shapes) {
            frame.shapes.forEach(s => s.draw(tCtx));
        }

        div.appendChild(thumbCanvas);

        // Frame Info Overlay
        const info = document.createElement('div');
        info.className = 'frame-info';
        info.innerHTML = `
            <span class="frame-number">${i + 1}</span>
            <span class="frame-duration">${frame.duration}ms</span>
        `;
        div.appendChild(info);

        // Handle
        const handle = document.createElement('div');
        handle.className = 'resize-handle';
        div.appendChild(handle);

        track.appendChild(div);
        leftOffset += width;
    });

    // Update Playhead
    updatePlayheadPosition();
}

export function updatePlayheadPosition() {
    if (!playhead) return;

    let timeMs = 0;
    if (state.isPlaying) {
        // Calculated in animate loop
        return;
    }

    for (let i = 0; i < state.currentFrameIndex; i++) {
        timeMs += parseInt(state.frames[i].duration);
    }
    const ratio = timeMs / 1000;
    playhead.style.left = (ratio * PIXELS_PER_SEC) + 'px';
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

// Playback Logic
export function startPlay() {
    if (state.isPlaying) return;
    state.isPlaying = true;
    state.playStartTime = performance.now();

    // Calculate start offset based on current frame
    let offset = 0;
    for (let i = 0; i < state.currentFrameIndex; i++) {
        offset += parseInt(state.frames[i].duration);
    }
    state.playStartOffset = offset;

    state.totalDuration = state.frames.reduce((sum, f) => sum + parseInt(f.duration), 0);

    document.getElementById('play-btn').classList.add('hidden');
    document.getElementById('stop-btn').classList.remove('hidden');

    state.animFrameId = requestAnimationFrame(animate);
}

export function stopPlay() {
    state.isPlaying = false;
    cancelAnimationFrame(state.animFrameId);
    document.getElementById('play-btn').classList.remove('hidden');
    document.getElementById('stop-btn').classList.add('hidden');
    updatePlayheadPosition();
}

function animate(now) {
    if (!state.isPlaying) return;

    const elapsed = now - state.playStartTime + state.playStartOffset;
    let t = elapsed;

    // Loop
    const loopMode = document.getElementById('loop-mode')?.value || 'loop';
    if (t >= state.totalDuration) {
        if (loopMode === 'once') {
            stopPlay();
            state.currentFrameIndex = state.frames.length - 1;
            if (state.actions.renderEditor) state.actions.renderEditor();
            return;
        } else if (loopMode === 'loop') {
            t = t % state.totalDuration;
            state.playStartTime = now - (t - state.playStartOffset); // Reset base
        } else if (loopMode === 'pingpong') {
            // Complex logic omitted for brevity in plan, strictly assuming loop/once for now or basic impl
            t = t % state.totalDuration;
        }
    }

    // Find Frame
    let accum = 0;
    let foundIndex = 0;
    let localT = 0; // 0..1 within frame

    for (let i = 0; i < state.frames.length; i++) {
        const dur = parseInt(state.frames[i].duration);
        if (t < accum + dur) {
            foundIndex = i;
            localT = (t - accum) / dur;
            break;
        }
        accum += dur;
    }

    state.currentFrameIndex = foundIndex;

    // Interpolation Setup
    const easingMode = document.getElementById('easing-mode')?.value || 'linear';
    const easedT = applyEasing(localT, easingMode);

    const nextIndex = (state.currentFrameIndex + 1) % state.frames.length;
    const nextFrame = state.frames[nextIndex];

    // Pass interpolation to renderPreview
    if (state.actions.renderPreview) state.actions.renderPreview(state.currentFrameIndex, { nextFrame, t: easedT });

    // Update Playhead Visual
    if (playhead) playhead.style.left = ((t / 1000) * PIXELS_PER_SEC) + 'px';
    updatePlayheadTime(t); // UI update

    // Highlight active block
    document.querySelectorAll('.frame-block').forEach((el, i) => {
        if (i === state.currentFrameIndex) el.classList.add('active');
        else el.classList.remove('active');
    });

    state.animFrameId = requestAnimationFrame(animate);
}
