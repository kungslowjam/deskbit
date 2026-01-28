import { state } from './state.js';
import { showToast } from './ui.js';
import { Frame, Shape, TextShape } from './models.js';

export async function saveFile(content, filename, mimeType) {
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

export async function exportToJSON() {
    let name = document.getElementById('anim-name')?.value || 'animation';
    name = name.replace(/[^a-zA-Z0-9_-]/g, '_');

    if (state.activeStateId) {
        const current = state.projectStates.find(s => s.id === state.activeStateId);
        if (current) current.frames = [...state.frames];
    }

    const data = {
        version: "1.1",
        width: state.GRID_WIDTH,
        height: state.GRID_HEIGHT,
        fps: parseInt(document.getElementById('fps-range')?.value || 12),
        easingMode: document.getElementById('easing-mode')?.value || 'linear',
        activeStateId: state.activeStateId,
        states: state.projectStates.map(st => ({
            id: st.id,
            name: st.name,
            frames: st.frames.map((frame, index) => ({
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

    await saveFile(JSON.stringify(data, null, 2), `${name}.json`, 'application/json');
}

export async function importFromJSON(resizeCanvasCallback, updateUIHelpers) {
    const input = document.createElement('input');
    input.type = 'file';
    input.accept = '.json';

    input.onchange = async (e) => {
        const file = e.target.files[0];
        if (!file) return;

        try {
            const text = await file.text();
            const data = JSON.parse(text);

            // Restore Global Settings
            if (data.width && data.height) {
                if (resizeCanvasCallback) resizeCanvasCallback(data.width, data.height);
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
                    easingSelect.dispatchEvent(new Event('change'));
                }
            }

            if (data.states && Array.isArray(data.states)) {
                state.projectStates = data.states.map(stateData => ({
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
                                // Map properties back
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

                                if (shape.type === 'path') shape.parsePath();
                                return shape;
                            });
                        }
                        return frame;
                    })
                }));
                state.activeStateId = data.activeStateId || state.projectStates[0].id;
            } else if (data.frames) {
                // Legacy v1.0
                const framesList = data.frames.map(frameData => {
                    const frame = new Frame();
                    return frame;
                });
                state.projectStates = [{ id: 'idle', name: 'Idle', frames: framesList }];
                state.activeStateId = 'idle';
            }

            // Set active frames
            const activeState = state.projectStates.find(s => s.id === state.activeStateId);
            if (activeState) {
                state.frames = activeState.frames;
                state.currentFrameIndex = 0;
            }

            if (updateUIHelpers) updateUIHelpers();

            showToast(`Loaded ${state.projectStates.length} states`);
        } catch (err) {
            console.error(err);
            showToast('Failed to load file');
        }
    };
    input.click();
}

export async function exportToC() {
    let name = document.getElementById('anim-name')?.value || 'my_anim';
    name = name.replace(/[^a-zA-Z0-9_]/g, '_').toLowerCase();

    let cContent = `// Generated by Robot Face Studio\n#include "lvgl/lvgl.h"\n\n`;
    cContent += `const lv_img_dsc_t ${name} = { 0 }; // Placeholder\n`;

    await saveFile(cContent, `${name}.c`, 'text/x-c');
}

export async function sendToRobot() {
    let name = document.getElementById('anim-name')?.value || 'my_anim';
    name = name.replace(/[^a-zA-Z0-9_]/g, '_').toLowerCase();

    if (state.activeStateId) {
        const current = state.projectStates.find(s => s.id === state.activeStateId);
        if (current) current.frames = [...state.frames];
    }

    // Construct payload matching script.js
    const payload = {
        name,
        width: state.GRID_WIDTH,
        height: state.GRID_HEIGHT,
        fps: parseInt(document.getElementById('fps-range')?.value || 12),
        activeStateId: state.activeStateId,
        states: state.projectStates.map(st => ({
            id: st.id,
            name: st.name,
            frames: st.frames.map(f => ({
                pixels: f.pixels.map((c, i) => c ? { i, c } : null).filter(p => p),
                duration: f.duration,
                shapes: f.shapes ? f.shapes.map(s => ({ ...s })) : [] // Deep copy properties
            }))
        }))
    };

    const API_BASE = window.location.protocol === 'file:' ? 'http://localhost:8000' : '';
    try {
        const res = await fetch(`${API_BASE}/save-anim`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
        });
        if (res.ok) showToast('✅ Saved! Run "idf.py build"');
        else showToast('❌ Error: Bridge not running?');
    } catch (e) {
        showToast('❌ Failed! Is bridge server running?');
    }
}

export async function deleteFromRobot() {
    alert("Delete button triggered!"); // DEBUG
    console.log("Delete action triggered");
    let name = document.getElementById('anim-name')?.value;
    if (!name) {
        showToast('⚠️ Enter animation name first');
        return;
    }
    name = name.replace(/[^a-zA-Z0-9_]/g, '_').toLowerCase();

    console.log("Confirming delete for:", name);
    if (!confirm(`Confirm delete '${name}' from Robot Project?`)) return;

    showToast(`🗑️ Deleting ${name}...`);
    const API_BASE = window.location.protocol === 'file:' ? 'http://localhost:8000' : '';

    try {
        const response = await fetch(`${API_BASE}/delete-anim`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ name: name })
        });
        if (response.ok) {
            showToast(`✅ Deleted ${name}! Run "idf.py build" to update.`);
        } else {
            showToast('❌ Error: Start "run_bridge.bat" first!');
        }
    } catch (e) {
        showToast('❌ Failed! Is bridge server running?');
    }
}

export async function exportToLVGLNative() {
    let baseName = document.getElementById('anim-name')?.value || 'my_anim';
    baseName = baseName.replace(/[^a-zA-Z0-9_]/g, '_').toLowerCase();

    // Cache current frames to active state
    if (state.activeStateId) {
        const current = state.projectStates.find(s => s.id === state.activeStateId);
        if (current) current.frames = [...state.frames];
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
    state.projectStates.forEach(projState => {
        cContent += `// --- State: ${projState.name} ---\n`;
        cContent += `static void create_state_${projState.id}(lv_obj_t * parent) {\n`;

        const stateFrames = projState.frames;
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
                cContent += `    lv_label_set_text(${varName}, "${(s.text || '').replace(/"/g, '\\"')}");\n`;
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
            if (s.rotation !== 0) {
                cContent += `    lv_obj_set_style_transform_rotation(${varName}, ${Math.round(s.rotation * 10)}, 0);\n`;
            }
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
                        { k: 'rotation', cb: 'lv_obj_set_style_transform_rotation', s: 10 },
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
    state.projectStates.forEach((projState, i) => {
        const cond = i === 0 ? `if` : `else if`;
        cContent += `    ${cond} (strcmp(state_id, "${projState.id}") == 0) create_state_${projState.id}(face_container);\n`;
    });
    cContent += `}\n\n`;

    cContent += `void init_${baseName}(lv_obj_t * parent) {\n`;
    cContent += `    face_container = lv_obj_create(parent);\n`;
    cContent += `    lv_obj_set_size(face_container, LV_PCT(100), LV_PCT(100));\n`;
    cContent += `    lv_obj_set_style_bg_opa(face_container, 0, 0);\n`;
    cContent += `    lv_obj_set_style_border_width(face_container, 0, 0);\n`;
    cContent += `    ${baseName}_switch_state("${state.activeStateId || state.projectStates[0].id}");\n`;
    cContent += `}\n`;

    await saveFile(cContent, `${baseName}_native.c`, 'text/x-c');
    showToast(`✅ Exported State Machine: ${baseName}_native.c`);
}
