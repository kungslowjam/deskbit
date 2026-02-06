/**
 * @file ui_robo_eyes.c
 * @brief Robot Eyes - Canvas Based Perfect Teardrop (Fat Shape)
 */

#include "ui_robo_eyes.h"
#include "anim_manager.h"
#include "lvgl.h"
#include "ui_custom_anim.h" // Add support for custom animations
#include "ui_settings.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

extern void register_all_animations(void);

LV_FONT_DECLARE(lv_font_montserratMedium_20);
LV_FONT_DECLARE(lv_font_montserratMedium_23);
LV_FONT_DECLARE(lv_font_montserrat_48); // Largest Standard Font

// Screen constants
#define SCREEN_WIDTH 466
#define SCREEN_HEIGHT 466
#include "esp_heap_caps.h"

// Eye config
#define EYE_WIDTH 120
#define EYE_HEIGHT 190
#define EYE_RADIUS 100
#define EYE_OFFSET_X 90
#define EYE_COLOR lv_color_hex(0x00FFFF)
#define BG_COLOR lv_color_hex(0x000000)

// Happy Config
#define CHEEK_SIZE 240
#define CHEEK_OFFSET_Y_HIDDEN 240
#define CHEEK_OFFSET_Y_VISIBLE 90

// Sad Config
#define SAD_MASK_SIZE 320
#define SAD_MASK_OFFSET_X 60
#define SAD_MASK_OFFSET_Y_HIDDEN -300
#define SAD_MASK_OFFSET_Y_VISIBLE -160
#define EYE_HEIGHT_SAD 140

// Teardrop Config (Fat)
#define TEAR_WIDTH 36
#define TEAR_HEIGHT 60
#define TEAR_OFFSET_X 0
#define TEAR_START_Y 100
#define TEAR_END_Y 220

// Laugh Config
#define LAUGH_EYE_WIDTH 100
#define LAUGH_EYE_OFFSET_Y -30
// Omega mouth "ω" config
#define OMEGA_MOUTH_WIDTH 120   // Total width of omega
#define OMEGA_MOUTH_HEIGHT 30   // Height of the curves
#define OMEGA_MOUTH_OFFSET_Y 70 // Position below center

// Laugh Animation Config - LIVELY & ENERGETIC!
#define LAUGH_CYCLE_MS 150
#define SQUINT_AMPLITUDE 8 // Bigger squint effect
#define BOUNCE_AMPLITUDE 6 // More bounce
#define WOBBLE_AMPLITUDE 8 // More wobble = more playful
#define MOUTH_BOUNCE_AMPLITUDE 25
// Enhanced Eye Animation - SMOOTH & ALIVE
#define EYE_PULSE_SCALE_MIN 0.90f
#define EYE_PULSE_SCALE_MAX 1.10f
#define EYE_SQUINT_HEIGHT_MIN 24
#define EYE_SQUINT_HEIGHT_MAX 36
#define EYE_PULSE_SPEED 5.0f // Slower dialled back for realism

// Objects
static lv_obj_t *scr_eyes = NULL;
static lv_obj_t *left_eye = NULL;
static lv_obj_t *right_eye = NULL;
static lv_obj_t *left_cheek = NULL;
static lv_obj_t *right_cheek = NULL;
static lv_obj_t *left_sad_mask = NULL;
static lv_obj_t *right_sad_mask = NULL;
static lv_obj_t *left_tear = NULL;
static lv_obj_t *right_tear = NULL;
static lv_obj_t *happy_open_mouth = NULL; // New: Big open happy mouth

// Laugh Objects
static lv_obj_t *left_laugh_eye = NULL;
static lv_obj_t *right_laugh_eye = NULL;

static lv_obj_t *big_laugh_mouth = NULL; // Frame 1: Big Pink Mouth (Open)
static lv_obj_t *laugh_tongue = NULL;    // Tongue inside mouth
static lv_obj_t *grin_laugh_mouth =
    NULL; // Frame 2: Grinning Teeth Mouth (Closed)

// Love Objects (Heart Eyes) - Canvas based for reliability
static lv_obj_t *left_heart_eye = NULL;
static lv_obj_t *right_heart_eye = NULL;
static lv_obj_t *canvas_heart_ref = NULL; // Cached heart canvas

// Heart Canvas Config - LARGER for visibility!
#define HEART_CANVAS_SIZE 120

// Big Laugh Mouth Config
#define BIG_MOUTH_WIDTH 180
#define BIG_MOUTH_HEIGHT 150
#define BIG_MOUTH_RADIUS 80
#define BIG_MOUTH_COLOR lv_color_hex(0xFF9999) // Pinkish
#define BIG_MOUTH_OFFSET_Y 60

// Love Config (Heart Eyes)
#define HEART_EYE_SIZE 100
#define HEART_EYE_OFFSET_X 90
#define HEART_BEAT_CYCLE_MS 800
#define HEART_SCALE_MIN 0.85f
#define HEART_SCALE_MAX 1.20f
#define HEART_FLOAT_AMPLITUDE 8
#define HEART_COLOR lv_color_hex(0xFF6B8A)      // Pink color for glow
#define ANGRY_COLOR lv_color_hex(0xFF0000)      // Red
#define ANGRY_RAGE_COLOR lv_color_hex(0xCC0000) // Darker Red for pulse
#define ANGRY_EYE_WIDTH 160
#define ANGRY_EYE_HEIGHT 110

// Canvas Teardrop Single Instance Cache
// Canvas Teardrop Single Instance Cache
static lv_obj_t *canvas_tear_ref = NULL;
static lv_obj_t *canvas_grin_ref = NULL;

static lv_obj_t *left_angry_eye = NULL;
static lv_obj_t *right_angry_eye = NULL;

// Frame 2: Blue Angry Face (Cyan Eyes + Mouth)
static lv_obj_t *left_angry_blue_eye = NULL;
static lv_obj_t *right_angry_blue_eye = NULL;
static lv_obj_t *angry_blue_mouth = NULL;
// Lids removed - using direct canvas styling

static lv_timer_t *main_timer = NULL;

// Custom Animation Player
static custom_anim_t *custom_anim_player = NULL;

// References to exported shape animation data
extern const shape_keyframe_t my_anim_f0_shapes[];
extern const shape_keyframe_t my_anim_f1_shapes[];
extern const shape_keyframe_t my_anim_f2_shapes[];
extern const uint8_t my_anim_f0_shape_count;
extern const uint8_t my_anim_f1_shape_count;
extern const uint8_t my_anim_f2_shape_count;
extern const uint32_t my_anim_total_duration;
extern const anim_vector_t angry_anim_data; // New Angry Animation Data
extern const anim_vector_t dizzy_anim_data; // New Dizzy/Seething Animation Data

// State
typedef enum {
  EMO_IDLE,
  EMO_HAPPY,
  EMO_SAD,
  EMO_LAUGH,
  EMO_LOVE,
  EMO_SLEEP,
  EMO_ANGRY,
  EMO_CUSTOM
} emotion_t;
static emotion_t current_emotion = EMO_IDLE;
static robot_emotion_t last_api_emotion = (robot_emotion_t)-1; // Global guard
static int timer_ms = 0;
static int breath_phase = 0;
// Removed static int16_t gaze_x/y as we use floats now
// static int16_t gaze_x = 0;
// static int16_t gaze_y = 0;
static int laugh_shake_phase = 0;
static int laugh_cycle = 0;
static int laugh_intensity = 0; // 0-100, builds up during laugh
static int angry_phase = 0;     // 0: Pulse, 1: Animation

// Styles
static lv_style_t style_eye;
static lv_style_t style_mask;
static lv_style_t style_tear_part;
static lv_style_t style_line;
#define HAPPY_MOUTH_POINTS 17
static lv_point_t happy_mouth_pts[HAPPY_MOUTH_POINTS];

// Laugh Eye Points (Curved 'n' shape) - 17 Points for SUPER SMOOTH arc
// Will be set dynamically in update_positions
#define LAUGH_EYE_POINTS 17
static lv_point_t left_arrow_pts[LAUGH_EYE_POINTS];
static lv_point_t right_arrow_pts[LAUGH_EYE_POINTS];

// Dynamic animation state for eyes
// Dynamic animation state for eyes
static float eye_pulse_phase = 0.0f;
// static float breath_val = 0.0f; // Unused for now

// LIFELIKE GAZE SYSTEM (Physics-based)
// "Target" is where we want to look, "Current" is where eyes actually are
static float current_gaze_x = 0.0f;
static float current_gaze_y = 0.0f;
static float target_gaze_x = 0.0f;
static float target_gaze_y = 0.0f;
static float gaze_vel_x = 0.0f; // Velocity for spring physics
static float gaze_vel_y = 0.0f;

// Physics Constants
#define GAZE_SPRING_K 0.12f // Stiffness (Adjusted for faster loop)
#define GAZE_DAMPING 0.85f  // Friction (Higher value = Less loss per frame)

// Saccade (Random micro-movements)
static int saccade_timer = 0;
static int next_saccade_time = 50; // Randomize start

// Blink Randomness
static int blink_timer = 0;
static int next_blink_time = 3000;

// Love animation state
static float heart_beat_phase = 0.0f;
static int love_intensity = 0;

// (Old omega mouth pts removed)
static lv_style_t style_happy_line;

static lv_obj_t *happy_mouth = NULL;

// Sleep Elements
static lv_obj_t *sleep_z1 = NULL;
static lv_obj_t *sleep_z2 = NULL;
static lv_obj_t *sleep_z3 = NULL;
static lv_obj_t *left_sleep_eye = NULL;
static lv_obj_t *right_sleep_eye = NULL;
static lv_obj_t *sleep_mouth = NULL; // New: Yawning mouth
static lv_point_t sleep_eye_pts[LAUGH_EYE_POINTS];

// Dynamic mouth state
// (Old waveline mouth vars removed)

// Idle Animation State (Random Ticks)
typedef enum {
  IDLE_ACT_NONE,
  IDLE_ACT_SQUINT,
  IDLE_ACT_WIDE,
  IDLE_ACT_SCAN,
  IDLE_ACT_WINK,
  IDLE_ACT_THINK,
  IDLE_ACT_ROLL,
  IDLE_ACT_GLANCE,
  IDLE_ACT_SHIVER,
  IDLE_ACT_BOUNCE,
  IDLE_ACT_TILT,
  IDLE_ACT_CURIOUS
} idle_action_t;
static lv_style_t style_z;
static idle_action_t idle_anim_type = IDLE_ACT_NONE;
static int idle_anim_timer = 0;
static int next_idle_anim_time = 2000;

static float idle_anim_phase = 0.0f; // 0.0 to 1.0 logic

// Smoothed Size State (for Normal/Idle Jelly Physics)
static float current_eye_w = EYE_WIDTH;
static float current_eye_h = EYE_HEIGHT;
#define SIZE_LERP 0.2f // Higher = Faster/More responsive, Lower = Smoother

// Forward Declarations for Static Functions
static void fade_out_normal_eyes(void);
static void hide_happy(void);
static void hide_sad(void);
static void hide_laugh(void);
static void hide_love(void);
static void hide_sleep(void);
static void hide_angry(void);
static void show_idle(void);
static void do_blink(void);

// -----------------------------------------------------------------------------
// Physics & Animation Helpers
// -----------------------------------------------------------------------------

static void update_laugh_physics(float phase_rad, float intensity_scale) {
  // --- 1. Update EYES (Arc 'n' vs Sharp '><') ---
  // Rhythm matches the head bounce
  float wave_speed = 1.5f;
  float wave_for_eyes = sinf(phase_rad * wave_speed);

  // Check if mouth is open (Sync with mouth logic: wave > -0.2)
  bool is_mouth_open = (wave_for_eyes > -0.2f);

  if (is_mouth_open) {
    // -----------------------------------------------------
    // Draw '><' Shape (Sharp Squint)
    // -----------------------------------------------------
    // Left Eye: '>' (Points Right)
    // Path: Top-Left -> Center-Right -> Bottom-Left
    int16_t x1 = 10, y1 = 0;
    int16_t x2 = 90, y2 = 25; // Apex
    int16_t x3 = 10, y3 = 50;

    for (int i = 0; i < LAUGH_EYE_POINTS; i++) {
      float t = (float)i / (LAUGH_EYE_POINTS - 1);
      if (t < 0.5f) {
        // Top leg
        float st = t * 2.0f; // 0..1
        left_arrow_pts[i].x = x1 + (int16_t)((x2 - x1) * st);
        left_arrow_pts[i].y = y1 + (int16_t)((y2 - y1) * st);
      } else {
        // Bottom leg
        float st = (t - 0.5f) * 2.0f; // 0..1
        left_arrow_pts[i].x = x2 + (int16_t)((x3 - x2) * st);
        left_arrow_pts[i].y = y2 + (int16_t)((y3 - y2) * st);
      }
    }

    // Right Eye: '<' (Points Left)
    // Path: Top-Right -> Center-Left -> Bottom-Right
    x1 = 90;
    y1 = 0;
    x2 = 10;
    y2 = 25; // Apex
    x3 = 90;
    y3 = 50;

    for (int i = 0; i < LAUGH_EYE_POINTS; i++) {
      float t = (float)i / (LAUGH_EYE_POINTS - 1);
      if (t < 0.5f) {
        float st = t * 2.0f;
        right_arrow_pts[i].x = x1 + (int16_t)((x2 - x1) * st);
        right_arrow_pts[i].y = y1 + (int16_t)((y2 - y1) * st);
      } else {
        float st = (t - 0.5f) * 2.0f;
        right_arrow_pts[i].x = x2 + (int16_t)((x3 - x2) * st);
        right_arrow_pts[i].y = y2 + (int16_t)((y3 - y2) * st);
      }
    }

  } else {
    // -----------------------------------------------------
    // Draw '^' Shape (Arc) - Existing Logic
    // -----------------------------------------------------
    // Morph Factor: -1.0 (Squint/Grin) to 1.0 (Open/High Arc)
    float base_h = 12.0f;
    float dynamic_h = base_h + (5.0f * wave_for_eyes * intensity_scale);

    // Curvature (a coeff)
    float sharpness =
        1.0f - (0.3f * wave_for_eyes); // 1.3 (Sharp) to 0.7 (Round)
    float a_coeff = (dynamic_h / 2500.0f) * sharpness;

    for (int i = 0; i < LAUGH_EYE_POINTS; i++) {
      float t = (float)i / (LAUGH_EYE_POINTS - 1); // 0.0 to 1.0
      int16_t x = (int16_t)(t * 100.0f);
      // y = a * (x - 50)^2
      float dx = x - 50.0f;
      int16_t y = (int16_t)(a_coeff * dx * dx);

      left_arrow_pts[i].x = x;
      left_arrow_pts[i].y = y;
      right_arrow_pts[i].x = x;
      right_arrow_pts[i].y = y;
    }
  }

  if (left_laugh_eye)
    lv_line_set_points(left_laugh_eye, left_arrow_pts, LAUGH_EYE_POINTS);
  if (right_laugh_eye)
    lv_line_set_points(right_laugh_eye, right_arrow_pts, LAUGH_EYE_POINTS);

  // Line width - THICKER for better look
  float pulse_rad = phase_rad; // Reuse phase for sync
  int16_t line_thickness =
      16 + (int16_t)(4.0f * intensity_scale * sinf(pulse_rad * 2.0f));

  if (left_laugh_eye)
    lv_obj_set_style_line_width(left_laugh_eye, line_thickness, 0);
  if (right_laugh_eye)
    lv_obj_set_style_line_width(right_laugh_eye, line_thickness, 0);
}

static void update_laugh_mouth_and_shake(float phase_rad, float intensity_scale,
                                         int16_t *out_shake_x,
                                         int16_t *out_shake_y,
                                         int16_t *out_squint) {
  float wave_speed = 1.5f;
  float laugh_wave = sinf(phase_rad * wave_speed);

  // Soft bounce (no hard clipping)
  float bounce = (laugh_wave > 0) ? laugh_wave : (laugh_wave * 0.2f);
  *out_shake_y = -(int16_t)(8.0f * bounce * intensity_scale);
  *out_shake_x = 0;

  // Squint syncs with bounce
  *out_squint = (int16_t)(4.0f * bounce * intensity_scale);

  // BIG MOUTH / GRIN MOUTH Animation (2-Frame Toggle)
  // Toggle Threshold > -0.2 : Morph towards Open
  bool show_open = (laugh_wave > -0.2f);

  if (show_open) {
    if (big_laugh_mouth &&
        lv_obj_has_flag(big_laugh_mouth, LV_OBJ_FLAG_HIDDEN)) {
      lv_obj_clear_flag(big_laugh_mouth, LV_OBJ_FLAG_HIDDEN);
    }
    if (grin_laugh_mouth &&
        !lv_obj_has_flag(grin_laugh_mouth, LV_OBJ_FLAG_HIDDEN)) {
      lv_obj_add_flag(grin_laugh_mouth, LV_OBJ_FLAG_HIDDEN);
    }
  } else {
    if (grin_laugh_mouth &&
        lv_obj_has_flag(grin_laugh_mouth, LV_OBJ_FLAG_HIDDEN)) {
      lv_obj_clear_flag(grin_laugh_mouth, LV_OBJ_FLAG_HIDDEN);
    }
    if (big_laugh_mouth &&
        !lv_obj_has_flag(big_laugh_mouth, LV_OBJ_FLAG_HIDDEN)) {
      lv_obj_add_flag(big_laugh_mouth, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

static void update_sleep_physics(float breath_rad, float *out_heave) {
  // 1. ASYMMETRIC BREATH: Snore rhythm
  float wave = sinf(breath_rad);
  float heave = (wave > 0) ? powf(wave, 1.5f) : (wave * 0.5f);
  *out_heave = heave;

  // 2. YAWNING MOUTH LOGIC
  if (sleep_mouth) {
    float mouth_open = (heave > 0) ? heave : 0.0f;
    int16_t m_h = (int16_t)(5.0f + 40.0f * mouth_open);
    int16_t m_w = (int16_t)(10.0f + 20.0f * mouth_open);
    lv_obj_set_size(sleep_mouth, m_w, m_h);
    lv_obj_set_style_opa(sleep_mouth, 100 + (int16_t)(155 * mouth_open), 0);
  }

  // 3. EYES SQUASH & STRETCH
  float width_mod = 0.9f + (0.25f * (heave + 1.0f) * 0.5f);
  float squint_fac = (heave > 0.5f) ? (1.0f + (heave - 0.5f) * 0.8f) : 1.0f;
  float bend_mod = 0.015f * (1.0f - 0.4f * heave * squint_fac);
  float eye_dip = 15.0f + (8.0f * heave);

  for (int i = 0; i < LAUGH_EYE_POINTS; i++) {
    float t = (float)i / (LAUGH_EYE_POINTS - 1);
    int16_t x = (int16_t)(t * 90.0f * width_mod);
    float center_x = 45.0f * width_mod;
    float dx = (float)x - center_x;
    int16_t y = (int16_t)eye_dip - (int16_t)(bend_mod * dx * dx);

    sleep_eye_pts[i].x = x;
    sleep_eye_pts[i].y = y;
  }

  if (left_sleep_eye) {
    lv_line_set_points(left_sleep_eye, sleep_eye_pts, LAUGH_EYE_POINTS);
    lv_obj_set_style_line_width(left_sleep_eye, 24, 0);
    lv_obj_set_style_line_opa(left_sleep_eye, 160 + (int16_t)(40 * heave), 0);
  }
  if (right_sleep_eye) {
    lv_line_set_points(right_sleep_eye, sleep_eye_pts, LAUGH_EYE_POINTS);
    lv_obj_set_style_line_width(right_sleep_eye, 24, 0);
    lv_obj_set_style_line_opa(right_sleep_eye, 160 + (int16_t)(40 * heave), 0);
  }
}

static void update_angry_physics(int timer_ms, int16_t *out_shake_y,
                                 int16_t *out_zoom) {
  // 1. "Seething" Compression (Eyes shrink/constrict to focus)
  // ~ 2 second cycle
  float rage_cycle = sinf((float)timer_ms * 0.003f);

  // Constrict eyes: Scale down to range [200..230] (approx 0.8x)
  // Normal is 256. Smaller = More intense/focused anger.
  *out_zoom = 215 + (int16_t)(15.0f * rage_cycle);

  // 2. "Intensity" - replaced tremble with just stable state
  // No random shaking. Just focus on the zooming.
  *out_shake_y = 0;
}

static void update_happy_physics(float breath_rad, int16_t *out_bounce) {
  // 1. Organic Elastic Bounce (Slightly reduced translation to favor scaling)
  float s = sinf(breath_rad);
  // Power-sine creates a bubbly feel
  float bouncy_wave = powf(fabsf(s), 0.8f) * (s > 0 ? 1 : -1);
  *out_bounce = (int16_t)(6.0f * bouncy_wave); // Less sliding...

  // 2. STABLE EXPRESSION MANAGEMENT
  if (happy_open_mouth) {
    if (lv_obj_has_flag(happy_open_mouth, LV_OBJ_FLAG_HIDDEN)) {
      lv_obj_clear_flag(happy_open_mouth, LV_OBJ_FLAG_HIDDEN);
      if (happy_mouth)
        lv_obj_add_flag(happy_mouth, LV_OBJ_FLAG_HIDDEN);
    }

    // Unified Excitement Oscillator (Slightly slower for smoothness)
    // 3 pulses per breath cycle
    float pulse_phase = breath_rad * 3.0f;
    float pulse_val = 1.0f + 0.30f * sinf(pulse_phase); // Keep high amplitude

    // Smooth pulsation on the internal shape
    lv_obj_t *shape = lv_obj_get_child(happy_open_mouth, 0);
    if (shape) {
      int16_t s_size = (int16_t)(155 * pulse_val);
      lv_obj_set_size(shape, s_size, s_size);
      lv_obj_align(shape, LV_ALIGN_TOP_MID, 0, -s_size / 2);
    }
  }
}

static void update_love_physics(float intensity_scale, int16_t *out_zoom,
                                int16_t *out_float_y, int16_t *out_wobble) {
  // 1. CLEAR HEART RHYTHM (Lub-Dub..... Lub-Dub.....)
  // Phase speed: 12.0 per frame => ~30 frames/cycle => ~100 BPM
  heart_beat_phase += 15.0f;
  if (heart_beat_phase >= 360.0f)
    heart_beat_phase = 0.0f;

  float pulse_val = 0.0f;

  // LUB (First big beat): 0 - 60 degrees
  if (heart_beat_phase < 60.0f) {
    // 0..1..0 sine window
    float t = (heart_beat_phase / 60.0f) * 3.14159f;
    pulse_val = sinf(t); // Full amplitude
  }
  // DUB (Second small beat): 90 - 150 degrees
  else if (heart_beat_phase > 90.0f && heart_beat_phase < 150.0f) {
    float t = ((heart_beat_phase - 90.0f) / 60.0f) * 3.14159f;
    pulse_val = sinf(t) * 0.6f; // Smaller amplitude
  }
  // REST: 150 - 360 degrees (pulse_val = 0)

  float base_zoom = 256.0f;

  // Visuals
  // Growth: Static size increase based on intensity
  float growth = 50.0f * intensity_scale;

  // Dynamic Pulse: Snappy burst based on rhythm
  float pulse_amp = 60.0f * intensity_scale;

  *out_zoom = (int16_t)(base_zoom + growth + (pulse_amp * pulse_val));

  // Jump Effect: Eyes jump UP on beat
  // -10px on beat
  *out_float_y = -(int16_t)(10.0f * pulse_val * intensity_scale);

  // Impact Shake: Only shake on the beat Peak
  // Simple random-ish jitter sync with pulse
  if (pulse_val > 0.5f) {
    float jitter = (float)((int)heart_beat_phase % 3 - 1); // -1, 0, 1
    *out_wobble = (int16_t)(8.0f * jitter * intensity_scale);
  } else {
    *out_wobble = 0;
  }
}

// -----------------------------------------------------------------------------
// Position Update
// -----------------------------------------------------------------------------
static void update_positions(void) {
  breath_phase += 2;
  if (breath_phase >= 360)
    breath_phase = 0;

  // ==========================================
  // ANIMATION MANAGER UPDATE
  // ==========================================
  anim_manager_update();

  // Detect if Web/Custom Animation is playing to hide built-in eyes
  static bool was_anim_playing = false;
  bool is_anim_playing = anim_manager_is_playing();

  if (is_anim_playing) {
    if (!was_anim_playing) {
      // Transition to Custom Mode: Hide all built-in UI elements
      fade_out_normal_eyes();
      hide_happy();
      hide_sad();
      hide_laugh();
      hide_love();
      hide_sleep();
      // DO NOT call hide_angry() here if we ARE in EMO_ANGRY,
      // as it might contain logic we want to preserve or would interfere.
      if (current_emotion != EMO_ANGRY) {
        hide_angry();
        current_emotion = EMO_CUSTOM;
      }
      timer_ms = 0;
    }
    was_anim_playing = true;
    return; // SKIP PROCEDURAL EYE LOGIC
  } else if (was_anim_playing) {
    // Just finished custom animation
    was_anim_playing = false;

    // Don't auto-reset if we are in a multi-phase emotion like EMO_ANGRY
    if (current_emotion != EMO_ANGRY) {
      show_idle();
      current_emotion = EMO_IDLE;
      timer_ms = 0;
    }
  }

  // Global Breathe Wave (used by Normal, Happy, Sleep)
  float breath_rad = breath_phase * 0.01745f;

  // Normal Eye Breathe Offset
  float rad2 = breath_phase * 2.1f * 0.01745f;
  int16_t breath_offset =
      (int16_t)(3.2f * sinf(breath_rad) + 0.6f * sinf(rad2));

  // -----------------------------------------------------------------------------
  // 1. SPRING-DAMPER PHYSICS UPDATE (Gaze)
  // -----------------------------------------------------------------------------
  float force_x = (target_gaze_x - current_gaze_x) * GAZE_SPRING_K;
  float force_y = (target_gaze_y - current_gaze_y) * GAZE_SPRING_K;
  gaze_vel_x = (gaze_vel_x + force_x) * GAZE_DAMPING;
  gaze_vel_y = (gaze_vel_y + force_y) * GAZE_DAMPING;
  current_gaze_x += gaze_vel_x;
  current_gaze_y += gaze_vel_y;

  // Precision stop
  if (fabsf(gaze_vel_x) < 0.01f &&
      fabsf(target_gaze_x - current_gaze_x) < 0.01f) {
    current_gaze_x = target_gaze_x;
    gaze_vel_x = 0;
  }
  if (fabsf(gaze_vel_y) < 0.01f &&
      fabsf(target_gaze_y - current_gaze_y) < 0.01f) {
    current_gaze_y = target_gaze_y;
    gaze_vel_y = 0;
  }

  int16_t gaze_x = (int16_t)current_gaze_x;
  int16_t gaze_y = (int16_t)current_gaze_y;

  // -----------------------------------------------------------------------------
  // 2. Emotion Specific Logic
  // -----------------------------------------------------------------------------

  int16_t shake_x = 0, shake_y = 0;

  if (current_emotion == EMO_LAUGH) {
    if (laugh_intensity < 100)
      laugh_intensity += 6;
    float intensity_scale = laugh_intensity / 100.0f;

    laugh_shake_phase += 15; // Slower, smoother heavy laugh (was 25)
    if (laugh_shake_phase >= 360)
      laugh_shake_phase = 0;
    float phase_rad = laugh_shake_phase * 0.01745f;

    eye_pulse_phase += EYE_PULSE_SPEED;
    if (eye_pulse_phase >= 360.0f)
      eye_pulse_phase = 0.0f;

    // Update Helpers
    update_laugh_physics(phase_rad, intensity_scale);

    int16_t squint_offset = 0;
    update_laugh_mouth_and_shake(phase_rad, intensity_scale, &shake_x, &shake_y,
                                 &squint_offset);

    // Apply Positions
    if (left_laugh_eye)
      lv_obj_align(left_laugh_eye, LV_ALIGN_CENTER,
                   -EYE_OFFSET_X + gaze_x + shake_x - squint_offset,
                   LAUGH_EYE_OFFSET_Y + shake_y + gaze_y);
    if (right_laugh_eye)
      lv_obj_align(right_laugh_eye, LV_ALIGN_CENTER,
                   EYE_OFFSET_X + gaze_x + shake_x + squint_offset,
                   LAUGH_EYE_OFFSET_Y + shake_y + gaze_y);

    // Mouth Align
    if (big_laugh_mouth &&
        !lv_obj_has_flag(big_laugh_mouth, LV_OBJ_FLAG_HIDDEN))
      lv_obj_align(big_laugh_mouth, LV_ALIGN_CENTER, gaze_x + shake_x,
                   BIG_MOUTH_OFFSET_Y + shake_y + gaze_y);
    if (grin_laugh_mouth &&
        !lv_obj_has_flag(grin_laugh_mouth, LV_OBJ_FLAG_HIDDEN))
      lv_obj_align(grin_laugh_mouth, LV_ALIGN_CENTER, gaze_x + shake_x,
                   BIG_MOUTH_OFFSET_Y + shake_y + gaze_y);

  } else if (current_emotion == EMO_SLEEP) {
    float heave = 0;
    update_sleep_physics(breath_rad, &heave);

    // Apply Mouth Pos (already partly handled in helper but align needs gaze)
    if (sleep_mouth) {
      lv_obj_align(sleep_mouth, LV_ALIGN_CENTER, (int16_t)current_gaze_x,
                   100 + (int16_t)current_gaze_y + (int16_t)(heave * 5.0f));
    }

    // Apply Eye Pos
    if (left_sleep_eye)
      lv_obj_align(left_sleep_eye, LV_ALIGN_CENTER, -EYE_OFFSET_X + gaze_x,
                   breath_offset + gaze_y + 10);
    if (right_sleep_eye)
      lv_obj_align(right_sleep_eye, LV_ALIGN_CENTER, EYE_OFFSET_X + gaze_x,
                   breath_offset + gaze_y + 10);

  } else if (current_emotion == EMO_ANGRY) {
    // Only run procedural seething in Phase 0
    if (angry_phase != 0) {
      return;
    }

    int16_t zoom_val = 256;

    update_angry_physics(timer_ms, &shake_y, &zoom_val);

    // SMOOTH BLENDING (Pulse Only)
    // Cycle: Blue Zoom Pulse (No Red)
    // Speed increased significantly: approx 4Hz pulse
    float cycle_speed = 0.025f;
    float raw_wave = sinf((float)timer_ms * cycle_speed);

    // Sharpen the wave: Make it spend more time at extremes (Pulse)
    float blend_factor = (raw_wave + 1.0f) * 0.5f;
    blend_factor = blend_factor * blend_factor; // 0..1 non-linear

    // Smooth Zoom Interpolation
    // Blue Base: 220, Blue Max: 290
    int16_t current_zoom = 220 + (int16_t)(70.0f * blend_factor);

    // RED EYES DISABLED (Always Hidden/Transparent)
    if (left_angry_eye) {
      lv_obj_add_flag(left_angry_eye, LV_OBJ_FLAG_HIDDEN);
    }
    if (right_angry_eye) {
      lv_obj_add_flag(right_angry_eye, LV_OBJ_FLAG_HIDDEN);
    }

    // --- BLUE EYES (Always Visible + Pulsing Zoom) ---
    if (left_angry_blue_eye) {
      lv_obj_clear_flag(left_angry_blue_eye, LV_OBJ_FLAG_HIDDEN);
      lv_obj_align(left_angry_blue_eye, LV_ALIGN_CENTER, -EYE_OFFSET_X + gaze_x,
                   gaze_y + shake_y);
      lv_img_set_zoom(left_angry_blue_eye, current_zoom);
      lv_obj_set_style_img_opa(left_angry_blue_eye, 255, 0); // Full Opaque
    }
    if (right_angry_blue_eye) {
      lv_obj_clear_flag(right_angry_blue_eye, LV_OBJ_FLAG_HIDDEN);
      lv_obj_align(right_angry_blue_eye, LV_ALIGN_CENTER, EYE_OFFSET_X + gaze_x,
                   gaze_y + shake_y);
      lv_img_set_zoom(right_angry_blue_eye, current_zoom);
      lv_obj_set_style_img_opa(right_angry_blue_eye, 255, 0); // Full Opaque
    }

    // --- BLUE MOUTH (Morphing Shape) ---
    if (angry_blue_mouth) {
      lv_obj_clear_flag(angry_blue_mouth, LV_OBJ_FLAG_HIDDEN);
      lv_obj_align(angry_blue_mouth, LV_ALIGN_CENTER, gaze_x,
                   80 + gaze_y + shake_y);

      // Morph Shape: Clenched (Wide/Short) <-> Scream (Narrow/Tall)
      // Width: 90 -> 70
      // Height: 40 -> 80
      int16_t m_w = 90 - (int16_t)(20.0f * blend_factor);
      int16_t m_h = 40 + (int16_t)(40.0f * blend_factor);
      int16_t m_rad = 15 + (int16_t)(15.0f * blend_factor); // Rounder when open

      lv_obj_set_size(angry_blue_mouth, m_w, m_h);
      lv_obj_set_style_radius(angry_blue_mouth, m_rad, 0);

      // Ensure Reset Zoom (we use Size now)
      lv_obj_set_style_transform_zoom(angry_blue_mouth, 256, 0);
      lv_obj_set_style_bg_opa(angry_blue_mouth, 255, 0);
    }

  } else if (current_emotion == EMO_HAPPY) {
    // Speed up Happy Animation (2.5x speed)
    float fast_happy_rad = breath_rad * 2.5f;

    int16_t happy_bounce = 0;
    update_happy_physics(fast_happy_rad, &happy_bounce);

    if (happy_mouth) {
      lv_obj_align(happy_mouth, LV_ALIGN_CENTER, gaze_x,
                   OMEGA_MOUTH_OFFSET_Y + happy_bounce + gaze_y);
    }
    if (left_cheek)
      lv_obj_align(left_cheek, LV_ALIGN_CENTER, -EYE_OFFSET_X + gaze_x,
                   CHEEK_OFFSET_Y_VISIBLE + happy_bounce + gaze_y);
    if (right_cheek)
      lv_obj_align(right_cheek, LV_ALIGN_CENTER, EYE_OFFSET_X + gaze_x,
                   CHEEK_OFFSET_Y_VISIBLE + happy_bounce + gaze_y);

    // Normal Eyes with bounce
    static float happy_eye_h = 160.0f;
    static float happy_eye_w = 120.0f;

    if (left_eye) {
      // 1. Dynamic Target Height
      float target_h = (lv_obj_has_flag(happy_open_mouth, LV_OBJ_FLAG_HIDDEN))
                           ? 150.0f
                           : 240.0f;

      // 2. Synced Shape Morph (Squash & Stretch)
      // Pulsing synced with mouth (3.0f) -> Using fast_happy_rad
      float pulse_wave = sinf(fast_happy_rad * 3.0f);

      // Squash factors: +15% height expansion, -10% width constriction
      float h_pulse = 1.0f + 0.15f * pulse_wave;
      float w_pulse = 1.0f - 0.10f * pulse_wave;

      // 3. Smooth Evolution (Higher Lerp factor for responsiveness)
      happy_eye_h = happy_eye_h * 0.80f + (target_h * h_pulse) * 0.20f;
      happy_eye_w = happy_eye_w * 0.80f + (EYE_WIDTH * w_pulse) * 0.20f;

      int16_t final_h = (int16_t)happy_eye_h;
      int16_t final_w = (int16_t)happy_eye_w;

      // OPTIMIZATION: Check before setting size to avoid expensive re-layouts
      if (lv_obj_get_width(left_eye) != final_w ||
          lv_obj_get_height(left_eye) != final_h) {
        lv_obj_set_size(left_eye, final_w, final_h);
        lv_obj_set_size(right_eye, final_w, final_h);
      }

      // REMOVED: Expensive per-frame radius calculation
      // Radius is set to LV_RADIUS_CIRCLE in init, which handles capsule shape
      // automatically.

      lv_obj_align(left_eye, LV_ALIGN_CENTER, -EYE_OFFSET_X + gaze_x,
                   breath_offset + gaze_y + happy_bounce);
      lv_obj_align(right_eye, LV_ALIGN_CENTER, EYE_OFFSET_X + gaze_x,
                   breath_offset + gaze_y + happy_bounce);
    }

    if (happy_open_mouth &&
        !lv_obj_has_flag(happy_open_mouth, LV_OBJ_FLAG_HIDDEN)) {
      lv_obj_align(happy_open_mouth, LV_ALIGN_CENTER, gaze_x,
                   OMEGA_MOUTH_OFFSET_Y + happy_bounce + gaze_y + 10);
    }

  } else if (current_emotion == EMO_LOVE) {
    if (love_intensity < 100)
      love_intensity += 10; // Very Fast ramp (0.3s to max)
    float intensity_scale = love_intensity / 100.0f;

    int16_t zoom_val = 256;
    int16_t float_y = 0;
    int16_t wobble = 0;
    update_love_physics(intensity_scale, &zoom_val, &float_y, &wobble);

    if (left_heart_eye) {
      lv_obj_align(left_heart_eye, LV_ALIGN_CENTER,
                   -HEART_EYE_OFFSET_X + gaze_x - wobble, gaze_y + float_y);
      lv_img_set_zoom(left_heart_eye, zoom_val);
    }
    if (right_heart_eye) {
      lv_obj_align(right_heart_eye, LV_ALIGN_CENTER,
                   HEART_EYE_OFFSET_X + gaze_x + wobble, gaze_y + float_y);
      lv_img_set_zoom(right_heart_eye, zoom_val);
    }

  } else {
    // EMO_IDLE or others (Normal)
    laugh_intensity = 0;
    love_intensity = 0;

    if (left_eye && right_eye &&
        !lv_obj_has_flag(left_eye, LV_OBJ_FLAG_HIDDEN)) {

      // 1. EMO-LIKE DYNAMIC PHYSICS
      float deform_factor = 0.4f; // How "jelly-like" the eyes are

      // Base Breath LFO
      float breath_val = sinf(breath_rad); // -1 to 1

      // 2. Velocity Stretching (Motion Blur effect)
      // If moving X fast -> Widen, Shorten
      // If moving Y fast -> Tall, Narrow
      float stretch_x = 1.0f + (fabsf(gaze_vel_x) * 0.015f * deform_factor);
      float stretch_y = 1.0f + (fabsf(gaze_vel_y) * 0.015f * deform_factor);

      // Conserve area roughly: If stretching one way, squash the other
      if (stretch_x > 1.05f)
        stretch_y *= (1.0f / stretch_x);
      if (stretch_y > 1.05f)
        stretch_x *= (1.0f / stretch_y);

      // Combine with Breath
      int16_t target_w =
          (int16_t)(EYE_WIDTH * stretch_x) - (int16_t)(6.0f * breath_val);
      int16_t target_h =
          (int16_t)(EYE_HEIGHT * stretch_y) + (int16_t)(6.0f * breath_val);

      // Apply SMOOTHING (Lerp)
      // Faster LERP (0.3) to avoid "drag" feeling, but kept for jitter removal
      current_eye_w = current_eye_w * 0.7f + ((float)target_w * 0.3f);
      current_eye_h = current_eye_h * 0.7f + ((float)target_h * 0.3f);

      int16_t final_w = (int16_t)current_eye_w;
      int16_t final_h = (int16_t)current_eye_h;

      // Optimization: Only update if changed to save FPS
      if (lv_obj_get_width(left_eye) != final_w ||
          lv_obj_get_height(left_eye) != final_h) {
        lv_obj_set_size(left_eye, final_w, final_h);
        lv_obj_set_size(right_eye, final_w, final_h);
      }

      // 3. 3D Rolling Effect -> REMOVED for Performance (FPS Drop fix)
      // Rotation on software renderer is very heavy.
      // We rely on Squash/Stretch + Position for 3D feel.

      lv_obj_align(left_eye, LV_ALIGN_CENTER, -EYE_OFFSET_X + gaze_x,
                   breath_offset + gaze_y);
      lv_obj_align(right_eye, LV_ALIGN_CENTER, EYE_OFFSET_X + gaze_x,
                   breath_offset + gaze_y);

      // Reset rotation just in case it was set by other modes
      lv_obj_set_style_transform_angle(left_eye, 0, 0);
      lv_obj_set_style_transform_angle(right_eye, 0, 0);

      // 4. RANDOM IDLE ACTIONS (Ticks)
      if (idle_anim_type != IDLE_ACT_NONE) {
        float act_val = 0.0f;
        // Simple bell curve 0->1->0 based on phase
        if (idle_anim_phase < 0.5f)
          act_val = idle_anim_phase * 2.0f;
        else
          act_val = (1.0f - idle_anim_phase) * 2.0f;

        // Smooth sine ease
        act_val = sinf(act_val * 1.5707f);

        if (idle_anim_type == IDLE_ACT_SQUINT) {
          // Squint: Shorten height, slightly wider
          int16_t sq_h = final_h - (int16_t)(40.0f * act_val);
          int16_t sq_w = final_w + (int16_t)(10.0f * act_val);
          lv_obj_set_size(left_eye, sq_w, sq_h);
          lv_obj_set_size(right_eye, sq_w, sq_h);
        } else if (idle_anim_type == IDLE_ACT_WIDE) {
          // Surprise: Taller and Wider
          int16_t wide_h = final_h + (int16_t)(30.0f * act_val);
          int16_t wide_w = final_w + (int16_t)(20.0f * act_val);
          lv_obj_set_size(left_eye, wide_w, wide_h);
          lv_obj_set_size(right_eye, wide_w, wide_h);
        } else if (idle_anim_type == IDLE_ACT_WINK) {
          // Wink: Only left eye squints
          int16_t sq_h = final_h - (int16_t)(EYE_HEIGHT * 0.8f * act_val);
          lv_obj_set_size(left_eye, final_w, sq_h);
          lv_obj_set_size(right_eye, final_w, final_h);
        } else if (idle_anim_type == IDLE_ACT_THINK) {
          // Thinking: Slight squint both eyes
          int16_t sq_h = final_h - (int16_t)(20.0f * act_val);
          lv_obj_set_size(left_eye, final_w, sq_h);
          lv_obj_set_size(right_eye, final_w, sq_h);
        } else if (idle_anim_type == IDLE_ACT_ROLL) {
          // Rol: Slight "dizzy" squash
          int16_t sq_h = final_h - (int16_t)(15.0f * act_val);
          int16_t sq_w = final_w + (int16_t)(10.0f * act_val);
          lv_obj_set_size(left_eye, sq_w, sq_h);
          lv_obj_set_size(right_eye, sq_w, sq_h);
        } else if (idle_anim_type == IDLE_ACT_SHIVER) {
          // Shiver: Rapid micro-scale jitter
          float jitter = (sinf(idle_anim_phase * 50.0f) * 4.0f);
          lv_obj_set_size(left_eye, final_w + (int16_t)jitter,
                          final_h - (int16_t)jitter);
          lv_obj_set_size(right_eye, final_w - (int16_t)jitter,
                          final_h + (int16_t)jitter);
        } else if (idle_anim_type == IDLE_ACT_BOUNCE) {
          // Bounce: Vertical "jump" effect
          float jump = sinf(act_val * 3.14f) * 25.0f;
          lv_obj_align(left_eye, LV_ALIGN_CENTER, -EYE_OFFSET_X + gaze_x,
                       breath_offset + gaze_y - (int16_t)jump);
          lv_obj_align(right_eye, LV_ALIGN_CENTER, EYE_OFFSET_X + gaze_x,
                       breath_offset + gaze_y - (int16_t)jump);
          // Squash when landing
          if (act_val > 0.8f) {
            int16_t squash = (int16_t)((act_val - 0.8f) * 50.0f);
            lv_obj_set_size(left_eye, final_w + squash, final_h - squash);
            lv_obj_set_size(right_eye, final_w + squash, final_h - squash);
          }
        } else if (idle_anim_type == IDLE_ACT_TILT) {
          // Tilt: Simulate head tilt by vertical offset
          float tilt = sinf(act_val * 3.14f) * 15.0f;
          lv_obj_align(left_eye, LV_ALIGN_CENTER, -EYE_OFFSET_X + gaze_x,
                       breath_offset + gaze_y - (int16_t)tilt);
          lv_obj_align(right_eye, LV_ALIGN_CENTER, EYE_OFFSET_X + gaze_x,
                       breath_offset + gaze_y + (int16_t)tilt);
        } else if (idle_anim_type == IDLE_ACT_CURIOUS) {
          // Curious: Tilted squint
          float tilt = sinf(act_val * 3.14f) * 10.0f;
          int16_t sq_h = final_h - (int16_t)(15.0f * act_val);
          lv_obj_set_size(left_eye, final_w, sq_h);
          lv_obj_set_size(right_eye, final_w, sq_h);
          lv_obj_align(left_eye, LV_ALIGN_CENTER,
                       -EYE_OFFSET_X + gaze_x - (int16_t)tilt,
                       breath_offset + gaze_y);
          lv_obj_align(right_eye, LV_ALIGN_CENTER,
                       EYE_OFFSET_X + gaze_x - (int16_t)tilt,
                       breath_offset + gaze_y);
        }
        // SCAN handled via gaze target in main loop, no deform needed here
      }
    }
  }
}

// -----------------------------------------------------------------------------
// Animations & Transitions
// -----------------------------------------------------------------------------
static void anim_opa_cb(void *var, int32_t v) {
  lv_obj_set_style_opa((lv_obj_t *)var, v, 0);
}

static void anim_line_opa_cb(void *var, int32_t v) {
  lv_obj_set_style_line_opa((lv_obj_t *)var, v, 0);
}

static void anim_img_opa_cb(void *var, int32_t v) {
  lv_obj_set_style_img_opa((lv_obj_t *)var, v, 0);
}

static void fade_out_anim_ready_cb(lv_anim_t *a) {
  lv_obj_add_flag((lv_obj_t *)a->var, LV_OBJ_FLAG_HIDDEN);
}

// Fade IN Normal Eyes (if hidden)
static void fade_in_normal_eyes(void) {
  if (!left_eye || !right_eye)
    return;

  // Only fade in if hidden or transparent
  if (!lv_obj_has_flag(left_eye, LV_OBJ_FLAG_HIDDEN) &&
      lv_obj_get_style_opa(left_eye, 0) == LV_OPA_COVER)
    return;

  lv_obj_clear_flag(left_eye, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(right_eye, LV_OBJ_FLAG_HIDDEN);

  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, left_eye);
  lv_anim_set_values(&a, 0, 255);
  lv_anim_set_time(&a, 300);
  lv_anim_set_exec_cb(&a, anim_opa_cb);
  lv_anim_start(&a);

  lv_anim_set_var(&a, right_eye);
  lv_anim_start(&a);
}

// Fade OUT Normal Eyes (if visible)
static void fade_out_normal_eyes(void) {
  if (!left_eye || !right_eye)
    return;

  if (lv_obj_has_flag(left_eye, LV_OBJ_FLAG_HIDDEN))
    return;

  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, left_eye);
  lv_anim_set_values(&a, 255, 0);
  lv_anim_set_time(&a, 300);
  lv_anim_set_exec_cb(&a, anim_opa_cb);
  lv_anim_set_ready_cb(&a, fade_out_anim_ready_cb);
  lv_anim_start(&a);

  lv_anim_set_var(&a, right_eye);
  lv_anim_start(&a);
}

static void anim_height_cb(void *var, int32_t v) {
  if (left_eye)
    lv_obj_set_height(left_eye, v);
  if (right_eye)
    lv_obj_set_height(right_eye, v);
}

// Helper for Jelly Width during blink
static void anim_blink_width_cb(void *var, int32_t v) {
  lv_obj_set_width((lv_obj_t *)var, v);
}

void do_blink(void) {
  if (!left_eye || !right_eye)
    return;
  // Don't blink if not in a state that supports it (Custom anim has its own)
  if (current_emotion == EMO_CUSTOM || current_emotion == EMO_SLEEP ||
      current_emotion == EMO_LAUGH)
    return;

  // 1. ASYNC DELAY: One eye blinks slightly after the other (Organic feel)
  int delay_left = (rand() % 30);
  int delay_right = (rand() % 30);

  // 2. JELLY PHYSICS: Eye gets wider as it gets shorter
  uint16_t current_w = lv_obj_get_width(left_eye);
  uint16_t current_h = lv_obj_get_height(left_eye);
  uint16_t jelly_w = current_w + 15;

  lv_anim_t a_h, a_w;

  // HEIGHT ANIM (Closing is fast 80ms, Opening is smooth 150ms)
  lv_anim_init(&a_h);
  lv_anim_set_values(&a_h, current_h, 5);
  lv_anim_set_time(&a_h, 70);
  lv_anim_set_playback_time(&a_h, 150);
  lv_anim_set_path_cb(&a_h, lv_anim_path_ease_in); // Close fast
  // Opening use overshoot for life!
  // Note: LVGL internal playback uses the same path usually, but we want
  // different feel. For simplicity in one call, we use overshoot.
  lv_anim_set_exec_cb(&a_h, anim_height_cb);

  // WIDTH ANIM (Jelly bulge)
  lv_anim_init(&a_w);
  lv_anim_set_values(&a_w, current_w, jelly_w);
  lv_anim_set_time(&a_w, 70);
  lv_anim_set_playback_time(&a_w, 150);
  lv_anim_set_exec_cb(&a_w, anim_blink_width_cb);

  // Apply Left
  lv_anim_set_var(&a_h, left_eye);
  lv_anim_set_delay(&a_h, delay_left);
  lv_anim_start(&a_h);
  lv_anim_set_var(&a_w, left_eye);
  lv_anim_set_delay(&a_w, delay_left);
  lv_anim_start(&a_w);

  // Apply Right
  lv_anim_set_var(&a_h, right_eye);
  lv_anim_set_delay(&a_h, delay_right);
  lv_anim_start(&a_h);
  lv_anim_set_var(&a_w, right_eye);
  lv_anim_set_delay(&a_w, delay_right);
  lv_anim_start(&a_w);
}

static void anim_cheek_cb(void *var, int32_t v) {
  if (left_cheek)
    lv_obj_align(left_cheek, LV_ALIGN_CENTER, -EYE_OFFSET_X, v);
  if (right_cheek)
    lv_obj_align(right_cheek, LV_ALIGN_CENTER, EYE_OFFSET_X, v);
}

// (Old unused mouth animation helpers removed)

static void show_happy(void) {
  fade_in_normal_eyes(); // Ensure eyes are visible

  // Cheeks Slide Up
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, NULL);
  lv_anim_set_values(&a, CHEEK_OFFSET_Y_HIDDEN, CHEEK_OFFSET_Y_VISIBLE);
  lv_anim_set_time(&a, 400);
  lv_anim_set_exec_cb(&a, anim_cheek_cb);
  lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
  lv_anim_start(&a);

  // Show Mouths (Update positions will handle visibility and alignment
  // immediately)
  if (happy_mouth) {
    lv_obj_clear_flag(happy_mouth, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_line_opa(happy_mouth, LV_OPA_COVER, 0);
  }
  if (happy_open_mouth) {
    // Keep it hidden initially if cycle says so, but ensure opa is set
    lv_obj_set_style_opa(happy_open_mouth, LV_OPA_COVER, 0);
    // update_happy_physics will decide HIDDEN flag in the next frame (16ms)
  }
}

// (Old unused mouth helper removed)

static void hide_happy(void) {
  // Hide Cheeks
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, NULL);
  lv_anim_set_values(&a, CHEEK_OFFSET_Y_VISIBLE, CHEEK_OFFSET_Y_HIDDEN);
  lv_anim_set_time(&a, 300);
  lv_anim_set_exec_cb(&a, anim_cheek_cb);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
  lv_anim_start(&a);

  if (happy_mouth)
    lv_obj_add_flag(happy_mouth, LV_OBJ_FLAG_HIDDEN);
  if (happy_open_mouth)
    lv_obj_add_flag(happy_open_mouth, LV_OBJ_FLAG_HIDDEN);
}

static void anim_sad_mask_cb(void *var, int32_t v) {
  if (left_sad_mask)
    lv_obj_align(left_sad_mask, LV_ALIGN_CENTER,
                 -EYE_OFFSET_X - SAD_MASK_OFFSET_X, v);
  if (right_sad_mask)
    lv_obj_align(right_sad_mask, LV_ALIGN_CENTER,
                 EYE_OFFSET_X + SAD_MASK_OFFSET_X, v);
}

static void anim_tear_y_cb(void *var, int32_t v) {
  lv_obj_t *tear = (lv_obj_t *)var;
  if (!tear)
    return;
  // Use target_gaze because tears might not need smooth look (or use
  // current_gaze casted)
  int16_t curr_x = (int16_t)current_gaze_x;
  int16_t x = (tear == left_tear) ? (-EYE_OFFSET_X + TEAR_OFFSET_X + curr_x)
                                  : (EYE_OFFSET_X - TEAR_OFFSET_X + curr_x);
  lv_obj_align(tear, LV_ALIGN_CENTER, x, v);
}

static void anim_tear_opa_cb(void *var, int32_t v) {
  // Fade the image object directly
  lv_obj_t *obj = (lv_obj_t *)var;
  if (obj)
    lv_obj_set_style_img_opa(obj, v, 0);
}

static void start_tear_anim(lv_obj_t *tear, uint32_t delay) {
  if (!tear)
    return;
  int16_t curr_x = (int16_t)current_gaze_x;
  int16_t x = (tear == left_tear) ? (-EYE_OFFSET_X + TEAR_OFFSET_X + curr_x)
                                  : (EYE_OFFSET_X - TEAR_OFFSET_X + curr_x);
  lv_obj_align(tear, LV_ALIGN_CENTER, x, TEAR_START_Y);
  lv_obj_clear_flag(tear, LV_OBJ_FLAG_HIDDEN);

  anim_tear_opa_cb(tear, LV_OPA_COVER);

  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, tear);
  lv_anim_set_values(&a, TEAR_START_Y, TEAR_END_Y);
  lv_anim_set_time(&a, 1000);
  lv_anim_set_delay(&a, delay);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_exec_cb(&a, anim_tear_y_cb);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
  lv_anim_start(&a);

  lv_anim_t b;
  lv_anim_init(&b);
  lv_anim_set_var(&b, tear);
  lv_anim_set_values(&b, LV_OPA_COVER, LV_OPA_TRANSP);
  lv_anim_set_time(&b, 1000);
  lv_anim_set_delay(&b, delay);
  lv_anim_set_repeat_count(&b, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_exec_cb(&b, anim_tear_opa_cb);
  lv_anim_start(&b);
}

static void show_sad(void) {
  fade_in_normal_eyes();

  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, NULL);
  lv_anim_set_values(&a, SAD_MASK_OFFSET_Y_HIDDEN, SAD_MASK_OFFSET_Y_VISIBLE);
  lv_anim_set_time(&a, 400);
  lv_anim_set_exec_cb(&a, anim_sad_mask_cb);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
  lv_anim_start(&a);

  lv_anim_set_values(&a, EYE_HEIGHT, EYE_HEIGHT_SAD);
  lv_anim_set_exec_cb(&a, anim_height_cb);
  lv_anim_start(&a);

  start_tear_anim(left_tear, 0);
  start_tear_anim(right_tear, 400);
}

static void hide_sad(void) {
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, NULL);
  lv_anim_set_values(&a, SAD_MASK_OFFSET_Y_VISIBLE, SAD_MASK_OFFSET_Y_HIDDEN);
  lv_anim_set_time(&a, 300);
  lv_anim_set_exec_cb(&a, anim_sad_mask_cb);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
  lv_anim_start(&a);

  lv_anim_set_values(&a, EYE_HEIGHT_SAD, EYE_HEIGHT);
  lv_anim_set_exec_cb(&a, anim_height_cb);
  lv_anim_start(&a);

  if (left_tear) {
    lv_anim_del(left_tear, anim_tear_y_cb);
    lv_anim_del(left_tear, anim_tear_opa_cb);
    lv_obj_add_flag(left_tear, LV_OBJ_FLAG_HIDDEN);
  }
  if (right_tear) {
    lv_anim_del(right_tear, anim_tear_y_cb);
    lv_anim_del(right_tear, anim_tear_opa_cb);
    lv_obj_add_flag(right_tear, LV_OBJ_FLAG_HIDDEN);
  }
}

static void anim_laugh_eye_scale_cb(void *var, int32_t v) {
  // Scale the line style width to create squint effect
  if (left_laugh_eye)
    lv_obj_set_style_line_width(left_laugh_eye, v, 0);
  if (right_laugh_eye)
    lv_obj_set_style_line_width(right_laugh_eye, v, 0);
}

static void show_laugh(void) {
  // Reset laugh state
  laugh_intensity = 0;
  laugh_cycle = 0;
  laugh_shake_phase = 0;

  fade_out_normal_eyes(); // Fade out normal eyes smoothly

  // Show laugh eyes with animation
  if (left_laugh_eye) {
    lv_obj_set_style_line_width(left_laugh_eye, 4, 0); // Start thin
    lv_obj_set_style_line_opa(left_laugh_eye, LV_OPA_TRANSP,
                              0); // Correct property (was opa)
    lv_obj_clear_flag(left_laugh_eye, LV_OBJ_FLAG_HIDDEN);

    // Fade In Left
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, left_laugh_eye);
    lv_anim_set_values(&a, 0, 255);
    lv_anim_set_time(&a, 300);
    lv_anim_set_exec_cb(&a, anim_line_opa_cb); // Line
    lv_anim_start(&a);
  }
  if (right_laugh_eye) {
    lv_obj_set_style_line_width(right_laugh_eye, 4, 0);
    lv_obj_set_style_line_opa(right_laugh_eye, LV_OPA_TRANSP,
                              0); // Correct property
    lv_obj_clear_flag(right_laugh_eye, LV_OBJ_FLAG_HIDDEN);

    // Fade In Right
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, right_laugh_eye);
    lv_anim_set_values(&a, 0, 255);
    lv_anim_set_time(&a, 300);
    lv_anim_set_exec_cb(&a, anim_line_opa_cb); // Line
    lv_anim_start(&a);
  }

  // Animate eyes getting thicker (squint effect)
  lv_anim_t eye_anim;
  lv_anim_init(&eye_anim);
  lv_anim_set_var(&eye_anim, NULL);
  lv_anim_set_values(&eye_anim, 4, 14);
  lv_anim_set_time(&eye_anim, 200); // Wait for fade a bit?
  lv_anim_set_exec_cb(&eye_anim, anim_laugh_eye_scale_cb);
  lv_anim_set_path_cb(&eye_anim, lv_anim_path_overshoot);
  lv_anim_start(&eye_anim);

  // Show BIG MOUTH with fade in
  if (big_laugh_mouth) {
    lv_obj_set_style_bg_opa(big_laugh_mouth, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(big_laugh_mouth, LV_OBJ_FLAG_HIDDEN);

    lv_anim_t mouth_anim;
    lv_anim_init(&mouth_anim);
    lv_anim_set_var(&mouth_anim, big_laugh_mouth); // Fix Var
    lv_anim_set_values(&mouth_anim, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&mouth_anim, 300);
    lv_anim_set_exec_cb(&mouth_anim, anim_opa_cb); // Generic
    lv_anim_set_path_cb(&mouth_anim, lv_anim_path_ease_in);
    lv_anim_start(&mouth_anim);
  }
}

static void laugh_exit_anim_done(lv_anim_t *a) {
  // Hide laugh elements after animation
  if (left_laugh_eye)
    lv_obj_add_flag(left_laugh_eye, LV_OBJ_FLAG_HIDDEN);
  if (right_laugh_eye)
    lv_obj_add_flag(right_laugh_eye, LV_OBJ_FLAG_HIDDEN);
  if (big_laugh_mouth)
    lv_obj_add_flag(big_laugh_mouth, LV_OBJ_FLAG_HIDDEN);

  // NOTE: We do NOT restore Normal eyes here anymore.
  // The next emotion's show function handles that.
}

static void hide_laugh(void) {
  // Fade out BIG MOUTH
  if (big_laugh_mouth) {
    lv_anim_t mouth_anim;
    lv_anim_init(&mouth_anim);
    lv_anim_set_var(&mouth_anim, big_laugh_mouth);
    lv_anim_set_values(&mouth_anim, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&mouth_anim, 200);
    lv_anim_set_exec_cb(&mouth_anim, anim_opa_cb);
    lv_anim_set_ready_cb(&mouth_anim, laugh_exit_anim_done);
    lv_anim_set_path_cb(&mouth_anim, lv_anim_path_ease_in);
    lv_anim_start(&mouth_anim);
  } else {
    laugh_exit_anim_done(NULL);
  }

  if (grin_laugh_mouth)
    lv_obj_add_flag(grin_laugh_mouth, LV_OBJ_FLAG_HIDDEN);

  // Fade out Eyes
  if (left_laugh_eye) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, left_laugh_eye);
    lv_anim_set_values(&a, 255, 0);
    lv_anim_set_time(&a, 200);
    lv_anim_set_exec_cb(&a, anim_line_opa_cb);
    lv_anim_start(&a);
  }
  if (right_laugh_eye) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, right_laugh_eye);
    lv_anim_set_values(&a, 255, 0);
    lv_anim_set_time(&a, 200);
    lv_anim_set_exec_cb(&a, anim_line_opa_cb);
    lv_anim_start(&a);
  }
}

// -----------------------------------------------------------------------------
// ANGRY Emotion - Sharp glare and Trembling
// -----------------------------------------------------------------------------
static void show_angry(void) {
  fade_out_normal_eyes();

  // Red eyes removed - ensure hidden
  if (left_angry_eye)
    lv_obj_add_flag(left_angry_eye, LV_OBJ_FLAG_HIDDEN);
  if (right_angry_eye)
    lv_obj_add_flag(right_angry_eye, LV_OBJ_FLAG_HIDDEN);

  // Directly show Blue Eyes/Mouth (Fade In can be added if needed, but pulsing
  // starts immediately) For now, let update_positions handle the
  // visibility/opacity to avoid conflict
}

static void hide_angry(void) {
  // Hide Blue procedural components
  if (left_angry_blue_eye)
    lv_obj_add_flag(left_angry_blue_eye, LV_OBJ_FLAG_HIDDEN);
  if (right_angry_blue_eye)
    lv_obj_add_flag(right_angry_blue_eye, LV_OBJ_FLAG_HIDDEN);
  if (angry_blue_mouth)
    lv_obj_add_flag(angry_blue_mouth, LV_OBJ_FLAG_HIDDEN);

  // Restore normal eyes if they were hidden for the vector burst
  if (left_eye)
    lv_obj_clear_flag(left_eye, LV_OBJ_FLAG_HIDDEN);
  if (right_eye)
    lv_obj_clear_flag(right_eye, LV_OBJ_FLAG_HIDDEN);
}

static void anim_heart_scale_cb(void *var, int32_t v) {
  // Scale both heart eyes together
  int16_t size = (int16_t)v;
  if (left_heart_eye) {
    lv_img_set_zoom(left_heart_eye, size);
  }
  if (right_heart_eye) {
    lv_img_set_zoom(right_heart_eye, size);
  }
}

static void show_love(void) {
  // Reset love state
  love_intensity = 0;
  heart_beat_phase = 0.0f;

  fade_out_normal_eyes();

  // Show heart eyes with fade-in animation
  if (left_heart_eye) {
    lv_obj_set_style_img_opa(left_heart_eye, LV_OPA_TRANSP, 0);
    lv_img_set_zoom(left_heart_eye, 180); // Start at 70%
    lv_obj_clear_flag(left_heart_eye, LV_OBJ_FLAG_HIDDEN);
  }
  if (right_heart_eye) {
    lv_obj_set_style_img_opa(right_heart_eye, LV_OPA_TRANSP, 0);
    lv_img_set_zoom(right_heart_eye, 180);
    lv_obj_clear_flag(right_heart_eye, LV_OBJ_FLAG_HIDDEN);
  }

  // Animate opacity (fade in)
  lv_anim_t opa_anim;
  lv_anim_init(&opa_anim);
  lv_anim_set_var(&opa_anim, left_heart_eye); // Fix var
  lv_anim_set_values(&opa_anim, LV_OPA_TRANSP, LV_OPA_COVER);
  lv_anim_set_time(&opa_anim, 300);
  lv_anim_set_exec_cb(&opa_anim, anim_img_opa_cb); // Use Image Opa
  lv_anim_set_path_cb(&opa_anim, lv_anim_path_ease_out);
  lv_anim_start(&opa_anim);

  lv_anim_set_var(&opa_anim, right_heart_eye);
  lv_anim_start(&opa_anim);

  // Animate scale (pop in effect)
  lv_anim_t scale_anim;
  lv_anim_init(&scale_anim);
  lv_anim_set_var(&scale_anim, NULL);
  lv_anim_set_values(&scale_anim, 180, 280); // 70% to 110%
  lv_anim_set_time(&scale_anim, 400);
  lv_anim_set_exec_cb(&scale_anim, anim_heart_scale_cb);
  lv_anim_set_path_cb(&scale_anim, lv_anim_path_overshoot);
  lv_anim_start(&scale_anim);
}

static void hide_love(void) {
  // Hide hearts immediately to prevent bleeding into Sleep screen
  if (left_heart_eye) {
    lv_anim_del(left_heart_eye, anim_img_opa_cb);
    lv_anim_del(left_heart_eye, anim_heart_scale_cb);
    lv_obj_add_flag(left_heart_eye, LV_OBJ_FLAG_HIDDEN);
  }
  if (right_heart_eye) {
    lv_anim_del(right_heart_eye, anim_img_opa_cb);
    // Scale anim var was NULL in start, but best to just force hide
    lv_obj_add_flag(right_heart_eye, LV_OBJ_FLAG_HIDDEN);
  }
}

// -----------------------------------------------------------------------------
// SLEEP Emotion - 'zzz' Animation
// -----------------------------------------------------------------------------
// Helper to create Z image for Sleep mode (so we can Rotate/Zoom it)
// Zzz Animation Callback (For Labels - REVERTED TO ROBUST LABELS)
static void anim_z_cb(void *var, int32_t v) {
  lv_obj_t *z = (lv_obj_t *)var;
  if (!z)
    return;

  // v: 0 to 100
  float p = v / 100.0f;

  // Fly up from CENTER
  int16_t start_y = 0;
  int16_t end_y = -200; // Fly up high
  int16_t start_x = 0;  // Dead center horizontally

  // Drift slightly right
  int16_t end_x = 40;

  int16_t sway = (int16_t)(30.0f * sinf(p * 8.0f));

  int16_t curr_y = start_y + (int16_t)((end_y - start_y) * p);
  int16_t curr_x = start_x + (int16_t)((end_x - start_x) * p) + sway;

  lv_obj_align(z, LV_ALIGN_CENTER, curr_x, curr_y);

  // Fade In/Out
  // Zoom and Rotation removed as they often fail on Labels without specific
  // config
  if (p < 0.2f) {
    lv_obj_set_style_text_opa(z, (uint8_t)(255 * (p / 0.2f)), 0);
  } else if (p > 0.7f) {
    lv_obj_set_style_text_opa(z, (uint8_t)(255 * (1.0f - (p - 0.7f) / 0.3f)),
                              0);
  } else {
    lv_obj_set_style_text_opa(z, 255, 0);
  }
}

static void start_z_anim(lv_obj_t *z, uint32_t delay) {
  if (!z)
    return;

  // FORCE TO FRONT: Ensure it's not hidden behind eyes/bg
  lv_obj_move_foreground(z);
  lv_obj_clear_flag(z, LV_OBJ_FLAG_HIDDEN);

  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, z);
  lv_anim_set_values(&a, 0, 100);
  lv_anim_set_time(&a, 1500); // Faster float up
  lv_anim_set_delay(&a, delay);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_exec_cb(&a, anim_z_cb);
  lv_anim_set_path_cb(&a, lv_anim_path_linear);
  lv_anim_start(&a);
}

static void show_sleep(void) {
  fade_out_normal_eyes();

  if (left_sleep_eye) {
    lv_obj_set_style_line_opa(left_sleep_eye, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(left_sleep_eye, LV_OBJ_FLAG_HIDDEN);

    // Fade In
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, left_sleep_eye);
    lv_anim_set_values(&a, 0, 255);
    lv_anim_set_time(&a, 400);
    lv_anim_set_exec_cb(&a, anim_line_opa_cb);
    lv_anim_start(&a);
  }
  if (right_sleep_eye) {
    lv_obj_set_style_line_opa(right_sleep_eye, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(right_sleep_eye, LV_OBJ_FLAG_HIDDEN);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, right_sleep_eye);
    lv_anim_set_values(&a, 0, 255);
    lv_anim_set_time(&a, 400);
    lv_anim_set_exec_cb(&a, anim_line_opa_cb);
    lv_anim_start(&a);
  }
  if (sleep_mouth)
    lv_obj_clear_flag(sleep_mouth, LV_OBJ_FLAG_HIDDEN);

  // We move foreground inside start_z_anim now for safety,
  // but keeping explicit calls here doesn't hurt.
  start_z_anim(sleep_z1, 0);
  start_z_anim(sleep_z2, 800);
  start_z_anim(sleep_z3, 1600);
}

static void hide_sleep(void) {
  if (left_sleep_eye) {
    // Fade Out
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, left_sleep_eye);
    lv_anim_set_values(&a, 255, 0);
    lv_anim_set_time(&a, 250);
    lv_anim_set_exec_cb(&a, anim_line_opa_cb);
    lv_anim_set_ready_cb(&a, fade_out_anim_ready_cb);
    lv_anim_start(&a);
  }
  if (right_sleep_eye) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, right_sleep_eye);
    lv_anim_set_values(&a, 255, 0);
    lv_anim_set_time(&a, 250);
    lv_anim_set_exec_cb(&a, anim_line_opa_cb);
    lv_anim_set_ready_cb(&a, fade_out_anim_ready_cb);
    lv_anim_start(&a);
  }
  if (sleep_mouth)
    lv_obj_add_flag(sleep_mouth, LV_OBJ_FLAG_HIDDEN);

  if (sleep_z1) {
    lv_anim_del(sleep_z1, anim_z_cb);
    lv_obj_add_flag(sleep_z1, LV_OBJ_FLAG_HIDDEN);
  }
  if (sleep_z2) {
    lv_anim_del(sleep_z2, anim_z_cb);
    lv_obj_add_flag(sleep_z2, LV_OBJ_FLAG_HIDDEN);
  }
  if (sleep_z3) {
    lv_anim_del(sleep_z3, anim_z_cb);
    lv_obj_add_flag(sleep_z3, LV_OBJ_FLAG_HIDDEN);
  }

  if (sleep_z3) {
    lv_anim_del(sleep_z3, anim_z_cb);
    lv_obj_add_flag(sleep_z3, LV_OBJ_FLAG_HIDDEN);
  }
}

static void show_idle(void) { fade_in_normal_eyes(); }

static void switch_to_next_emotion(void) {
  // Logic to switch to next emotion
  // Stop custom anim if switching to any other emotion
  if (custom_anim_player) {
    ui_custom_anim_stop(custom_anim_player);
  }

  switch (current_emotion) {
  case EMO_IDLE:
    show_happy();
    current_emotion = EMO_HAPPY;
    break;
  case EMO_HAPPY:
    hide_happy();
    show_sad();
    current_emotion = EMO_SAD;
    break;
  case EMO_SAD:
    hide_sad();
    show_laugh();
    current_emotion = EMO_LAUGH;
    break;
  case EMO_LAUGH:
    hide_laugh();
    show_love();
    current_emotion = EMO_LOVE;
    break;
  case EMO_LOVE:
    hide_love();
    show_sleep();
    current_emotion = EMO_SLEEP;
    break;
  case EMO_SLEEP:
    hide_sleep();
    show_angry();
    current_emotion = EMO_ANGRY;
    break;
  case EMO_ANGRY:
    hide_angry();
    anim_manager_stop(); // Explicitly stop manager when LEAVING angry
    show_idle();         // Restore normal state
    current_emotion = EMO_IDLE;
    break;
  case EMO_CUSTOM:
    // If currently in custom, switch to idle
    if (custom_anim_player) {
      ui_custom_anim_stop(custom_anim_player);
      lv_obj_add_flag(custom_anim_player->canvas, LV_OBJ_FLAG_HIDDEN);
    }
    show_idle();
    current_emotion = EMO_IDLE;
    break;
  }
  timer_ms = 0; // Reset timer just in case
  last_api_emotion =
      (robot_emotion_t)-1; // Reset API guard so external calls can re-trigger
}

static void scr_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_GESTURE) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (dir == LV_DIR_TOP)
      ui_settings_show();
  }
}

static void main_loop(lv_timer_t *timer) {
  update_positions();
  anim_manager_update(); // MUST call this to advance vector animations
  timer_ms += 16;        // 60 FPS update

  // Saccade Update (Random micro-movements)
  // Only create saccades in IDLE or HAPPY to mimic looking around
  if (current_emotion == EMO_IDLE || current_emotion == EMO_HAPPY) {
    if (idle_anim_type == IDLE_ACT_NONE) { // Don't interrupt special acts
      saccade_timer += 50;
      if (saccade_timer >= next_saccade_time) {
        saccade_timer = 0;
        next_saccade_time = 500 + (rand() % 2500);
        int16_t rand_x = (rand() % 31) - 15;
        int16_t rand_y = (rand() % 21) - 10;
        target_gaze_x = (float)rand_x;
        target_gaze_y = (float)rand_y;
      }
    }
  }

  // IDLE TICKS (Squint, Wide, etc.) - Only in IDLE mode
  if (current_emotion == EMO_IDLE) {
    if (idle_anim_type == IDLE_ACT_NONE) {
      idle_anim_timer += 50;
      if (idle_anim_timer >= next_idle_anim_time) {
        idle_anim_timer = 0;
        next_idle_anim_time = 2000 + (rand() % 4000); // More frequent: 2-6s

        // Pick Random Action
        int r = rand() % 100;
        if (r < 12)
          idle_anim_type = IDLE_ACT_SQUINT;
        else if (r < 24)
          idle_anim_type = IDLE_ACT_WIDE;
        else if (r < 36) {
          idle_anim_type = IDLE_ACT_SCAN;
          target_gaze_x = -70.0f; // Look Left
        } else if (r < 44) {
          idle_anim_type = IDLE_ACT_WINK;
        } else if (r < 54) {
          idle_anim_type = IDLE_ACT_THINK;
          target_gaze_x = 50.0f;
          target_gaze_y = -40.0f;
        } else if (r < 62) {
          idle_anim_type = IDLE_ACT_ROLL;
        } else if (r < 72) {
          idle_anim_type = IDLE_ACT_GLANCE;
          target_gaze_x = (rand() % 2 == 0) ? -90.0f : 90.0f;
        } else if (r < 80) {
          idle_anim_type = IDLE_ACT_SHIVER;
        } else if (r < 88) {
          idle_anim_type = IDLE_ACT_BOUNCE;
        } else if (r < 94) {
          idle_anim_type = IDLE_ACT_TILT;
        } else {
          idle_anim_type = IDLE_ACT_CURIOUS;
          target_gaze_x = (rand() % 2 == 0) ? -40.0f : 40.0f;
          target_gaze_y = 20.0f;
        }
        idle_anim_phase = 0.0f;
      }
    } else {
      // Process Active Animation
      float speed = 0.05f;
      if (idle_anim_type == IDLE_ACT_ROLL)
        speed = 0.015f;
      if (idle_anim_type == IDLE_ACT_GLANCE)
        speed = 0.15f;
      if (idle_anim_type == IDLE_ACT_SHIVER)
        speed = 0.20f;
      if (idle_anim_type == IDLE_ACT_BOUNCE)
        speed = 0.10f;

      idle_anim_phase += speed;

      // Special Physics per Action
      if (idle_anim_type == IDLE_ACT_SCAN) {
        if (idle_anim_phase > 0.3f && idle_anim_phase < 0.35f)
          target_gaze_x = 70.0f; // Look Right
        if (idle_anim_phase > 0.7f && idle_anim_phase < 0.75f)
          target_gaze_x = 0.0f; // Look Center
      } else if (idle_anim_type == IDLE_ACT_ROLL) {
        float angle = idle_anim_phase * 6.28f * 2.0f;
        target_gaze_x = cosf(angle) * 50.0f;
        target_gaze_y = sinf(angle) * 40.0f;
      } else if (idle_anim_type == IDLE_ACT_GLANCE) {
        if (idle_anim_phase > 0.4f && idle_anim_phase < 0.6f) {
          target_gaze_x = 0;
        } else if (idle_anim_phase > 0.65f) {
          static float last_x = 0;
          if (idle_anim_phase < 0.7f)
            last_x = target_gaze_x;
          target_gaze_x = (last_x < 0) ? 90.0f : -90.0f;
        }
      } else if (idle_anim_type == IDLE_ACT_CURIOUS) {
        // Tilt left/right gaze while looking down/sideways
        if (idle_anim_phase > 0.5f) {
          target_gaze_x = -target_gaze_x;
        }
      } else if (idle_anim_type == IDLE_ACT_THINK) {
        if (idle_anim_phase > 0.8f) {
          target_gaze_x = 0;
          target_gaze_y = 0;
        }
      }

      if (idle_anim_phase >= 1.0f) {
        idle_anim_type = IDLE_ACT_NONE;
        idle_anim_phase = 0.0f;
      }
    }
  } else {
    idle_anim_type = IDLE_ACT_NONE; // Reset if switched away
  }

  // ANGRY Phase 0 Procedural Update
  if (current_emotion == EMO_ANGRY && angry_phase == 0) {
    int16_t zoom_val = 256;
    int16_t shake_y = 0;
    update_angry_physics(timer_ms, &shake_y, &zoom_val);

    if (left_eye)
      lv_img_set_zoom(left_eye, zoom_val);
    if (right_eye)
      lv_img_set_zoom(right_eye, zoom_val);

    // Show blue components during the initial seething phase
    if (left_angry_blue_eye) {
      lv_obj_clear_flag(left_angry_blue_eye, LV_OBJ_FLAG_HIDDEN);
      lv_img_set_zoom(left_angry_blue_eye, zoom_val);
    }
    if (right_angry_blue_eye) {
      lv_obj_clear_flag(right_angry_blue_eye, LV_OBJ_FLAG_HIDDEN);
      lv_img_set_zoom(right_angry_blue_eye, zoom_val);
    }
    if (angry_blue_mouth) {
      lv_obj_clear_flag(angry_blue_mouth, LV_OBJ_FLAG_HIDDEN);
      lv_img_set_zoom(angry_blue_mouth, zoom_val);
    }
  }

  // Random Blink
  blink_timer += 50;
  if (current_emotion != EMO_LAUGH && current_emotion != EMO_LOVE &&
      current_emotion != EMO_CUSTOM) { // Shake/Love/Custom overrides blink
    if (blink_timer >= next_blink_time) {
      blink_timer = 0;
      do_blink();
      // Next blink: 2s to 6s
      next_blink_time = 2000 + (rand() % 4000);

      // 10% chance of double blink soon
      if ((rand() % 100) < 10) {
        next_blink_time = 300; // Blink again very soon
      }
    }
  }

  // Automatic emotion switching with VARIABLE DURATION
  int duration_ms = 2000; // Default
  switch (current_emotion) {
  case EMO_IDLE:
    duration_ms = 8000;
    break;
  case EMO_HAPPY:
    duration_ms = 1500;
    break;
  case EMO_SAD:
    duration_ms = 2000;
    break;
  case EMO_LAUGH:
    duration_ms = 2000;
    break;
  case EMO_LOVE:
    duration_ms = 2000;
    break;
  case EMO_SLEEP:
    duration_ms = 5000;
    break;
  case EMO_ANGRY:
    // Angry 4-Phase Logic
    // Phase 0: Blue Seething procedural -> 400ms
    // Phase 1: Focus Gap (Still/Wait) -> 300ms
    // Phase 2: Angry Animation (Vector Burst) -> Plays Once
    // Phase 3: Dizzy Animation (Vector Loop) -> 2 Seconds
    if (angry_phase == 0) {
      duration_ms = 400;
    } else if (angry_phase == 1) {
      duration_ms = 300; // Spacing/Gap
    } else if (angry_phase == 2) {
      duration_ms = 10000; // Timeout for vector burst
    } else {               // angry_phase == 3
      duration_ms = 2000;  // Final seething loop
    }
    break;
  case EMO_CUSTOM:
    duration_ms = 9999999;
    break;
  }

  // Dynamic check for Phase 2 (Vector Burst) -> Phase 3 (Dizzy Loop)
  if (current_emotion == EMO_ANGRY && angry_phase == 2) {
    if (!anim_manager_is_playing()) {
      angry_phase = 3;
      printf("[UI] Angry Phase 2 -> 3: Burst done, starting Dizzy loop\n");
      anim_manager_play("dizzy_anim", 0);
      timer_ms = 0;
      return;
    }
  }

  if (timer_ms >= duration_ms) {
    if (current_emotion == EMO_ANGRY) {
      if (angry_phase == 0) {
        // Phase 0 (Blue) -> Phase 1 (Gap)
        angry_phase = 1;
        printf("[UI] Angry Phase 0 -> 1: Hiding blue, focusing...\n");
        if (left_angry_blue_eye)
          lv_obj_add_flag(left_angry_blue_eye, LV_OBJ_FLAG_HIDDEN);
        if (right_angry_blue_eye)
          lv_obj_add_flag(right_angry_blue_eye, LV_OBJ_FLAG_HIDDEN);
        if (angry_blue_mouth)
          lv_obj_add_flag(angry_blue_mouth, LV_OBJ_FLAG_HIDDEN);
        timer_ms = 0;
        return;
      } else if (angry_phase == 1) {
        // Phase 1 (Gap) -> Phase 2 (Burst)
        angry_phase = 2;
        printf("[UI] Angry Phase 1 -> 2: Triggering burst 'angry_anim'\n");
        // Hide normal eyes during vector burst to avoid overlapping
        if (left_eye)
          lv_obj_add_flag(left_eye, LV_OBJ_FLAG_HIDDEN);
        if (right_eye)
          lv_obj_add_flag(right_eye, LV_OBJ_FLAG_HIDDEN);

        bool ok = anim_manager_play("angry_anim", 1);
        if (!ok) {
          printf("[UI] ERR: Failed to play 'angry_anim', skipping burst\n");
          angry_phase = 3;
          anim_manager_play("dizzy_anim", 0);
        }
        timer_ms = 0;
        return;
      }
    }

    switch_to_next_emotion();
  }
}

// Draw Teardrop on Canvas to get perfect shape
static lv_obj_t *create_tear_image(lv_obj_t *parent) {
  if (canvas_tear_ref == NULL) {
    // Dynamically allocate buffer in PSRAM
    size_t buf_size =
        LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA(TEAR_WIDTH, TEAR_HEIGHT);
    void *cbuf = heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);

    canvas_tear_ref = lv_canvas_create(lv_scr_act());
    lv_canvas_set_buffer(canvas_tear_ref, cbuf, TEAR_WIDTH, TEAR_HEIGHT,
                         LV_IMG_CF_TRUE_COLOR_ALPHA);
    lv_canvas_fill_bg(canvas_tear_ref, lv_color_make(0, 0, 0), LV_OPA_TRANSP);

    // Draw Bottom Circle
    lv_draw_rect_dsc_t draw_dsc;
    lv_draw_rect_dsc_init(&draw_dsc);
    draw_dsc.bg_color = EYE_COLOR;
    draw_dsc.bg_opa = LV_OPA_COVER;
    draw_dsc.radius = LV_RADIUS_CIRCLE;
    // Rect(0, 24, 36, 36) -> Center(18, 42), Radius 18
    lv_canvas_draw_rect(canvas_tear_ref, 0, 24, 36, 36, &draw_dsc);

    // Draw Triangle
    // Pts: Top(18, 0), Left(-2, 28), Right(38, 28)
    lv_point_t points[3] = {{18, 0}, {2, 34}, {34, 34}}; // Tuned
    lv_canvas_draw_polygon(canvas_tear_ref, points, 3, &draw_dsc);

    // Highlight
    lv_draw_rect_dsc_t hl_dsc;
    lv_draw_rect_dsc_init(&hl_dsc);
    hl_dsc.bg_color = lv_color_hex(0xFFFFFF);
    hl_dsc.bg_opa = LV_OPA_50;
    hl_dsc.radius = 5;
    lv_canvas_draw_rect(canvas_tear_ref, 20, 36, 8, 14, &hl_dsc);

    lv_obj_add_flag(canvas_tear_ref, LV_OBJ_FLAG_HIDDEN);
  }

  // Create Image from Canvas source
  lv_obj_t *img = lv_img_create(parent);
  lv_img_set_src(img, lv_canvas_get_img(canvas_tear_ref));
  lv_obj_set_style_bg_opa(img, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(img, 0, 0);
  return img;
}

// Draw Left Angry Eye (White Bold Crescent)
static lv_obj_t *create_left_angry_eye_image(lv_obj_t *parent) {
  static lv_obj_t *canvas_angry_ref = NULL;
  if (canvas_angry_ref == NULL) {
    // Canvas size
    int w = 200;
    int h = 200;

    size_t buf_size = LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA(w, h);
    void *cbuf = heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    canvas_angry_ref = lv_canvas_create(lv_scr_act());
    lv_canvas_set_buffer(canvas_angry_ref, cbuf, w, h,
                         LV_IMG_CF_TRUE_COLOR_ALPHA);
    lv_canvas_fill_bg(canvas_angry_ref, lv_color_make(0, 0, 0), LV_OPA_TRANSP);

    // 1. White Base Circle
    lv_draw_rect_dsc_t base_dsc;
    lv_draw_rect_dsc_init(&base_dsc);
    base_dsc.bg_color = lv_color_hex(0xFF0000); // Red
    base_dsc.bg_opa = LV_OPA_COVER;
    base_dsc.radius = LV_RADIUS_CIRCLE;

    // Position: Center-ish
    // Box: x,y,w,h.
    int base_size = 130;
    int base_x = 20;
    int base_y = 50;
    lv_canvas_draw_rect(canvas_angry_ref, base_x, base_y, base_size, base_size,
                        &base_dsc);

    // 2. Black Mask Circle (To cut out the top-inner part)
    // Left Eye: Inner is Right. Top-Right needs to be cut.
    lv_draw_rect_dsc_t mask_dsc;
    lv_draw_rect_dsc_init(&mask_dsc);
    mask_dsc.bg_color = lv_color_hex(0x000000); // Black to hide
    mask_dsc.bg_opa = LV_OPA_COVER;
    mask_dsc.radius = LV_RADIUS_CIRCLE;

    int mask_size = 140; // Slightly larger mask
    // Shift Right (Logic: Cut inner side) and Up (Logic: Angry brow)
    int mask_x = base_x + 40;
    int mask_y = base_y - 45;
    lv_canvas_draw_rect(canvas_angry_ref, mask_x, mask_y, mask_size, mask_size,
                        &mask_dsc);

    lv_obj_add_flag(canvas_angry_ref, LV_OBJ_FLAG_HIDDEN);
  }

  lv_obj_t *img = lv_img_create(parent);
  lv_img_set_src(img, lv_canvas_get_img(canvas_angry_ref));
  lv_obj_set_style_img_recolor_opa(img, LV_OPA_TRANSP, 0);
  lv_img_set_antialias(img, true);
  return img;
}

// Draw Right Angry Eye (White Bold Crescent - Mirrored)
static lv_obj_t *create_right_angry_eye_image(lv_obj_t *parent) {
  static lv_obj_t *canvas_angry_right_ref = NULL;
  if (canvas_angry_right_ref == NULL) {
    int w = 200;
    int h = 200;

    size_t buf_size = LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA(w, h);
    void *cbuf = heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    canvas_angry_right_ref = lv_canvas_create(lv_scr_act());
    lv_canvas_set_buffer(canvas_angry_right_ref, cbuf, w, h,
                         LV_IMG_CF_TRUE_COLOR_ALPHA);
    lv_canvas_fill_bg(canvas_angry_right_ref, lv_color_make(0, 0, 0),
                      LV_OPA_TRANSP);

    // 1. White Base Circle
    lv_draw_rect_dsc_t base_dsc;
    lv_draw_rect_dsc_init(&base_dsc);
    base_dsc.bg_color = lv_color_hex(0xFF0000); // Red
    base_dsc.bg_opa = LV_OPA_COVER;
    base_dsc.radius = LV_RADIUS_CIRCLE;

    int base_size = 130;
    int base_x = 50; // Shifted right slightly compared to left eye box? No,
                     // mirror logic.
    // Left eye was at 20. Total width 200.
    // Let's keep it symmetric in its own box.
    base_x = 50;
    int base_y = 50;
    lv_canvas_draw_rect(canvas_angry_right_ref, base_x, base_y, base_size,
                        base_size, &base_dsc);

    // 2. Black Mask Circle
    // Right Eye: Inner is Left. Top-Left needs to be cut.
    lv_draw_rect_dsc_t mask_dsc;
    lv_draw_rect_dsc_init(&mask_dsc);
    mask_dsc.bg_color = lv_color_hex(0x000000);
    mask_dsc.bg_opa = LV_OPA_COVER;
    mask_dsc.radius = LV_RADIUS_CIRCLE;

    int mask_size = 140;
    // Shift Left (Cut inner) and Up (Angry brow)
    int mask_x = base_x - 50;
    int mask_y = base_y - 45;
    lv_canvas_draw_rect(canvas_angry_right_ref, mask_x, mask_y, mask_size,
                        mask_size, &mask_dsc);

    lv_obj_add_flag(canvas_angry_right_ref, LV_OBJ_FLAG_HIDDEN);
  }

  lv_obj_t *img = lv_img_create(parent);
  lv_img_set_src(img, lv_canvas_get_img(canvas_angry_right_ref));
  lv_obj_set_style_img_recolor_opa(img, LV_OPA_TRANSP, 0);
  lv_img_set_antialias(img, true); // AA Enabled
  return img;
}

// -----------------------------------------------------------------------------
// BLUE ANGRY VARIANTS (Frame 2)
// -----------------------------------------------------------------------------
static lv_obj_t *create_left_angry_blue_eye_image(lv_obj_t *parent) {
  static lv_obj_t *canvas_angry_blue_ref = NULL;
  if (canvas_angry_blue_ref == NULL) {
    int w = 200;
    int h = 200;
    size_t buf_size = LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA(w, h);
    void *cbuf = heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    canvas_angry_blue_ref = lv_canvas_create(lv_scr_act());
    lv_canvas_set_buffer(canvas_angry_blue_ref, cbuf, w, h,
                         LV_IMG_CF_TRUE_COLOR_ALPHA);
    lv_canvas_fill_bg(canvas_angry_blue_ref, lv_color_make(0, 0, 0),
                      LV_OPA_TRANSP);

    // 1. CYAN Base Circle (The U-shape body)
    lv_draw_rect_dsc_t base_dsc;
    lv_draw_rect_dsc_init(&base_dsc);
    base_dsc.bg_color = lv_color_hex(0x00FFFF); // Cyan
    base_dsc.bg_opa = LV_OPA_COVER;
    base_dsc.radius = 40;

    int base_w = 120;
    int base_h = 70;
    int base_x = 40;
    int base_y = 65;
    lv_canvas_draw_rect(canvas_angry_blue_ref, base_x, base_y, base_w, base_h,
                        &base_dsc);

    // 2. Black Mask Rect (To cut the top flat and slanted)
    // Left Eye: Slants DOWN towards center ( \ )
    lv_draw_rect_dsc_t mask_dsc;
    lv_draw_rect_dsc_init(&mask_dsc);
    mask_dsc.bg_color = lv_color_hex(0x000000);
    mask_dsc.bg_opa = LV_OPA_COVER;

    // Use a polygon for the slanted cut
    // Points: Top-Left, Top-Right(lower), Bottom-Right(lower)
    lv_point_t mask_pts[4];
    mask_pts[0].x = 0;
    mask_pts[0].y = 0;
    mask_pts[1].x = 200;
    mask_pts[1].y = 0;
    mask_pts[2].x = 200;
    mask_pts[2].y = 95; // Deep cut inner (Right side of left eye canvas)
    mask_pts[3].x = 0;
    mask_pts[3].y = 65; // Shallow cut outer (Left side)

    lv_canvas_draw_polygon(canvas_angry_blue_ref, mask_pts, 4, &mask_dsc);

    lv_obj_add_flag(canvas_angry_blue_ref, LV_OBJ_FLAG_HIDDEN);
  }

  lv_obj_t *img = lv_img_create(parent);
  lv_img_set_src(img, lv_canvas_get_img(canvas_angry_blue_ref));
  lv_obj_set_style_img_recolor_opa(img, LV_OPA_TRANSP, 0);
  lv_img_set_antialias(img, true);
  return img;
}

static lv_obj_t *create_right_angry_blue_eye_image(lv_obj_t *parent) {
  static lv_obj_t *canvas_angry_blue_right_ref = NULL;
  if (canvas_angry_blue_right_ref == NULL) {
    int w = 200;
    int h = 200;
    size_t buf_size = LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA(w, h);
    void *cbuf = heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    canvas_angry_blue_right_ref = lv_canvas_create(lv_scr_act());
    lv_canvas_set_buffer(canvas_angry_blue_right_ref, cbuf, w, h,
                         LV_IMG_CF_TRUE_COLOR_ALPHA);
    lv_canvas_fill_bg(canvas_angry_blue_right_ref, lv_color_make(0, 0, 0),
                      LV_OPA_TRANSP);

    // 1. CYAN Base Circle (The U-shape body)
    lv_draw_rect_dsc_t base_dsc;
    lv_draw_rect_dsc_init(&base_dsc);
    base_dsc.bg_color = lv_color_hex(0x00FFFF); // Cyan
    base_dsc.bg_opa = LV_OPA_COVER;
    base_dsc.radius = 40;

    int base_w = 120;
    int base_h = 70;
    int base_x = 40;
    int base_y = 65;
    lv_canvas_draw_rect(canvas_angry_blue_right_ref, base_x, base_y, base_w,
                        base_h, &base_dsc);

    // 2. Slanted Mask (Mirrored)
    // Right Eye: Slant DOWN to Left ( / )
    lv_draw_rect_dsc_t mask_dsc;
    lv_draw_rect_dsc_init(&mask_dsc);
    mask_dsc.bg_color = lv_color_hex(0x000000);
    mask_dsc.bg_opa = LV_OPA_COVER;

    // Cut deeper on Left (Inner)
    lv_point_t mask_pts[4];
    mask_pts[0].x = 0;
    mask_pts[0].y = 0;
    mask_pts[1].x = 200;
    mask_pts[1].y = 0;
    mask_pts[2].x = 200;
    mask_pts[2].y = 65; // Shallow cut outer
    mask_pts[3].x = 0;
    mask_pts[3].y = 95; // Deep cut inner

    lv_canvas_draw_polygon(canvas_angry_blue_right_ref, mask_pts, 4, &mask_dsc);

    lv_obj_add_flag(canvas_angry_blue_right_ref, LV_OBJ_FLAG_HIDDEN);
  }

  lv_obj_t *img = lv_img_create(parent);
  lv_img_set_src(img, lv_canvas_get_img(canvas_angry_blue_right_ref));
  lv_obj_set_style_img_recolor_opa(img, LV_OPA_TRANSP, 0);
  lv_img_set_antialias(img, true); // AA Enabled
  return img;
}

lv_obj_t *ui_robo_eyes_get_scr(void) { return scr_eyes; }

// Touch event handler for the screen
static void screen_touch_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    if (current_emotion != EMO_CUSTOM) {
      printf("[UI] Screen clicked! Force switching to Custom Animation.\n");
      ui_robo_eyes_set_emotion_type(EMOTION_CUSTOM);
    } else {
      printf("[UI] Screen clicked! Returning to Normal state.\n");
      ui_robo_eyes_set_emotion_type(EMOTION_NORMAL);
    }
  }
}

void ui_robo_eyes_init(void) {
  if (scr_eyes)
    return;

  scr_eyes = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr_eyes, BG_COLOR, 0);
  lv_obj_set_style_bg_opa(scr_eyes, LV_OPA_COVER, 0);
  lv_obj_clear_flag(scr_eyes, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(scr_eyes, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(scr_eyes, screen_touch_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(scr_eyes, scr_event_cb, LV_EVENT_GESTURE, NULL);

  lv_style_init(&style_eye);
  lv_style_set_radius(&style_eye, LV_RADIUS_CIRCLE);
  lv_style_set_bg_color(&style_eye, EYE_COLOR);
  lv_style_set_bg_opa(&style_eye, LV_OPA_COVER);
  lv_style_set_shadow_width(&style_eye, 20);
  lv_style_set_shadow_color(&style_eye, lv_color_hex(0x00AAAA));
  lv_style_set_shadow_opa(&style_eye, LV_OPA_50);
  lv_style_set_border_width(&style_eye, 0);

  lv_style_init(&style_mask);
  lv_style_set_radius(&style_mask, LV_RADIUS_CIRCLE);
  lv_style_set_bg_color(&style_mask, BG_COLOR);
  lv_style_set_bg_opa(&style_mask, LV_OPA_COVER);
  lv_style_set_border_width(&style_mask, 0);

  lv_style_init(&style_line);
  lv_style_set_line_width(&style_line, 14);
  lv_style_set_line_color(&style_line, EYE_COLOR);
  lv_style_set_line_rounded(&style_line, true);

  lv_style_init(&style_tear_part); // For shadow
  lv_style_set_shadow_width(&style_tear_part, 15);
  lv_style_set_shadow_color(&style_tear_part, EYE_COLOR);
  lv_style_set_shadow_opa(&style_tear_part, LV_OPA_80);

  left_eye = lv_obj_create(scr_eyes);
  lv_obj_add_style(left_eye, &style_eye, 0);
  lv_obj_set_size(left_eye, EYE_WIDTH, EYE_HEIGHT);
  lv_obj_align(left_eye, LV_ALIGN_CENTER, -EYE_OFFSET_X, 0);
  lv_obj_clear_flag(left_eye, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(left_eye, LV_OBJ_FLAG_CLICKABLE);

  right_eye = lv_obj_create(scr_eyes);
  lv_obj_add_style(right_eye, &style_eye, 0);
  lv_obj_set_size(right_eye, EYE_WIDTH, EYE_HEIGHT);
  lv_obj_align(right_eye, LV_ALIGN_CENTER, EYE_OFFSET_X, 0);
  lv_obj_clear_flag(right_eye, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(right_eye, LV_OBJ_FLAG_CLICKABLE);

  left_laugh_eye = lv_line_create(scr_eyes);
  lv_line_set_points(left_laugh_eye, left_arrow_pts, LAUGH_EYE_POINTS);
  lv_obj_add_style(left_laugh_eye, &style_line, 0);
  lv_obj_align(left_laugh_eye, LV_ALIGN_CENTER, -EYE_OFFSET_X, 0);
  lv_obj_add_flag(left_laugh_eye, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(left_laugh_eye, LV_OBJ_FLAG_CLICKABLE);

  right_laugh_eye = lv_line_create(scr_eyes);
  lv_line_set_points(right_laugh_eye, right_arrow_pts, LAUGH_EYE_POINTS);
  lv_obj_add_style(right_laugh_eye, &style_line, 0);
  lv_obj_align(right_laugh_eye, LV_ALIGN_CENTER, EYE_OFFSET_X, 0);
  lv_obj_add_flag(right_laugh_eye, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(right_laugh_eye, LV_OBJ_FLAG_CLICKABLE);

  // Big Laugh Mouth (Pink & Filled) - Frame 1
  big_laugh_mouth = lv_obj_create(scr_eyes);
  lv_obj_set_size(big_laugh_mouth, BIG_MOUTH_WIDTH, BIG_MOUTH_HEIGHT);
  lv_obj_set_style_radius(big_laugh_mouth, BIG_MOUTH_RADIUS, 0);
  lv_obj_set_style_bg_color(big_laugh_mouth, BIG_MOUTH_COLOR, 0);
  lv_obj_set_style_bg_opa(big_laugh_mouth, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(big_laugh_mouth, EYE_COLOR, 0); // Cyan Lips
  lv_obj_set_style_border_width(big_laugh_mouth, 10, 0); // Thicker border
  lv_obj_set_style_border_side(big_laugh_mouth, LV_BORDER_SIDE_FULL, 0);
  lv_obj_set_style_clip_corner(big_laugh_mouth, true, 0); // Fix aliasing
  lv_obj_align(big_laugh_mouth, LV_ALIGN_CENTER, 0, BIG_MOUTH_OFFSET_Y);
  lv_obj_add_flag(big_laugh_mouth, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(big_laugh_mouth, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(big_laugh_mouth, LV_OBJ_FLAG_CLICKABLE);

  // Create Tongue (Child of Big Mouth)
  // It will move and scale automatically with the mouth!
  laugh_tongue = lv_obj_create(big_laugh_mouth);
  lv_obj_set_size(laugh_tongue, 100, 60);       // Width 100, Height 60
  lv_obj_set_style_radius(laugh_tongue, 30, 0); // Rounded
  lv_obj_set_style_bg_color(laugh_tongue, lv_color_hex(0xE8505B),
                            0); // Tongue Color (Red-ish)
  lv_obj_set_style_bg_opa(laugh_tongue, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(laugh_tongue, 0, 0);
  // Align at bottom-center of mouth
  lv_obj_align(laugh_tongue, LV_ALIGN_BOTTOM_MID, 0,
               10); // Offset 10px down to hide bottom curve slightly

  // Grin Laugh Mouth (Teeth) - Frame 2
  // Create static canvas for grin if needed, or just draw lines on a bg
  // Let's use a canvas for the detailed grid look
  if (canvas_grin_ref == NULL) {
    size_t buf_size = LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA(BIG_MOUTH_WIDTH, 80);
    void *cbuf = heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    canvas_grin_ref = lv_canvas_create(lv_scr_act());
    lv_canvas_set_buffer(canvas_grin_ref, cbuf, BIG_MOUTH_WIDTH, 80,
                         LV_IMG_CF_TRUE_COLOR_ALPHA);
    lv_canvas_fill_bg(canvas_grin_ref, lv_color_make(0, 0, 0), LV_OPA_TRANSP);

    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = lv_color_make(0, 0, 0); // Black inside
    rect_dsc.bg_opa = LV_OPA_COVER;
    rect_dsc.border_color = EYE_COLOR; // Cyan border
    rect_dsc.border_width = 8;
    rect_dsc.radius = 40; // More rounded corners
    lv_canvas_draw_rect(canvas_grin_ref, 0, 0, BIG_MOUTH_WIDTH, 80, &rect_dsc);

    // Draw teeth lines
    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = EYE_COLOR;
    line_dsc.width = 6;
    line_dsc.round_start = 1;
    line_dsc.round_end = 1;

    // Horizontal mid line
    lv_point_t p_mid[] = {{10, 40}, {BIG_MOUTH_WIDTH - 10, 40}};
    lv_canvas_draw_line(canvas_grin_ref, p_mid, 2, &line_dsc);

    // Vertical lines
    for (int i = 1; i < 5; i++) {
      int x = (BIG_MOUTH_WIDTH / 5) * i;
      lv_point_t p_vert[] = {{x, 10}, {x, 70}};
      lv_canvas_draw_line(canvas_grin_ref, p_vert, 2, &line_dsc);
    }
    lv_obj_add_flag(canvas_grin_ref, LV_OBJ_FLAG_HIDDEN);
  }

  grin_laugh_mouth = lv_img_create(scr_eyes);
  lv_img_set_src(grin_laugh_mouth, lv_canvas_get_img(canvas_grin_ref));
  lv_obj_set_style_clip_corner(grin_laugh_mouth, true, 0); // Fix aliasing
  lv_obj_align(grin_laugh_mouth, LV_ALIGN_CENTER, 0, BIG_MOUTH_OFFSET_Y);
  lv_obj_add_flag(grin_laugh_mouth, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(grin_laugh_mouth, LV_OBJ_FLAG_CLICKABLE);

  // Heart Eyes (LOVE emotion) - Canvas drawn heart shape
  // Create heart canvas (cached, like teardrop)
  if (canvas_heart_ref == NULL) {
    size_t buf_size = LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA(HEART_CANVAS_SIZE,
                                                          HEART_CANVAS_SIZE);
    void *heart_cbuf = heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);

    canvas_heart_ref = lv_canvas_create(lv_scr_act());
    lv_canvas_set_buffer(canvas_heart_ref, heart_cbuf, HEART_CANVAS_SIZE,
                         HEART_CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR_ALPHA);
    lv_canvas_fill_bg(canvas_heart_ref, lv_color_make(0, 0, 0), LV_OPA_TRANSP);

    // Heart main color - Beautiful Red/Pink
    lv_draw_rect_dsc_t heart_dsc;
    lv_draw_rect_dsc_init(&heart_dsc);
    heart_dsc.bg_color = lv_color_hex(0xE8505B); // Nice red-pink
    heart_dsc.bg_opa = LV_OPA_COVER;
    heart_dsc.radius = LV_RADIUS_CIRCLE;

    // STEP 1: Draw bottom triangle FIRST (it will be behind the circles)
    // Make it start from higher up to overlap with circles
    lv_point_t heart_pts[3] = {
        {60, 118}, // Bottom point
        {-5, 35},  // Left corner (outside canvas for full coverage)
        {125, 35}  // Right corner (outside canvas for full coverage)
    };
    lv_canvas_draw_polygon(canvas_heart_ref, heart_pts, 3, &heart_dsc);

    // STEP 2: Draw two overlapping circles at top (they go on top of triangle)
    int circle_size = 60;
    // Left bump - positioned to overlap
    lv_canvas_draw_rect(canvas_heart_ref, 3, 8, circle_size, circle_size,
                        &heart_dsc);
    // Right bump - positioned to overlap
    lv_canvas_draw_rect(canvas_heart_ref, 57, 8, circle_size, circle_size,
                        &heart_dsc);

    // STEP 3: Add subtle highlight for 3D effect (small circle at top-left)
    lv_draw_rect_dsc_t highlight_dsc;
    lv_draw_rect_dsc_init(&highlight_dsc);
    highlight_dsc.bg_color = lv_color_hex(0xFFCCCC); // Very light pink
    highlight_dsc.bg_opa = LV_OPA_50;
    highlight_dsc.radius = LV_RADIUS_CIRCLE;
    lv_canvas_draw_rect(canvas_heart_ref, 16, 18, 20, 20, &highlight_dsc);

    lv_obj_add_flag(canvas_heart_ref, LV_OBJ_FLAG_HIDDEN);
  }

  // Create left heart eye from canvas
  left_heart_eye = lv_img_create(scr_eyes);
  lv_img_set_src(left_heart_eye, lv_canvas_get_img(canvas_heart_ref));
  lv_obj_align(left_heart_eye, LV_ALIGN_CENTER, -HEART_EYE_OFFSET_X, 0);
  lv_obj_add_flag(left_heart_eye, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(left_heart_eye, LV_OBJ_FLAG_CLICKABLE);

  // Create right heart eye from canvas
  right_heart_eye = lv_img_create(scr_eyes);
  lv_img_set_src(right_heart_eye, lv_canvas_get_img(canvas_heart_ref));
  lv_obj_align(right_heart_eye, LV_ALIGN_CENTER, HEART_EYE_OFFSET_X, 0);
  lv_obj_add_flag(right_heart_eye, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(right_heart_eye, LV_OBJ_FLAG_CLICKABLE);

  // Removed omega mouth

  // Create cheeks FIRST (so mouth is on top)
  left_cheek = lv_obj_create(scr_eyes);
  lv_obj_add_style(left_cheek, &style_mask, 0);
  lv_obj_set_size(left_cheek, CHEEK_SIZE, CHEEK_SIZE);
  lv_obj_align(left_cheek, LV_ALIGN_CENTER, -EYE_OFFSET_X,
               CHEEK_OFFSET_Y_HIDDEN);
  lv_obj_clear_flag(left_cheek, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(left_cheek, LV_OBJ_FLAG_CLICKABLE);

  right_cheek = lv_obj_create(scr_eyes);
  lv_obj_add_style(right_cheek, &style_mask, 0);
  lv_obj_set_size(right_cheek, CHEEK_SIZE, CHEEK_SIZE);
  lv_obj_align(right_cheek, LV_ALIGN_CENTER, EYE_OFFSET_X,
               CHEEK_OFFSET_Y_HIDDEN);
  lv_obj_clear_flag(right_cheek, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(right_cheek, LV_OBJ_FLAG_CLICKABLE);

  // Create Happy Mouth "~" AFTER cheeks
  happy_mouth = lv_line_create(scr_eyes);
  // Points set in update_positions
  lv_style_init(&style_happy_line);
  lv_style_set_line_width(&style_happy_line, 12);
  lv_style_set_line_color(&style_happy_line, EYE_COLOR);
  lv_style_set_line_rounded(&style_happy_line, true);
  lv_obj_add_style(happy_mouth, &style_happy_line, 0);
  lv_obj_add_flag(happy_mouth, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(happy_mouth, LV_OBJ_FLAG_CLICKABLE);

  // Big Open Happy Mouth (New: Exact D-Shape Container)
  happy_open_mouth = lv_obj_create(scr_eyes);
  lv_obj_set_size(happy_open_mouth, 150, 75); // Semi-circle height
  lv_obj_set_style_bg_opa(happy_open_mouth, 0, 0);
  lv_obj_set_style_border_width(happy_open_mouth, 0, 0);
  lv_obj_set_style_radius(happy_open_mouth, 10, 0); // Softer top corners
  lv_obj_set_style_clip_corner(happy_open_mouth, true, 0);
  lv_obj_clear_flag(happy_open_mouth, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(happy_open_mouth, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t *shape = lv_obj_create(happy_open_mouth);
  lv_obj_set_size(shape, 150, 150); // Full circle
  lv_obj_set_style_bg_color(shape, EYE_COLOR, 0);
  lv_obj_set_style_bg_opa(shape, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(shape, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(shape, 0, 0);
  lv_obj_align(shape, LV_ALIGN_TOP_MID, 0, -75); // Move center to container top
  lv_obj_clear_flag(shape, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(shape, LV_OBJ_FLAG_CLICKABLE);

  left_sad_mask = lv_obj_create(scr_eyes);
  lv_obj_add_style(left_sad_mask, &style_mask, 0);
  lv_obj_set_size(left_sad_mask, SAD_MASK_SIZE, SAD_MASK_SIZE);
  lv_obj_align(left_sad_mask, LV_ALIGN_CENTER,
               -EYE_OFFSET_X - SAD_MASK_OFFSET_X, SAD_MASK_OFFSET_Y_HIDDEN);
  lv_obj_clear_flag(left_sad_mask, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(left_sad_mask, LV_OBJ_FLAG_CLICKABLE);

  right_sad_mask = lv_obj_create(scr_eyes);
  lv_obj_add_style(right_sad_mask, &style_mask, 0);
  lv_obj_set_size(right_sad_mask, SAD_MASK_SIZE, SAD_MASK_SIZE);
  lv_obj_align(right_sad_mask, LV_ALIGN_CENTER,
               EYE_OFFSET_X + SAD_MASK_OFFSET_X, SAD_MASK_OFFSET_Y_HIDDEN);
  lv_obj_clear_flag(right_sad_mask, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(right_sad_mask, LV_OBJ_FLAG_CLICKABLE);

  left_tear = create_tear_image(scr_eyes);
  // lv_obj_add_style(left_tear, &style_tear_part, 0); // Removed to fix square
  // shadow box
  lv_obj_align(left_tear, LV_ALIGN_CENTER, -EYE_OFFSET_X, TEAR_START_Y);
  lv_obj_clear_flag(left_tear, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(left_tear, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(left_tear, LV_OBJ_FLAG_CLICKABLE);

  right_tear = create_tear_image(scr_eyes);
  // lv_obj_add_style(right_tear, &style_tear_part, 0); // Removed to fix square
  // shadow box
  lv_obj_align(right_tear, LV_ALIGN_CENTER, EYE_OFFSET_X, TEAR_START_Y);
  lv_obj_clear_flag(right_tear, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(right_tear, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(right_tear, LV_OBJ_FLAG_CLICKABLE);

  // --- Sleep Elements ---
  left_sleep_eye = lv_line_create(scr_eyes);
  lv_obj_add_style(left_sleep_eye, &style_line, 0);
  lv_obj_add_flag(left_sleep_eye, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(left_sleep_eye, LV_OBJ_FLAG_CLICKABLE);

  right_sleep_eye = lv_line_create(scr_eyes);
  lv_obj_add_style(right_sleep_eye, &style_line, 0);
  lv_obj_add_flag(right_sleep_eye, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(right_sleep_eye, LV_OBJ_FLAG_CLICKABLE);

  // Sleep Mouth (Oval yawn shape)
  sleep_mouth = lv_obj_create(scr_eyes);
  lv_obj_set_size(sleep_mouth, 10, 5); // Start small
  lv_obj_set_style_radius(sleep_mouth, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(sleep_mouth, EYE_COLOR, 0);
  lv_obj_set_style_bg_opa(sleep_mouth, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(sleep_mouth, 0, 0);
  lv_obj_align(sleep_mouth, LV_ALIGN_CENTER, 0, 80);
  lv_obj_add_flag(sleep_mouth, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(sleep_mouth, LV_OBJ_FLAG_SCROLLABLE);

  // Style Z (Restored for Labels)
  lv_style_init(&style_z);
  lv_style_set_text_color(&style_z, EYE_COLOR);
  lv_style_set_text_font(&style_z, &lv_font_montserrat_48); // BIG FONT

  // ZZZ Labs (Reverted to Labels for visibility guarantee)
  sleep_z1 = lv_label_create(scr_eyes);
  lv_label_set_text(sleep_z1, "Z");
  lv_obj_add_style(sleep_z1, &style_z, 0);
  lv_obj_align(sleep_z1, LV_ALIGN_CENTER, 0, 0); // Start at center
  lv_obj_add_flag(sleep_z1, LV_OBJ_FLAG_HIDDEN);

  sleep_z2 = lv_label_create(scr_eyes);
  lv_label_set_text(sleep_z2, "z");
  lv_obj_add_style(sleep_z2, &style_z, 0);
  lv_obj_align(sleep_z2, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_flag(sleep_z2, LV_OBJ_FLAG_HIDDEN);

  sleep_z3 = lv_label_create(scr_eyes);
  lv_label_set_text(sleep_z3, "z");
  lv_obj_add_style(sleep_z3, &style_z, 0);
  lv_obj_align(sleep_z3, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_flag(sleep_z3, LV_OBJ_FLAG_HIDDEN);

  // --- Angry Elements ---
  left_angry_eye = create_left_angry_eye_image(scr_eyes);
  lv_obj_align(left_angry_eye, LV_ALIGN_CENTER, -EYE_OFFSET_X, 0);
  lv_obj_add_flag(left_angry_eye, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(left_angry_eye, LV_OBJ_FLAG_CLICKABLE);

  right_angry_eye = create_right_angry_eye_image(scr_eyes);
  lv_obj_align(right_angry_eye, LV_ALIGN_CENTER, EYE_OFFSET_X, 0);
  lv_obj_add_flag(right_angry_eye, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(right_angry_eye, LV_OBJ_FLAG_CLICKABLE);

  // --- Blue Angry Elements (Frame 2) ---
  // Re-use helper logic with Color Param?
  // For simplicity and speed, we will call duplicated specific functions for
  // Blue variants.
  left_angry_blue_eye = create_left_angry_blue_eye_image(scr_eyes);
  lv_obj_align(left_angry_blue_eye, LV_ALIGN_CENTER, -EYE_OFFSET_X, 0);
  lv_obj_add_flag(left_angry_blue_eye, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(left_angry_blue_eye, LV_OBJ_FLAG_CLICKABLE);

  right_angry_blue_eye = create_right_angry_blue_eye_image(scr_eyes);
  lv_obj_align(right_angry_blue_eye, LV_ALIGN_CENTER, EYE_OFFSET_X, 0);
  lv_obj_add_flag(right_angry_blue_eye, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(right_angry_blue_eye, LV_OBJ_FLAG_CLICKABLE);

  // Blue Mouth (Peanut/Rounded Rect)
  angry_blue_mouth = lv_obj_create(scr_eyes);
  lv_obj_set_size(angry_blue_mouth, 80, 40);
  lv_obj_set_style_radius(angry_blue_mouth, 20, 0); // Fully rounded
  lv_obj_set_style_bg_color(angry_blue_mouth, lv_color_hex(0x00FFFF),
                            0); // Cyan
  lv_obj_set_style_bg_opa(angry_blue_mouth, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(angry_blue_mouth, 0, 0);
  lv_obj_align(angry_blue_mouth, LV_ALIGN_CENTER, 0, 80);
  lv_obj_add_flag(angry_blue_mouth, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(angry_blue_mouth,
                    LV_OBJ_FLAG_CLICKABLE); // Fix blocking tap

  // Lids removed - baked into eye images

  // Add WiFi indicator to robot eyes screen
  ui_settings_create_wifi_indicator_on(scr_eyes);

  lv_scr_load(scr_eyes);

  // Initialize Animation Manager
  anim_manager_init(scr_eyes);
  register_all_animations();

  // Initialize Custom Shape Animation Player
  custom_anim_player = ui_custom_anim_create(scr_eyes);
  ui_custom_anim_align(custom_anim_player, LV_ALIGN_CENTER, 0, 0);

  // References for automatic frame loading
  /* Deprecated externs
  extern const uint8_t my_anim_frame_count;
  extern const shape_keyframe_t *my_anim_all_shapes[];
  extern const uint8_t my_anim_all_counts[];
  */

  // No more hardcoding! Directly pass the master arrays.
  /* Manual animation init - Deprecated by Animation Manager
  // Cast to (const shape_keyframe_t **) to match function signature
  ui_custom_anim_set_shape_src(custom_anim_player,
                               (const shape_keyframe_t **)my_anim_all_shapes,
                               my_anim_all_counts, my_anim_frame_count,
                               my_anim_total_duration, lv_anim_path_overshoot);

  // It starts hidden/stopped
  ui_custom_anim_stop(custom_anim_player);
  */

  srand(12345);

  main_timer = lv_timer_create(main_loop, 16, NULL); // 16ms = ~60 FPS

  // Add touch event to the whole screen to switch modes
  // No-op - removed double registration
}

// --- API ---
void ui_robo_eyes_set_emotion_type(robot_emotion_t emotion) {
  // Guard: Protect against redundant calls which cause animation flickering
  if (emotion == last_api_emotion)
    return;
  last_api_emotion = emotion;

  // Reset Angry Phase
  if (emotion == EMOTION_ANGRY) {
    angry_phase = 0;
  }

  // 6. Ensure other emotions stop the custom animation
  if (emotion != EMOTION_CUSTOM) {
    anim_manager_stop();
    // Also stop legacy if it exists
    if (custom_anim_player) {
      ui_custom_anim_stop(custom_anim_player);
      lv_obj_add_flag(custom_anim_player->canvas, LV_OBJ_FLAG_HIDDEN);
    }
  }

  // 5. Handle EMOTION_CUSTOM
  if (emotion == EMOTION_CUSTOM) {
    fade_out_normal_eyes();
    hide_happy();
    hide_sad();
    hide_laugh();
    hide_love();
    hide_sleep();
    hide_angry();

    // Start Custom Anim via Manager
    anim_manager_play("wink", 0); // Use "wink" as the demo custom animation

    current_emotion = EMO_CUSTOM;
    timer_ms = 0; // Reset timer
    return;
  }

  // Hide current
  switch (current_emotion) {
  case EMO_IDLE:
    fade_out_normal_eyes();
    break;
  case EMO_HAPPY:
    hide_happy();
    break;
  case EMO_SAD:
    hide_sad();
    break;
  case EMO_LAUGH:
    hide_laugh();
    break;
  case EMO_LOVE:
    hide_love();
    break;
  case EMO_SLEEP:
    hide_sleep();
    break;
  case EMO_ANGRY:
    hide_angry();
    break;
  case EMO_CUSTOM:
    if (custom_anim_player) {
      ui_custom_anim_stop(custom_anim_player);
      lv_obj_add_flag(custom_anim_player->canvas, LV_OBJ_FLAG_HIDDEN);
    }
    break;
  }

  // Show new (Map robot_emotion_t to internal emotion_t)
  switch (emotion) {
  case EMOTION_NORMAL:
    show_idle();
    current_emotion = EMO_IDLE;
    // Reset eye shape from Happy mode
    if (left_eye)
      lv_obj_set_style_radius(left_eye, EYE_RADIUS, 0);
    if (right_eye)
      lv_obj_set_style_radius(right_eye, EYE_RADIUS, 0);
    break;
  case EMOTION_HAPPY:
    show_happy();
    current_emotion = EMO_HAPPY;
    break;
  case EMOTION_SAD:
    show_sad();
    current_emotion = EMO_SAD;
    break;
  case EMOTION_LAUGH:
    show_laugh();
    current_emotion = EMO_LAUGH;
    break;
  case EMOTION_LOVE:
    show_love();
    current_emotion = EMO_LOVE;
    break;
  case EMOTION_SLEEP:
    show_sleep();
    current_emotion = EMO_SLEEP;
    break;
  case EMOTION_ANGRY:
    show_angry();
    current_emotion = EMO_ANGRY;
    angry_phase = 0; // Reset to Phase 0 (procedural)
    break;
  // Map others to normal for now if not implemented
  default:
    show_idle();
    current_emotion = EMO_IDLE;
    break;
  }
  timer_ms = 0; // Reset timer
}
void ui_robo_eyes_set_emotion(const char *emotion) {}
void ui_robo_eyes_look_at(int16_t x, int16_t y) {
  float prev_x = target_gaze_x;
  float prev_y = target_gaze_y;

  target_gaze_x = (float)x;
  target_gaze_y = (float)y;

  // Anticipatory Blink: If the gaze shift is large, trigger a blink!
  // This is what humans and animals do to "refresh" during a look-around.
  float dist =
      sqrtf(powf(target_gaze_x - prev_x, 2) + powf(target_gaze_y - prev_y, 2));
  if (dist > 35.0f && current_emotion != EMO_LAUGH) {
    do_blink();
  }

  // If difference is HUGE (initial boot), snap instantly
  if (fabsf(target_gaze_x - current_gaze_x) > 200.0f) {
    current_gaze_x = target_gaze_x;
    current_gaze_y = target_gaze_y;
    gaze_vel_x = 0;
    gaze_vel_y = 0;
  }
}
void ui_robo_eyes_blink(void) { do_blink(); }
robot_emotion_t ui_robo_eyes_get_emotion(void) {
  switch (current_emotion) {
  case EMO_IDLE:
    return EMOTION_NORMAL;
  case EMO_HAPPY:
    return EMOTION_HAPPY;
  case EMO_SAD:
    return EMOTION_SAD;
  case EMO_LAUGH:
    return EMOTION_LAUGH;
  case EMO_LOVE:
    return EMOTION_LOVE;
  case EMO_SLEEP:
    return EMOTION_SLEEP;
  case EMO_ANGRY:
    return EMOTION_ANGRY;
  case EMO_CUSTOM:
    return EMOTION_CUSTOM;
  default:
    return EMOTION_NORMAL;
  }
}
