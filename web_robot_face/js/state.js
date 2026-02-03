export const state = {
    // Canvas config
    GRID_WIDTH: 466,
    GRID_HEIGHT: 466,

    // Global Actions (Dependency Injection to avoid cycles)
    actions: {},

    // Data
    frames: [], // Array of Frame objects
    currentFrameIndex: 0,
    projectStates: [], // Array of State objects { id, name, frames, ... }
    activeStateId: null, // ID of the currently active state (e.g. 'neutral', 'happy')
    EYE_PATHS: {}, // Placeholder, populated from presets.js usually but needed for safe access

    // Playback
    isPlaying: false,
    currentTool: 'pen',
    currentColor: '#00ffff',
    brushSize: 1,
    isSymmetryEnabled: false,
    lastCoords: null,

    // Editor state
    isDrawing: false,

    // Shape Defaults
    shapeDefaults: {
        opacity: 1,
        blendMode: 'source-over',
        strokeWidth: 0,
        strokeColor: '#ffffff',
        cornerRadius: 0,
        polygonSides: 5
    },

    // Selection & Clipboard
    selectedShape: null, // Primary selection (for property editor)
    selectedShapes: [],  // All selected shapes (for multi-select actions)
    copiedFrameData: null,
    selectedFrames: [],
    clipboard: [],
    clipboardMode: null,
    shapeClipboard: null,

    // Files
    localFiles: {},
    currentFileName: 'default_anim',

    // Zoom
    zoomLevel: 1,

    // History
    undoStack: [],
    redoStack: [],

    // Onion Skin
    onionSkinEnabled: false,
    onionSkinOpacity: 0.3,

    // Timeline Interaction
    isResizingFrame: false,
    resizeFrameIndex: -1,
    resizeStartX: 0,
    resizeStartDuration: 0,

    // Playback state shared
    isPlaying: false
};
