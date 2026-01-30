import { state } from './state.js';
import { applyEasing } from './utils.js';
import { updatePlayheadTime, updateFrameInfo } from './ui.js';
import { PIXELS_PER_SEC } from './constants.js';

let playbackState = {
    isPlaying: false,
    startTime: 0,
    startOffset: 0,
    totalDuration: 0,
    animFrameId: null
};

// DOM Cache
const playhead = document.getElementById('playhead');
const playBtn = document.getElementById('play-btn');
const stopBtn = document.getElementById('stop-btn');

export function startPlayback() {
    if (playbackState.isPlaying) return;

    playbackState.isPlaying = true;
    state.isPlaying = true; // Sync for other modules if needed, but strive to remove dependency

    playbackState.startTime = performance.now();

    // Calculate start offset based on current frame
    let offset = 0;
    for (let i = 0; i < state.currentFrameIndex; i++) {
        offset += parseInt(state.frames[i].duration);
    }
    playbackState.startOffset = offset;
    playbackState.totalDuration = state.frames.reduce((sum, f) => sum + parseInt(f.duration), 0);

    if (playBtn) playBtn.classList.add('hidden');
    if (stopBtn) stopBtn.classList.remove('hidden');

    playbackState.animFrameId = requestAnimationFrame(animateLoop);
}

export function stopPlayback() {
    playbackState.isPlaying = false;
    state.isPlaying = false;

    if (playbackState.animFrameId) {
        cancelAnimationFrame(playbackState.animFrameId);
        playbackState.animFrameId = null;
    }

    if (playBtn) playBtn.classList.remove('hidden');
    if (stopBtn) stopBtn.classList.add('hidden');

    // Update UI one last time to snap
    updatePlayheadPosition();
}

export function togglePlayback() {
    if (playbackState.isPlaying) stopPlayback();
    else startPlayback();
}

function animateLoop(now) {
    if (!playbackState.isPlaying) return;

    const elapsed = now - playbackState.startTime + playbackState.startOffset;
    let t = elapsed;

    // Loop Logic
    const loopMode = document.getElementById('loop-mode')?.value || 'loop';

    if (t >= playbackState.totalDuration) {
        if (loopMode === 'once') {
            stopPlayback();
            state.currentFrameIndex = state.frames.length - 1;
            if (state.actions.renderEditor) state.actions.renderEditor();
            return;
        } else if (loopMode === 'loop') {
            t = t % playbackState.totalDuration;
            // Adjust start time to simulate loop
            playbackState.startTime = now - (t - playbackState.startOffset); // This logic might need refinement if startOffset was non-zero
            // Simplified: Treat as new start from 0? 
            // Better: 
            playbackState.startTime = now - t;
            playbackState.startOffset = 0; // Reset offset for subsequent loops
        }
    }

    // Find Frame (Linear Search - Optimize later if needed)
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

    // Update State
    if (state.currentFrameIndex !== foundIndex) {
        state.currentFrameIndex = foundIndex;
        // Highlight active block
        document.querySelectorAll('.frame-block').forEach((el, i) => {
            if (i === foundIndex) el.classList.add('active');
            else el.classList.remove('active');
        });
    }

    // Interpolation
    const easingMode = document.getElementById('easing-mode')?.value || 'linear';
    const easedT = applyEasing(localT, easingMode);

    const nextIndex = (state.currentFrameIndex + 1) % state.frames.length;
    const nextFrame = state.frames[nextIndex];

    // Render Preview
    if (state.actions.renderPreview) {
        state.actions.renderPreview(state.currentFrameIndex, { nextFrame, t: easedT });
    }

    // Update UI
    if (playhead) playhead.style.left = ((t / 1000) * PIXELS_PER_SEC) + 'px';
    updatePlayheadTime(t);

    playbackState.animFrameId = requestAnimationFrame(animateLoop);
}

export function updatePlayheadPosition() {
    if (!playhead) return;

    let timeMs = 0;
    if (playbackState.isPlaying) {
        return; // Handled by loop
    }

    for (let i = 0; i < state.currentFrameIndex; i++) {
        timeMs += parseInt(state.frames[i].duration);
    }
    const ratio = timeMs / 1000;
    playhead.style.left = (ratio * PIXELS_PER_SEC) + 'px';
    updatePlayheadTime(timeMs);
}
