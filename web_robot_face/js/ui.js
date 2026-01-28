import { state } from './state.js';
import { PIXELS_PER_SEC } from './constants.js';
// renderer imports removed to prevent circular dependency

// DOM Elements Helpers
const getEl = (id) => document.getElementById(id);

export function showToast(msg) {
    let container = getEl('toast-container');
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

export function updateToolButtons() {
    document.querySelectorAll('.tool-btn').forEach(btn => {
        btn.classList.remove('bg-white/20', 'text-accent');
        if (btn.id === `tool-${state.currentTool}`) {
            btn.classList.add('bg-white/20', 'text-accent');
        } else if (btn.id === `tool-${state.currentTool.replace('-obj', '')}-obj`) {
            btn.classList.add('bg-white/20', 'text-accent');
        }
    });
}

export function updateFrameInfo() {
    if (!state.frames || state.frames.length === 0) return;
    const frame = state.frames[state.currentFrameIndex];
    if (!frame) return;

    const indexEl = getEl('frame-index');
    if (indexEl) indexEl.textContent = state.currentFrameIndex + 1;

    const durationInput = getEl('frame-duration');
    if (durationInput) {
        durationInput.value = frame.duration;
        durationInput.style.color = '#00d2ff';
    }

    const totalMs = state.frames.reduce((sum, f) => sum + (parseInt(f.duration) || 100), 0);
    const totalTimeEl = getEl('total-time');
    if (totalTimeEl) totalTimeEl.textContent = (totalMs / 1000).toFixed(2) + 's';

    if (!state.isPlaying) {
        let timeMs = 0;
        for (let i = 0; i < state.currentFrameIndex; i++) {
            timeMs += parseInt(state.frames[i].duration) || 100;
        }
        updatePlayheadTime(timeMs);
    }

    updateLayersPanel();
}

export function updatePlayheadTime(timeMs) {
    const playheadTimeEl = getEl('playhead-time');
    if (playheadTimeEl) {
        const mins = Math.floor(timeMs / 60000);
        const secs = Math.floor((timeMs % 60000) / 1000);
        const ms = Math.floor(timeMs % 1000);
        playheadTimeEl.textContent = `${mins.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}.${ms.toString().padStart(3, '0')}`;
    }
}

export function updateLayersPanel() {
    const listEl = getEl('layers-list');
    if (!listEl) return;

    const shapes = state.frames[state.currentFrameIndex]?.shapes || [];

    if (shapes.length === 0) {
        listEl.innerHTML = '<div class="layer-item"><span class="layer-icon">○</span><span class="layer-name">No shapes</span></div>';
        return;
    }

    listEl.innerHTML = shapes.map((shape, i) => {
        const icon = shape.type === 'ellipse' ? '○' :
            shape.type === 'rect' ? '▢' :
                shape.type === 'line' ? '╱' :
                    shape.type === 'text' ? 'T' : '?';
        const isActive = state.selectedShape && state.selectedShape.id === shape.id;
        return `<div class="layer-item ${isActive ? 'active' : ''}" data-index="${i}">
            <span class="layer-icon">${icon}</span>
            <span class="layer-name">${shape.type} ${i + 1}</span>
            <span class="layer-visibility">👁</span>
        </div>`;
    }).reverse().join('');

    listEl.querySelectorAll('.layer-item').forEach(item => {
        item.onclick = () => {
            const idx = parseInt(item.dataset.index);
            state.selectedShape = state.frames[state.currentFrameIndex].shapes[idx];
            if (state.actions.renderEditor) state.actions.renderEditor();
            updateLayersPanel();
        };
    });
}

export function updateStatesPanel(switchStateCallback, deleteStateCallback) {
    const listEl = getEl('states-list');
    if (!listEl) return;

    if (state.projectStates.length === 0) {
        listEl.innerHTML = `
            <div class="text-[10px] text-zinc-600 font-bold text-center py-4 bg-black/20 rounded-lg border border-white/5 border-dashed">
                No states defined
            </div>`;
        return;
    }

    listEl.innerHTML = state.projectStates.map(s => {
        const isActive = state.activeStateId === s.id;
        return `
            <div class="flex items-center gap-2 p-2 rounded-lg cursor-pointer transition-all ${isActive ? 'bg-accent/20 border border-accent/20' : 'bg-white/5 hover:bg-white/10'}" 
                 data-id="${s.id}">
                <div class="w-1.5 h-1.5 rounded-full ${isActive ? 'bg-accent animate-pulse' : 'bg-zinc-600'}"></div>
                <span class="text-[11px] font-bold flex-1 ${isActive ? 'text-accent' : 'text-zinc-400'}">${s.name}</span>
                <button class="delete-state-btn p-1.5 text-zinc-600 hover:text-red-500 transition-colors" data-id="${s.id}">
                    <i data-lucide="x" class="w-3 h-3"></i>
                </button>
            </div>
        `;
    }).join('');

    if (window.lucide) window.lucide.createIcons();

    // Attach Listeners
    listEl.querySelectorAll('[data-id]').forEach(el => {
        el.onclick = (e) => {
            // Prevent triggering if clicked on delete button
            if (e.target.closest('.delete-state-btn')) return;
            switchStateCallback(el.dataset.id);
        };
    });

    listEl.querySelectorAll('.delete-state-btn').forEach(btn => {
        btn.onclick = (e) => {
            e.stopPropagation();
            deleteStateCallback(btn.dataset.id);
        };
    });

    // Update next-state dropdowns
    const nextStateSelect = getEl('trigger-next-state');
    if (nextStateSelect) {
        nextStateSelect.innerHTML = state.projectStates.map(s =>
            `<option value="${s.id}">${s.name}</option>`
        ).join('');
    }
}

export function updateInteractionEditor() {
    const editor = getEl('interaction-editor');
    const shapeExtras = getEl('shape-extras');

    if (state.currentTool === 'select' && state.selectedShape) {
        if (editor) editor.classList.remove('hidden');

        const triggerSelect = getEl('trigger-type');
        const nextStateSelect = getEl('trigger-next-state');

        if (state.selectedShape.interaction) {
            if (triggerSelect) triggerSelect.value = state.selectedShape.interaction.trigger || 'click';
            if (nextStateSelect) nextStateSelect.value = state.selectedShape.interaction.targetStateId || '';
        } else {
            if (triggerSelect) triggerSelect.value = 'click';
        }

        if (shapeExtras) {
            shapeExtras.classList.remove('hidden');
            const radProp = getEl('prop-corner-radius');
            if (radProp) {
                if (state.selectedShape.type === 'rect') {
                    radProp.classList.remove('hidden');
                    getEl('corner-radius-slider').value = state.selectedShape.cornerRadius || 0;
                    getEl('corner-radius-val').innerText = (state.selectedShape.cornerRadius || 0) + 'px';
                } else {
                    radProp.classList.add('hidden');
                }
            }
            getEl('stroke-width-slider').value = state.selectedShape.strokeWidth || 0;
            getEl('stroke-width-val').innerText = (state.selectedShape.strokeWidth || 0) + 'px';
            getEl('stroke-color-picker').value = state.selectedShape.strokeColor || '#ffffff';
        }
    } else {
        if (editor) editor.classList.add('hidden');
        if (shapeExtras) shapeExtras.classList.add('hidden');
    }
}

export function toggleRound(isRound) {
    const wrapper = getEl('canvas-wrapper');
    const previewContainer = getEl('preview-container');

    if (wrapper) {
        isRound ? wrapper.classList.add('canvas-round') : wrapper.classList.remove('canvas-round');
    }
    if (previewContainer) {
        isRound ? previewContainer.classList.add('canvas-round') : previewContainer.classList.remove('canvas-round');
    }
}

export function setZoom(level) {
    state.zoomLevel = Math.max(0.25, Math.min(8, level));
    const wrapper = getEl('canvas-wrapper');
    if (wrapper) {
        wrapper.style.transform = state.zoomLevel === 1 ? 'none' : `scale(${state.zoomLevel})`;
        wrapper.style.transformOrigin = 'center center';
    }
    const zoomLevelEl = getEl('zoom-level');
    if (zoomLevelEl) zoomLevelEl.innerText = Math.round(state.zoomLevel * 100) + '%';
}
