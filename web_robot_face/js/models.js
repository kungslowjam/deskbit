import { state } from './state.js';
import { drawShape } from './renderer.js';

export class Shape {
    constructor(type, x, y, w, h, color) {
        this.id = Date.now() + Math.random();
        this.type = type;
        this.x = x;
        this.y = y;
        this.width = w;
        this.height = h;
        this.color = color;
        this.rotation = 0;

        // Default Properties from State
        this.opacity = state.shapeDefaults.opacity;
        this.blendMode = state.shapeDefaults.blendMode;

        this.lineEnd = null;
        this.interaction = null;

        this.strokeWidth = state.shapeDefaults.strokeWidth;
        this.strokeColor = state.shapeDefaults.strokeColor;
        this.cornerRadius = state.shapeDefaults.cornerRadius;

        this.pathData = null;
        this.nodes = [];
        this.isEditingPositions = false;
        this.isMirrored = false;
    }

    parsePath() {
        if (!this.pathData) return;
        this.nodes = [];
        const commands = this.pathData.match(/[a-zA-Z][^a-zA-Z]*/g);
        if (!commands) return;

        commands.forEach(cmd => {
            const type = cmd[0];
            const args = cmd.slice(1).trim().split(/[\s,]+/).map(parseFloat);
            if (type === 'M' || type === 'L') {
                this.nodes.push({ type, x: args[0], y: args[1] });
            } else if (type === 'Q') {
                this.nodes.push({ type, cx: args[0], cy: args[1], x: args[2], y: args[3] });
            } else if (type === 'C') {
                this.nodes.push({ type, x1: args[0], y1: args[1], x2: args[2], y2: args[3], x: args[4], y: args[5] });
            } else if (type === 'Z' || type === 'z') {
                this.nodes.push({ type: 'Z' });
            }
        });
    }

    updatePathData() {
        this.pathData = this.nodes.map(n => {
            if (n.type === 'M' || n.type === 'L') return `${n.type}${n.x},${n.y}`;
            if (n.type === 'Q') return `Q${n.cx},${n.cy} ${n.x},${n.y}`;
            if (n.type === 'C') return `C${n.x1},${n.y1} ${n.x2},${n.y2} ${n.x},${n.y}`;
            if (n.type === 'Z') return 'Z';
            return '';
        }).join(' ');
    }

    containsPoint(px, py) {
        if (this.type === 'rect') {
            return px >= this.x && px <= this.x + this.width &&
                py >= this.y && py <= this.y + this.height;
        } else if (this.type === 'ellipse') {
            const cx = this.x + this.width / 2;
            const cy = this.y + this.height / 2;
            const rx = this.width / 2;
            const ry = this.height / 2;
            return Math.pow((px - cx) / rx, 2) + Math.pow((py - cy) / ry, 2) <= 1;
        } else if (this.type === 'line' && this.lineEnd) {
            const A = px - this.x;
            const B = py - this.y;
            const C = this.lineEnd.x - this.x;
            const D = this.lineEnd.y - this.y;
            const dot = A * C + B * D;
            const lenSq = C * C + D * D;
            let param = lenSq !== 0 ? dot / lenSq : -1;
            param = Math.max(0, Math.min(1, param));
            const xx = this.x + param * C;
            const yy = this.y + param * D;
            const dist = Math.sqrt((px - xx) ** 2 + (py - yy) ** 2);
            return dist < 5;
        } else if (this.type === 'triangle') {
            const x1 = this.x + this.width / 2, y1 = this.y;
            const x2 = this.x, y2 = this.y + this.height;
            const x3 = this.x + this.width, y3 = this.y + this.height;
            const area = 0.5 * (-y2 * x3 + y1 * (-x2 + x3) + x1 * (y2 - y3) + x2 * y3);
            const s = 1 / (2 * area) * (y1 * x3 - x1 * y3 + (y3 - y1) * px + (x1 - x3) * py);
            const t = 1 / (2 * area) * (x1 * y2 - y1 * x2 + (y1 - y2) * px + (x2 - x1) * py);
            return s > 0 && t > 0 && (1 - s - t) > 0;
        } else if ((this.type === 'path' && this.pathData) || this.type === 'image') {
            return px >= this.x && px <= this.x + this.width &&
                py >= this.y && py <= this.y + this.height;
        }
        return false;
    }

    getBounds() {
        if (this.type === 'line' && this.lineEnd) {
            return {
                x: Math.min(this.x, this.lineEnd.x),
                y: Math.min(this.y, this.lineEnd.y),
                width: Math.abs(this.lineEnd.x - this.x),
                height: Math.abs(this.lineEnd.y - this.y)
            };
        }
        return { x: this.x, y: this.y, width: this.width, height: this.height };
    }

    clone(preserveId = false) {
        const newShape = new Shape(this.type, this.x, this.y, this.width, this.height, this.color);
        newShape.rotation = this.rotation;
        newShape.opacity = this.opacity;
        newShape.blendMode = this.blendMode;
        if (this.lineEnd) {
            newShape.lineEnd = { ...this.lineEnd };
        }
        if (preserveId) {
            newShape.id = this.id;
        }
        if (this.interaction) {
            newShape.interaction = { ...this.interaction };
        }
        newShape.strokeWidth = this.strokeWidth;
        newShape.strokeColor = this.strokeColor;
        newShape.cornerRadius = this.cornerRadius;
        newShape.pathData = this.pathData;
        newShape.isMirrored = this.isMirrored;

        return newShape;
    }

    draw(context) {
        drawShape(context, this);
    }
}

export class TextShape extends Shape {
    constructor(x, y, text, fontSize, color) {
        super('text', x, y, 100, 30, color);
        this.text = text || 'Text';
        this.fontSize = fontSize || 16;
        this.id = Date.now() + Math.random();
    }

    draw(context) {
        drawShape(context, this);

        // Measure Logic (Side Effect - handled here)
        context.save();
        context.font = `${this.fontSize}px Inter, sans-serif`;
        const metrics = context.measureText(this.text);
        this.width = metrics.width;
        this.height = this.fontSize * 1.2;
        context.restore();
    }

    containsPoint(px, py) {
        return px >= this.x && px <= this.x + this.width &&
            py >= this.y && py <= this.y + this.height;
    }

    clone(preserveId = false) {
        const newShape = new TextShape(this.x, this.y, this.text, this.fontSize, this.color);
        newShape.rotation = this.rotation;
        newShape.opacity = this.opacity;
        newShape.blendMode = this.blendMode;
        if (preserveId) {
            newShape.id = this.id;
        } else {
            newShape.id = Date.now() + Math.random();
        }
        newShape.width = this.width;
        newShape.height = this.height;
        return newShape;
    }
}

export class Frame {
    constructor() {
        this.pixels = new Array(state.GRID_WIDTH * state.GRID_HEIGHT).fill(null);
        this.shapes = [];
        this.duration = 100;
        this.id = Date.now() + Math.random();
        this.cacheCanvas = document.createElement('canvas');
        this.cacheCanvas.width = state.GRID_WIDTH;
        this.cacheCanvas.height = state.GRID_HEIGHT;
        this.cacheCtx = this.cacheCanvas.getContext('2d');
        this.isCacheDirty = true;
    }

    updateCache() {
        const ctx = this.cacheCtx;
        ctx.clearRect(0, 0, state.GRID_WIDTH, state.GRID_HEIGHT);

        const imgData = ctx.createImageData(state.GRID_WIDTH, state.GRID_HEIGHT);
        const data = imgData.data;

        for (let i = 0; i < this.pixels.length; i++) {
            const color = this.pixels[i];
            if (color) {
                const r = parseInt(color.slice(1, 3), 16);
                const g = parseInt(color.slice(3, 5), 16);
                const b = parseInt(color.slice(5, 7), 16);
                const idx = i * 4;
                data[idx] = r;
                data[idx + 1] = g;
                data[idx + 2] = b;
                data[idx + 3] = 255;
            }
        }
        ctx.putImageData(imgData, 0, 0);
        this.isCacheDirty = false;
    }
}

export class ImageShape extends Shape {
    constructor(x, y, src, width, height) {
        super('image', x, y, width || 100, height || 100, '#ffffff');
        this.src = src;
        this.image = new Image();
        this.isLoaded = false;
        this.image.onload = () => {
            this.isLoaded = true;
            // Trigger re-render
            window.dispatchEvent(new CustomEvent('robot-face-refresh'));
        };
        this.image.onerror = () => {
            console.error("Failed to load image:", src);
        }
        this.image.src = src;
    }

    draw(context) {
        drawShape(context, this);
    }

    clone(preserveId = false) {
        const newShape = new ImageShape(this.x, this.y, this.src, this.width, this.height);
        newShape.rotation = this.rotation;
        newShape.opacity = this.opacity;
        newShape.blendMode = this.blendMode;
        if (preserveId) {
            newShape.id = this.id;
        } else {
            newShape.id = Date.now() + Math.random();
        }
        newShape.strokeWidth = this.strokeWidth;
        // Share image instance if loaded to avoid reload flicker
        if (this.isLoaded) {
            newShape.image = this.image;
            newShape.isLoaded = true;
        }
        return newShape;
    }
}
