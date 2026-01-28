import { state } from './state.js';
import { Frame, Shape, TextShape } from './models.js';
// Imports fixed to avoid circular deps and incorrect paths
import { updateLayersPanel } from './ui.js';

// Since renderTimeline is in timeline.js and history needs it for undo/redo...
// We will assign it later or import it.
// Ideally, Undo/Redo should just update state and trigger a generalized "Refresh" event.
// For now, allow injecting a refresh callback.

let refreshCallback = null;

export function setHistoryRefreshCallback(cb) {
    refreshCallback = cb;
}

export function serializeFrame(frame) {
    return {
        duration: frame.duration,
        pixels: [...frame.pixels],
        shapes: frame.shapes.map(s => s.clone(true))
    };
}

export function restoreFrame(frame, data) {
    frame.duration = data.duration;
    frame.pixels = [...data.pixels];
    frame.shapes = data.shapes.map(s => s.clone(true));
    frame.isCacheDirty = true;
}

export function saveUndo() {
    const frame = state.frames[state.currentFrameIndex];
    if (!frame) return;
    state.undoStack.push(serializeFrame(frame));
    if (state.undoStack.length > 50) state.undoStack.shift();
    state.redoStack = []; // Clear redo on new action
}

export function undo() {
    if (state.undoStack.length === 0) return;
    const frame = state.frames[state.currentFrameIndex];
    if (!frame) return;

    state.redoStack.push(serializeFrame(frame));
    const prev = state.undoStack.pop();
    restoreFrame(frame, prev);

    if (refreshCallback) refreshCallback();
}

export function redo() {
    if (state.redoStack.length === 0) return;
    const frame = state.frames[state.currentFrameIndex];
    if (!frame) return;

    state.undoStack.push(serializeFrame(frame));
    const next = state.redoStack.pop();
    restoreFrame(frame, next);

    if (refreshCallback) refreshCallback();
}
