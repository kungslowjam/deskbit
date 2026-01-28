export const state = {
    // Canvas config
    GRID_WIDTH: 466,
    GRID_HEIGHT: 466,

    // Global Actions (Dependency Injection to avoid cycles)
    actions: {},

    // Project state
    projectStates: [],
    activeStateId: null,
    frames: [],
    currentFrameIndex: 0,

    // Editor state
    isDrawing: false,
    isPlaying: false,
    currentTool: 'pen',
    currentColor: '#00ffff',
    brushSize: 1,
    isSymmetryEnabled: false,
    lastCoords: null,

    // Selection & Clipboard
    selectedShape: null,
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

    // Playback Internals
    playTimerId: null,
    playStartTime: 0,
    playStartOffset: 0,
    totalDuration: 0,
    animFrameId: null
};
