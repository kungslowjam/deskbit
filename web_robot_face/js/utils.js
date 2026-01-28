import { state } from './state.js';

export function lerp(start, end, t) {
    if (start === undefined || end === undefined) return start ?? end;
    if (isNaN(start) || isNaN(end)) return start !== undefined ? start : 0;
    return start + (end - start) * t;
}

export function lerpColor(c1, c2, t) {
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

export function rgbToHex(rgb) {
    if (rgb.startsWith('#')) return rgb;
    const result = rgb.match(/\d+/g);
    if (!result || result.length < 3) return rgb;
    return '#' + result.slice(0, 3).map(x => parseInt(x).toString(16).padStart(2, '0')).join('');
}

export function getPixelIndex(x, y) {
    if (x < 0 || x >= state.GRID_WIDTH || y < 0 || y >= state.GRID_HEIGHT) return -1;
    return y * state.GRID_WIDTH + x;
}

export function getCoordsFromEvent(e, canvas) {
    const rect = canvas.getBoundingClientRect();
    // Default to handling scale if canvas CSS size != attribute size
    const scaleX = canvas.width / rect.width;
    const scaleY = canvas.height / rect.height;

    let x = (e.clientX - rect.left) * scaleX;
    let y = (e.clientY - rect.top) * scaleY;

    x = Math.max(0, Math.min(state.GRID_WIDTH - 1, Math.floor(x)));
    y = Math.max(0, Math.min(state.GRID_HEIGHT - 1, Math.floor(y)));

    return { x, y };
}

// Cubic Bezier for Easing
export function getCubicBezier(t, p1x, p1y, p2x, p2y) {
    const cx = 3 * p1x;
    const bx = 3 * (p2x - p1x) - cx;
    const ax = 1 - cx - bx;
    const cy = 3 * p1y;
    const by = 3 * (p2y - p1y) - cy;
    const ay = 1 - cy - by;

    const sampleCurveX = (t) => ((ax * t + bx) * t + cx) * t;
    const sampleCurveY = (t) => ((ay * t + by) * t + cy) * t;
    const sampleCurveDerivativeX = (t) => (3.0 * ax * t + 2.0 * bx) * t + cx;

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

export function applyEasing(t, type) {
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
