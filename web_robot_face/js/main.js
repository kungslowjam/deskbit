import { state } from './state.js';
import { initRenderer, renderEditor, renderPreview, drawCurvePreview } from './renderer.js';

// Inject Dependencies
state.actions.renderEditor = renderEditor;
state.actions.renderPreview = renderPreview;
// History callback setup moved to init
import { initTimeline, renderTimeline, startPlay, stopPlay, addFrame, duplicateFrame, deleteFrame } from './timeline.js';
import { initTools, setupEventListeners, applyEyePreset } from './tools.js';
import { initRuler } from './timeline.js'; // Helper
import { updateFrameInfo, updateToolButtons, updateStatesPanel, toggleRound, setZoom, updateInteractionEditor, updateLayersPanel } from './ui.js';
import { undo, redo, saveUndo, setHistoryRefreshCallback } from './history.js';
import * as IO from './io.js';
import { PIXELS_PER_SEC } from './constants.js';
import { Frame } from './models.js';

// Global Resize Function
function resizeCanvas(newWidth, newHeight) {
    if (newWidth < 8 || newHeight < 8 || newWidth > 512 || newHeight > 512) {
        alert('Canvas size must be between 8 and 512'); // detailed toast in ui?
        return;
    }

    const oldWidth = state.GRID_WIDTH;
    const oldHeight = state.GRID_HEIGHT;

    state.GRID_WIDTH = newWidth;
    state.GRID_HEIGHT = newHeight;

    // Resize state.frames
    state.frames.forEach(frame => {
        const oldPixels = frame.pixels;
        frame.pixels = new Array(state.GRID_WIDTH * state.GRID_HEIGHT).fill(null);
        // Copy existing
        for (let y = 0; y < Math.min(oldHeight, state.GRID_HEIGHT); y++) {
            for (let x = 0; x < Math.min(oldWidth, state.GRID_WIDTH); x++) {
                const oldIdx = y * oldWidth + x;
                const newIdx = y * state.GRID_WIDTH + x;
                frame.pixels[newIdx] = oldPixels[oldIdx];
            }
        }
        // Resize Shapes if needed? Typically vector shapes stay absolute or relative?
        // Original script didn't move shapes.

        // Update cache canvas size
        frame.cacheCanvas.width = state.GRID_WIDTH;
        frame.cacheCanvas.height = state.GRID_HEIGHT;
        frame.isCacheDirty = true;
    });

    const canvas = document.getElementById('editor-canvas');
    const previewCanvas = document.getElementById('preview-canvas');

    if (canvas) { canvas.width = state.GRID_WIDTH; canvas.height = state.GRID_HEIGHT; }
    if (previewCanvas) { previewCanvas.width = state.GRID_WIDTH; previewCanvas.height = state.GRID_HEIGHT; }

    document.getElementById('canvas-width').value = state.GRID_WIDTH;
    document.getElementById('canvas-height').value = state.GRID_HEIGHT;

    if (state.GRID_WIDTH === 466 && state.GRID_HEIGHT === 466) {
        document.getElementById('round-toggle').checked = true;
        toggleRound(true);
    } else {
        document.getElementById('round-toggle').checked = false;
        toggleRound(false);
    }

    renderTimeline();
    renderEditor();
    renderPreview();
    // fitZoom(); // Implementation missing in ui.js? I'll use simple setZoom(1) logic or implement fitZoom.
    setZoom(1);
}

async function init() {
    try {
        console.log("Starting Init...");
        const canvas = document.getElementById('editor-canvas');
        const previewCanvas = document.getElementById('preview-canvas');

        if (!canvas) throw new Error("editor-canvas not found");
        if (!previewCanvas) throw new Error("preview-canvas not found");

        console.log("Initializing Sub-systems...");
        initRenderer(canvas, previewCanvas);
        initTimeline();
        initTools(canvas);

        console.log("Setting up History...");
        setHistoryRefreshCallback(() => {
            renderTimeline();
            renderEditor();
            updateLayersPanel();
        });

        console.log("Setting up Initial State...");
        state.frames = [new Frame()];
        if (state.projectStates.length === 0) {
            state.projectStates.push({ id: 'idle', name: 'Idle', frames: state.frames });
            state.activeStateId = 'idle';
        }

        console.log("Attaching Event Listeners...");
        // Toolbar
        document.querySelectorAll('.tool-btn').forEach(btn => {
            btn.onclick = () => {
                const id = btn.id;
                const tool = id.replace('tool-', '');
                state.currentTool = tool;
                updateToolButtons();
            };
        });

        document.getElementById('color-picker').addEventListener('input', (e) => {
            state.currentColor = e.target.value;
        });

        // Actions
        document.getElementById('action-undo')?.addEventListener('click', undo);
        document.getElementById('action-redo')?.addEventListener('click', redo);
        document.getElementById('action-clear')?.addEventListener('click', () => {
            saveUndo();
            const frame = state.frames[state.currentFrameIndex];
            frame.pixels.fill(null);
            frame.shapes = [];
            frame.isCacheDirty = true;
            renderEditor();
        });

        // Timeline Controls
        document.getElementById('play-btn')?.addEventListener('click', startPlay);
        document.getElementById('stop-btn')?.addEventListener('click', stopPlay);
        document.getElementById('add-frame-btn')?.addEventListener('click', addFrame);
        document.getElementById('dup-frame-btn')?.addEventListener('click', duplicateFrame);
        document.getElementById('del-frame-btn')?.addEventListener('click', deleteFrame);

        document.getElementById('fps-range')?.addEventListener('input', (e) => {
            document.getElementById('fps-value').textContent = e.target.value + ' FPS';
        });

        // Zoom
        document.getElementById('zoom-in-btn')?.addEventListener('click', () => setZoom(state.zoomLevel + 0.1));
        document.getElementById('zoom-out-btn')?.addEventListener('click', () => setZoom(state.zoomLevel - 0.1));
        document.getElementById('zoom-reset-btn')?.addEventListener('click', () => setZoom(1));

        // IO
        document.getElementById('export-c-btn')?.addEventListener('click', IO.exportToC);
        document.getElementById('export-lvgl-native-btn')?.addEventListener('click', IO.exportToLVGLNative);
        document.getElementById('export-btn')?.addEventListener('click', IO.exportToJSON);
        document.getElementById('import-btn')?.addEventListener('click', () => IO.importFromJSON(resizeCanvas, () => {
            renderTimeline();
            renderEditor();
            updateFrameInfo();
            updateStatesPanel(switchState, deleteState);
        }));
        console.log("Exposing IO Functions...");
        // Expose to window for direct HTML access (onclick)
        window.sendToRobot = IO.sendToRobot;
        window.deleteFromRobot = IO.deleteFromRobot;

        // Round Toggle
        document.getElementById('round-toggle')?.addEventListener('change', (e) => toggleRound(e.target.checked));

        // Canvas Size Inputs
        const wInput = document.getElementById('canvas-width');
        const hInput = document.getElementById('canvas-height');
        const onResize = () => resizeCanvas(parseInt(wInput.value), parseInt(hInput.value));
        wInput?.addEventListener('change', onResize);
        hInput?.addEventListener('change', onResize);

        // Expose for HTML OnClick
        window.applyEyePreset = applyEyePreset;

        // Shape/Layer Actions
        const toolImport = () => import('./tools.js');
        document.getElementById('action-duplicate')?.addEventListener('click', () => toolImport().then(m => m.duplicateShape()));
        document.getElementById('action-delete-shape')?.addEventListener('click', () => toolImport().then(m => m.deleteShape()));
        document.getElementById('action-scale-up')?.addEventListener('click', () => toolImport().then(m => m.scaleShape(1.1)));
        document.getElementById('action-scale-down')?.addEventListener('click', () => toolImport().then(m => m.scaleShape(0.9)));

        document.getElementById('align-left')?.addEventListener('click', () => toolImport().then(m => m.alignShapes('left')));
        document.getElementById('align-center-h')?.addEventListener('click', () => toolImport().then(m => m.alignShapes('center-h')));
        document.getElementById('align-right')?.addEventListener('click', () => toolImport().then(m => m.alignShapes('right')));
        document.getElementById('align-top')?.addEventListener('click', () => toolImport().then(m => m.alignShapes('top')));
        document.getElementById('align-center-v')?.addEventListener('click', () => toolImport().then(m => m.alignShapes('center-v')));
        document.getElementById('align-bottom')?.addEventListener('click', () => toolImport().then(m => m.alignShapes('bottom')));

        document.getElementById('layer-up-btn')?.addEventListener('click', () => toolImport().then(m => m.moveLayerUp()));
        document.getElementById('layer-down-btn')?.addEventListener('click', () => toolImport().then(m => m.moveLayerDown()));
        document.getElementById('layer-del-btn')?.addEventListener('click', () => toolImport().then(m => m.deleteShape()));

        console.log("Performing Initial Render...");
        resizeCanvas(466, 466);
        updateToolButtons();
        updateFrameInfo();
        updateStatesPanel(switchState, deleteState);

        // Remove loading overlay
        const loader = document.getElementById('loader');
        if (loader) loader.style.display = 'none';

        if (window.lucide) window.lucide.createIcons();

        console.log("Init Complete.");

    } catch (e) {
        console.error("CRITICAL INIT ERROR:", e);
        alert("CRITICAL INIT ERROR:\n" + e.message + "\nCheck console for details.");
    }
}

// State Management Helpers (Hoisted for use in init's callbacks)
function switchState(id) {
    if (state.activeStateId === id) return;
    state.activeStateId = id;
    const s = state.projectStates.find(x => x.id === id);
    if (s) {
        state.frames = s.frames;
        state.currentFrameIndex = 0;
        document.getElementById('anim-name').value = s.name;
        renderTimeline();
        renderEditor();
        updateStatesPanel(switchState, deleteState);
    }
}

function deleteState(id) {
    if (state.projectStates.length <= 1) return;
    const idx = state.projectStates.findIndex(s => s.id === id);
    if (idx > -1) {
        state.projectStates.splice(idx, 1);
        if (state.activeStateId === id) switchState(state.projectStates[0].id);
        else updateStatesPanel(switchState, deleteState);
    }
}

// Start
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', () => {
        try { init(); }
        catch (e) { alert('Startup Error: ' + e); console.error(e); }
        finally { if (window.lucide) window.lucide.createIcons(); }
    });
} else {
    try { init(); }
    catch (e) { alert('Startup Error: ' + e); console.error(e); }
    finally { if (window.lucide) window.lucide.createIcons(); }
}
