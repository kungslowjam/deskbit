export const EYE_PATHS = {
    // === Modern (Eilik/RUX/EMO Style) ===

    // Angry: Flat top, rounded bottom (Deep Glower)
    angry: "M5,40 L95,40 Q95,85 50,85 Q5,85 5,40 Z",

    // Happy: Thick Bridge/Arc (Joyful)
    happy: "M5,60 Q50,20 95,60 Q95,80 90,80 Q50,45 10,80 Q5,80 5,60 Z",

    // Sad: Inverted Thick Arc (Puppy Eyes)
    sad: "M10,40 Q50,80 90,40 Q90,20 85,20 Q50,60 15,20 Q10,20 10,40 Z",

    // Neutral: Rounded Rectangle (Standard EILIK)
    neutral: "M15,15 L85,15 Q95,15 95,25 L95,75 Q95,85 85,85 L15,85 Q5,85 5,75 L5,25 Q5,15 15,15 Z",

    // Sleepy: Flat pill shape
    sleepy: "M5,45 L95,45 Q100,45 100,55 L100,65 Q100,75 95,75 L5,75 Q0,75 0,65 L0,55 Q0,45 5,45 Z",

    // Love: Chunky Heart
    love: "M50,30 C20,0 -10,35 50,90 C110,35 80,0 50,30 Z",

    // Dead: Thick X
    dead: "M20,20 L30,20 L50,40 L70,20 L80,20 L80,30 L60,50 L80,70 L80,80 L70,80 L50,60 L30,80 L20,80 L20,70 L40,50 L20,30 L20,20 Z",

    // Dizzy: Progressive "Winding" Paths (For smooth creation animation)
    dizzy_1: "M50,50 m0,0 l5,-5 q10,0 10,10 q0,5 -5,5 q-5,0 -5,-5 z", // Core
    dizzy_2: "M50,50 m0,0 l5,-5 q10,0 10,10 q0,10 -10,10 q-15,0 -15,-15 q0,-5 5,-5 q5,0 5,5 q0,5 -5,5 z", // 1st Loop
    dizzy_3: "M50,50 m0,0 l5,-5 q10,0 10,10 q0,10 -10,10 q-15,0 -15,-15 q0,-20 20,-20 q25,0 25,25 q0,5 -5,5 q-5,0 -5,-5 z", // 2nd Loop
    dizzy_4: "M50,50 m0,0 l5,-5 q10,0 10,10 q0,10 -10,10 q-15,0 -15,-15 q0,-20 20,-20 q25,0 25,25 q0,30 -30,30 q-5,0 -5,-10 z", // 3rd Loop
    dizzy_5: "M50,50 m0,0 l5,-5 q10,0 10,10 q0,10 -10,10 q-15,0 -15,-15 q0,-20 20,-20 q25,0 25,25 q0,30 -30,30 q-35,0 -35,-35 q0,-5 5,-5 z", // 4th Loop
    dizzy: "M50,50 m0,0 l5,-5 q10,0 10,10 q0,10 -10,10 q-15,0 -15,-15 q0,-20 20,-20 q25,0 25,25 q0,30 -30,30 q-35,0 -35,-35 q0,-40 40,-40 q45,0 45,45 l-8,0 q0,-35 -35,-35 q-30,0 -30,30 q0,25 25,25 q20,0 20,-20 q0,-15 -15,-15 q-10,0 -10,10 q0,5 5,5 z", // Final Coil

    // Excited: Big Circle
    excited: "M50,10 C85,10 95,25 95,50 C95,75 85,90 50,90 C15,90 5,75 5,50 C5,25 15,10 50,10 Z",

    // Wink: Thick Line
    wink: "M10,48 L90,48 Q95,48 95,53 L95,57 Q95,62 90,62 L10,62 Q5,62 5,57 L5,53 Q5,48 10,48 Z",

    // Alert: Wide Egg
    alert: "M50,10 C80,10 95,30 95,50 C95,75 80,90 50,90 C20,90 5,75 5,50 C5,30 20,10 50,10 Z",

    // Evil: Sharp Slanted
    evil: "M5,55 L30,35 L95,50 Q60,70 5,55 Z",

    // Bored: Half-lidded Rect
    bored: "M10,45 L90,45 Q95,45 95,50 L95,60 Q95,65 90,65 L10,65 Q5,65 5,60 L5,50 Q5,45 10,45 Z",

    // Cry: Sad with Tear
    cry: "M10,40 Q50,80 90,40 Q90,20 85,20 Q50,60 15,20 Q10,20 10,40 Z",

    // Curious: Raised brow effect (Thick)
    curious: "M20,15 Q60,10 90,25 Q95,50 90,80 Q60,90 20,85 Q5,50 20,15 Z",

    // Thinking: Looking up
    thinking: "M10,15 Q50,10 90,15 Q95,40 90,70 Q50,80 10,70 Q5,40 10,15 Z",

    // Skeptic: One raised eyebrow (Asymmetric - usually requires left/right handling but this is general shape)
    skeptic: "M5,40 Q50,25 95,45 Q100,65 95,85 Q50,95 5,85 Q0,65 5,40 Z",

    // Star: Chunky Star
    star: "M50,5 L63,35 L95,40 L70,60 L78,90 L50,75 L22,90 L30,60 L5,40 L37,35 Z",

    // Question: Confused Mark
    question: "M30,20 Q70,10 80,40 Q85,60 55,70 L55,80 L45,80 L45,70 Q45,50 65,40 Q70,30 50,30 Q30,30 30,50 L20,50 Q20,20 30,20 M45,90 L55,90 L55,100 L45,100 Z",

    // Dot: Minimal
    dot: "M40,40 L60,40 L60,60 L40,60 Z",

    // Uwu: Cute squiggly
    uwu: "M5,60 Q25,40 50,60 Q75,40 95,60 Q95,70 85,70 Q75,60 50,75 Q25,60 15,70 Q5,70 5,60 Z",

    // Owo: Big round cute
    owo: "M50,10 C85,10 95,30 95,55 C95,80 85,100 50,100 C15,100 5,80 5,55 C5,30 15,10 50,10 Z",

    // Shock: Very wide open ring
    shock: "M50,5 C90,5 95,20 95,50 C95,80 90,95 50,95 C10,95 5,80 5,50 C5,20 10,5 50,5 Z M50,25 C25,25 25,75 50,75 C75,75 75,25 50,25 Z",

    // Focused: Slightly narrowed, determined
    focused: "M10,40 L90,40 Q95,55 90,70 L10,70 Q5,55 10,40 Z",

    // Tired: Heavy droopy
    tired: "M5,55 L95,55 Q95,85 50,85 Q5,85 5,55 Z",

    // Scared: Wide but trembling shape
    scared: "M50,10 Q85,10 95,40 Q100,65 90,90 Q50,100 10,90 Q0,65 5,40 Q15,10 50,10 Z",

    // Annoyed: Half-lidded, unimpressed
    annoyed: "M5,45 L95,40 Q100,55 95,70 L5,65 Q0,55 5,45 Z",

    // Shy: Small and looking away
    shy: "M30,40 Q50,35 70,45 Q75,60 70,75 Q50,80 30,75 Q25,60 30,40 Z",

    // Loading: Circular progress style
    loading: "M50,10 A40,40 0 1,1 50,90 A40,40 0 1,1 50,10 Z",

    // Closed: Completely shut
    closed: "M5,48 L95,48 Q100,50 95,52 L5,52 Q0,50 5,48 Z",

    // Squint: Almost closed, suspicious
    squint: "M10,45 L90,45 Q95,50 90,55 L10,55 Q5,50 10,45 Z",

    // Glare: Sharp angry stare
    glare: "M5,40 L95,40 Q95,75 50,80 Q5,75 5,40 Z",

    // Pleading: Puppy eyes
    pleading: "M50,5 C90,15 95,40 90,70 Q50,95 10,70 C5,40 10,15 50,5 Z",

    // Bean: Kidney shape
    bean: "M20,40 Q50,30 80,40 Q90,50 80,60 Q50,70 20,60 Q10,50 20,40 Z",

    // Slit: Vertical Line
    slit: "M45,10 L55,10 L55,90 L45,90 Z"
};

export const EYE_ROTATIONS = {
    angry: { left: 25, right: -25 },
    evil: { left: 15, right: -15 },
    sad: { left: -10, right: 10 },
    happy: { left: 0, right: 0 },
    love: { left: 0, right: 0 },
    sleepy: { left: 5, right: -5 },
    bored: { left: 0, right: 0 },
    excited: { left: -5, right: 5 },
    cry: { left: -15, right: 15 },
    alert: { left: 0, right: 0 },
    bean: { left: 0, right: 0 },
    slit: { left: 0, right: 0 },
    // EILIK-style rotations
    neutral: { left: 0, right: 0 },
    wink: { left: 0, right: 0 },
    curious: { left: -8, right: 8 },
    thinking: { left: 0, right: 0 },
    skeptic: { left: -12, right: 5 },
    dead: { left: 0, right: 0 },
    dizzy: { left: 15, right: -15 },
    heart: { left: 0, right: 0 },
    star: { left: 5, right: -5 },
    question: { left: 0, right: 0 },
    dot: { left: 0, right: 0 },
    uwu: { left: 0, right: 0 },
    owo: { left: 0, right: 0 },
    // New presets rotations
    shock: { left: 0, right: 0 },
    focused: { left: 0, right: 0 },
    tired: { left: 8, right: -8 },
    scared: { left: -5, right: 5 },
    annoyed: { left: 5, right: -5 },
    shy: { left: 15, right: -15 },
    loading: { left: 0, right: 0 },
    closed: { left: 0, right: 0 },
    squint: { left: 3, right: -3 },
    glare: { left: 20, right: -20 },
    pleading: { left: -8, right: 8 },
    sparkle: { left: -3, right: 3 }
};

export const ANIMATION_TEMPLATES = {
    wink: {
        frames: [
            { duration: 300, easing: 'linear', transform: 'reset' },
            { duration: 80, easing: 'ease-out', transform: { type: 'squash', target: 'right', factor: 0.8 } },
            { duration: 60, easing: 'ease-out', transform: { type: 'squash', target: 'right', factor: 0.1 } },
            { duration: 80, easing: 'linear', transform: { type: 'squash', target: 'right', factor: 0.1 } },
            { duration: 120, easing: 'overshoot', transform: { type: 'squash', target: 'right', factor: 1.1 } },
            { duration: 150, easing: 'ease-out', transform: 'reset' }
        ]
    },
    blink: {
        frames: [
            { duration: 400, easing: 'linear', transform: 'reset' },
            { duration: 50, easing: 'ease-out', transform: { type: 'squash', target: 'both', factor: 0.8 } },
            { duration: 40, easing: 'ease-out', transform: { type: 'squash', target: 'both', factor: 0.1 } },
            { duration: 30, easing: 'linear', transform: { type: 'squash', target: 'both', factor: 0.1 } },
            { duration: 100, easing: 'overshoot', transform: { type: 'squash', target: 'both', factor: 1.1 } },
            { duration: 120, easing: 'ease-out', transform: 'reset' }
        ]
    },
    surprise: {
        frames: [
            { duration: 80, easing: 'linear', transform: 'reset' },
            { duration: 80, easing: 'overshoot', transform: { type: 'translate', y: -8, scale: { w: 1.1, h: 1.2 } } },
            { duration: 60, easing: 'linear', transform: { type: 'translate', y: -5, scale: { w: 1.05, h: 1.1 } } },
            { duration: 600, easing: 'linear', transform: { type: 'translate', y: -4 } },
            { duration: 250, easing: 'ease-in-out', transform: 'reset' }
        ]
    },
    happy: {
        frames: [
            { duration: 150, easing: 'linear', transform: 'reset' },
            { duration: 100, easing: 'overshoot', transform: { type: 'translate', y: -10 } },
            { duration: 80, easing: 'linear', transform: { type: 'translate', y: -5 } },
            { duration: 80, easing: 'linear', transform: { type: 'translate', y: -7 } },
            { duration: 600, easing: 'linear', transform: { type: 'translate', y: -3 } },
            { duration: 300, easing: 'ease-in-out', transform: 'reset' }
        ]
    },
    sad: {
        frames: [
            { duration: 250, easing: 'linear', transform: 'reset' },
            { duration: 400, easing: 'ease-out', transform: { type: 'translate', y: 8, rotate: { left: -10, right: 10 } } },
            { duration: 200, easing: 'linear', transform: { type: 'translate', y: 10, x: 1, rotate: { left: -12, right: 12 } } },
            { duration: 200, easing: 'linear', transform: { type: 'translate', y: 9, x: -1, rotate: { left: -10, right: 10 } } },
            { duration: 400, easing: 'ease-in-out', transform: 'reset' }
        ]
    },
    nod: {
        frames: [
            { duration: 150, easing: 'ease-in-out', transform: 'reset' },
            { duration: 100, easing: 'ease-out', transform: { type: 'translate', y: 15 } },
            { duration: 80, easing: 'ease-in', transform: { type: 'translate', y: 5 } },
            { duration: 100, easing: 'ease-out', transform: { type: 'translate', y: 12 } },
            { duration: 150, easing: 'ease-in-out', transform: 'reset' }
        ]
    },
    shake: {
        frames: [
            { duration: 80, easing: 'linear', transform: 'reset' },
            { duration: 50, easing: 'ease-out', transform: { type: 'translate', x: -10 } },
            { duration: 50, easing: 'ease-out', transform: { type: 'translate', x: 10 } },
            { duration: 50, easing: 'ease-out', transform: { type: 'translate', x: -8 } },
            { duration: 50, easing: 'ease-out', transform: { type: 'translate', x: 8 } },
            { duration: 120, easing: 'ease-in-out', transform: 'reset' }
        ]
    },
    skeptic: {
        frames: [
            { duration: 200, easing: 'linear', transform: 'reset' },
            { duration: 180, easing: 'ease-out', transform: { type: 'squash-asymmetric', left: { squash: 1.0 }, right: { squash: 0.7, rotate: -10 } } },
            { duration: 800, easing: 'linear', transform: { type: 'squash-asymmetric', left: { rotate: 5 }, right: { squash: 0.6, rotate: -12 } } },
            { duration: 300, easing: 'ease-in-out', transform: 'reset' }
        ]
    },
    love: {
        frames: [
            { duration: 150, easing: 'linear', transform: 'reset' },
            { duration: 150, easing: 'overshoot', transform: { type: 'translate', scale: { w: 1.2, h: 1.2 } } },
            { duration: 120, easing: 'ease-out', transform: { type: 'translate', scale: { w: 1.05, h: 1.05 } } },
            { duration: 120, easing: 'ease-in', transform: { type: 'translate', scale: { w: 1.15, h: 1.15 } } },
            { duration: 120, easing: 'ease-out', transform: { type: 'translate', scale: { w: 1.05, h: 1.05 } } },
            { duration: 400, easing: 'linear', transform: 'reset' }
        ]
    },
    angry: {
        frames: [
            { duration: 100, easing: 'linear', transform: 'reset' },
            { duration: 80, easing: 'ease-out', transform: { type: 'translate', x: 3 } },
            { duration: 60, easing: 'linear', transform: { type: 'translate', x: -3 } },
            { duration: 60, easing: 'linear', transform: { type: 'translate', x: 3 } },
            { duration: 60, easing: 'linear', transform: { type: 'translate', x: -3 } },
            { duration: 600, easing: 'linear', transform: { type: 'translate', scale: { w: 1.05, h: 1.0 } } },
            { duration: 300, easing: 'ease-in-out', transform: 'reset' }
        ]
    },
    sleepy: {
        frames: [
            { duration: 400, easing: 'linear', transform: 'reset' },
            { duration: 500, easing: 'ease-out', transform: { type: 'squash', target: 'both', factor: 0.6 } },
            { duration: 300, easing: 'ease-out', transform: { type: 'squash', target: 'both', factor: 0.4 } },
            { duration: 400, easing: 'ease-out', transform: { type: 'squash', target: 'both', factor: 0.1 } },
            { duration: 600, easing: 'ease-in', transform: { type: 'squash', target: 'both', factor: 0.3 } },
            { duration: 800, easing: 'ease-out', transform: { type: 'squash', target: 'both', factor: 0.1 } }
        ]
    },
    confused: {
        frames: [
            { duration: 150, easing: 'linear', transform: 'reset' },
            { duration: 180, easing: 'ease-out', transform: { type: 'tilt', rotate: 10 } },
            { duration: 180, easing: 'ease-out', transform: { type: 'tilt', rotate: -10 } },
            { duration: 400, easing: 'linear', transform: 'reset' }
        ]
    },
    excited: {
        frames: [
            { duration: 80, easing: 'linear', transform: 'reset' },
            { duration: 60, easing: 'overshoot', transform: { type: 'translate', y: -12 } },
            { duration: 50, easing: 'linear', transform: { type: 'translate', y: 8 } },
            { duration: 50, easing: 'linear', transform: { type: 'translate', y: -10 } },
            { duration: 50, easing: 'linear', transform: { type: 'translate', y: 6 } },
            { duration: 400, easing: 'ease-out', transform: 'reset' }
        ]
    },
    idle: {
        frames: [
            { duration: 800, easing: 'ease-in-out', transform: 'reset' },
            { duration: 600, easing: 'ease-in-out', transform: { type: 'translate', scale: { w: 1.02, h: 1.02 } } },
            { duration: 800, easing: 'ease-in-out', transform: 'reset' },
            { duration: 600, easing: 'ease-in-out', transform: { type: 'translate', scale: { w: 1.02, h: 1.02 } } },
            { duration: 60, easing: 'ease-out', transform: { type: 'squash', target: 'both', factor: 0.1 } },
            { duration: 40, easing: 'linear', transform: { type: 'squash', target: 'both', factor: 0.1 } },
            { duration: 100, easing: 'overshoot', transform: 'reset' },
            { duration: 600, easing: 'ease-in-out', transform: 'reset' }
        ]
    },
    curious: {
        frames: [
            { duration: 200, easing: 'linear', transform: 'reset' },
            { duration: 300, easing: 'ease-in-out', transform: { type: 'translate', x: -15, rotate: { left: -5, right: -5 } } },
            { duration: 200, easing: 'ease-in-out', transform: 'reset' },
            { duration: 300, easing: 'ease-in-out', transform: { type: 'translate', x: 15, rotate: { left: 5, right: 5 } } },
            { duration: 200, easing: 'ease-in-out', transform: 'reset' }
        ]
    },
    laugh: {
        frames: [
            { duration: 100, easing: 'linear', transform: 'reset' },
            { duration: 80, easing: 'ease-out', transform: { type: 'translate', y: -8 } },
            { duration: 60, easing: 'linear', transform: { type: 'translate', y: -3 } },
            { duration: 60, easing: 'linear', transform: { type: 'translate', y: -10 } },
            { duration: 60, easing: 'linear', transform: { type: 'translate', y: -4 } },
            { duration: 300, easing: 'ease-out', transform: 'reset' }
        ]
    },
    bounce: {
        frames: [
            { duration: 100, easing: 'ease-in', transform: { type: 'translate', y: 0, scale: { w: 1.1, h: 0.8 } } },
            { duration: 80, easing: 'overshoot', transform: { type: 'translate', y: -20, scale: { w: 0.9, h: 1.1 } } },
            {
                duration: 100, easing: 'ease-in', transform: { type: 'reset' }, // Typo fix in original code or intnetional? original was 'reset'
                // Looking at Bounce...
                // { duration: 100, easing: 'ease-in', transform: 'reset' },
                duration: 100, easing: 'ease-in', transform: 'reset'
            },
            { duration: 80, easing: 'overshoot', transform: { type: 'translate', y: -5 } },
            { duration: 150, easing: 'ease-in-out', transform: 'reset' }
        ]
    },
    dizzy: {
        frames: [
            // Phase 1: Progressive Construction (Winding Up)
            { duration: 100, easing: 'ease-in-out', transform: { path: 'dizzy_1', type: 'translate', rotate: { left: 45, right: -45 }, scale: { w: 0.8, h: 0.8 } } },
            { duration: 80, easing: 'ease-in-out', transform: { path: 'dizzy_2', type: 'translate', rotate: { left: 90, right: -90 }, scale: { w: 0.85, h: 0.85 } } },
            { duration: 80, easing: 'ease-in-out', transform: { path: 'dizzy_3', type: 'translate', rotate: { left: 135, right: -135 }, scale: { w: 0.9, h: 0.9 } } },
            { duration: 80, easing: 'ease-in-out', transform: { path: 'dizzy_4', type: 'translate', rotate: { left: 180, right: -180 }, scale: { w: 0.95, h: 0.95 } } },
            { duration: 80, easing: 'ease-in-out', transform: { path: 'dizzy_5', type: 'translate', rotate: { left: 225, right: -225 }, scale: { w: 1.0, h: 1.0 } } },
            { duration: 80, easing: 'ease-out', transform: { path: 'dizzy', type: 'translate', rotate: { left: 270, right: -270 }, scale: { w: 1.05, h: 1.05 } } },

            // Phase 2: Hyper-Active Counter-Spin + Pulse (Alive Effect)
            { duration: 150, easing: 'linear', transform: { type: 'translate', rotate: { left: 360, right: -360 }, scale: { w: 1.1, h: 1.1 } } },
            { duration: 150, easing: 'linear', transform: { type: 'translate', rotate: { left: 450, right: -450 }, scale: { w: 1.0, h: 1.0 } } },
            { duration: 150, easing: 'linear', transform: { type: 'translate', rotate: { left: 540, right: -540 }, scale: { w: 1.1, h: 1.1 } } },
            { duration: 150, easing: 'linear', transform: { type: 'translate', rotate: { left: 630, right: -630 }, scale: { w: 1.0, h: 1.0 } } },
            { duration: 150, easing: 'linear', transform: { type: 'translate', rotate: { left: 720, right: -720 }, scale: { w: 1.1, h: 1.1 } } },
            { duration: 150, easing: 'linear', transform: { type: 'translate', rotate: { left: 810, right: -810 }, scale: { w: 1.0, h: 1.0 } } },
            { duration: 150, easing: 'linear', transform: { type: 'translate', rotate: { left: 900, right: -900 }, scale: { w: 1.1, h: 1.1 } } },
            { duration: 150, easing: 'linear', transform: { type: 'translate', rotate: { left: 1080, right: -1080 }, scale: { w: 1.0, h: 1.0 } } },

            // Phase 3: Settle Down
            { duration: 400, easing: 'ease-out', transform: 'reset' }
        ]
    }
};
