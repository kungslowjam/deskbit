/**
 * @file ui_robo_eyes.c
 * @brief Robot Eyes - Canvas Based Perfect Teardrop (Fat Shape)
 */

#include "ui_robo_eyes.h"
#include "lvgl.h"
#include "ui_settings.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

LV_FONT_DECLARE(lv_font_montserratMedium_20);
LV_FONT_DECLARE(lv_font_montserratMedium_23);

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
// Enhanced Eye Animation - FAST & DYNAMIC
#define EYE_PULSE_SCALE_MIN 0.85f
#define EYE_PULSE_SCALE_MAX 1.15f
#define EYE_SQUINT_HEIGHT_MIN 22
#define EYE_SQUINT_HEIGHT_MAX 38 // Bigger range = more visible
#define EYE_PULSE_SPEED 8.0f     // MUCH faster! Very energetic

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

// Laugh Objects
static lv_obj_t *left_laugh_eye = NULL;
static lv_obj_t *right_laugh_eye = NULL;
static lv_obj_t *omega_mouth = NULL; // "ω" shape mouth line

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
#define HEART_COLOR lv_color_hex(0xFF6B8A) // Pink color for glow

// Canvas Teardrop Single Instance Cache
// Canvas Teardrop Single Instance Cache
static lv_obj_t *canvas_tear_ref = NULL;
static lv_obj_t *canvas_grin_ref = NULL;

static lv_timer_t *main_timer = NULL;

// State
typedef enum {
  EMO_IDLE,
  EMO_HAPPY,
  EMO_SAD,
  EMO_LAUGH,
  EMO_LOVE,
  EMO_SLEEP
} emotion_t;
static emotion_t current_emotion = EMO_IDLE;
static int timer_ms = 0;
static int breath_phase = 0;
// Removed static int16_t gaze_x/y as we use floats now
// static int16_t gaze_x = 0;
// static int16_t gaze_y = 0;
static int laugh_shake_phase = 0;
static int laugh_cycle = 0;
static int laugh_intensity = 0; // 0-100, builds up during laugh

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
#define GAZE_SPRING_K 0.18f // Stiffness
#define GAZE_DAMPING 0.65f  // Friction (lower = more bouncy)

// Saccade (Random micro-movements)
static int saccade_timer = 0;
static int next_saccade_time = 50; // Randomize start

// Blink Randomness
static int blink_timer = 0;
static int next_blink_time = 3000;

// Love animation state
static float heart_beat_phase = 0.0f;
static int love_intensity = 0;

// Omega mouth "ω" points (5 points for the curve) - DYNAMIC!
// Shape:  \  /\  /
//          \/  \/
static lv_point_t omega_mouth_pts[] = {
    {0, 0},                                          // Left start
    {OMEGA_MOUTH_WIDTH / 4, OMEGA_MOUTH_HEIGHT},     // Left valley
    {OMEGA_MOUTH_WIDTH / 2, 0},                      // Center peak
    {OMEGA_MOUTH_WIDTH * 3 / 4, OMEGA_MOUTH_HEIGHT}, // Right valley
    {OMEGA_MOUTH_WIDTH, 0}                           // Right end
};
static lv_style_t style_omega_mouth;

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

// -----------------------------------------------------------------------------
// Position Update
// -----------------------------------------------------------------------------
static void update_positions(void) {
  breath_phase += 2;
  if (breath_phase >= 360)
    breath_phase = 0;

  // Organic Breathing: Main slow wave + subtle secondary wave for "life"
  float rad1 = breath_phase * 0.01745f;
  float rad2 = breath_phase * 2.1f * 0.01745f; // Faster micro-oscillation
  int16_t breath = (int16_t)(3.2f * sinf(rad1) + 0.6f * sinf(rad2));

  // Laugh animation - organic bouncy movement
  int16_t shake_x = 0;
  int16_t shake_y = 0;
  int16_t squint_offset = 0;

  if (current_emotion == EMO_LAUGH) {
    // Build up laugh intensity
    if (laugh_intensity < 100)
      laugh_intensity += 6;
    float intensity_scale = laugh_intensity / 100.0f;

    // Phases
    laugh_shake_phase += 25;
    if (laugh_shake_phase >= 360)
      laugh_shake_phase = 0;
    float phase_rad = laugh_shake_phase * 0.01745f;

    eye_pulse_phase += EYE_PULSE_SPEED;
    if (eye_pulse_phase >= 360.0f)
      eye_pulse_phase = 0.0f;
    float pulse_rad = eye_pulse_phase * 0.01745f;

    // --- 1. Update EYES (Arc 'n' Shape) - DYNAMIC MORPHING ---
    // Use laugh_wave (calculated below, but we can compute it here or use
    // previous frame's? Let's compute 'wave_for_eyes' here to be safe and
    // consistent.

    // Rhythm matches the head bounce
    float wave_speed = 1.5f;
    float wave_for_eyes = sinf(phase_rad * wave_speed);

    // Morph Factor: -1.0 (Squint/Grin) to 1.0 (Open/High Arc)
    // We map this to parabola parameters.

    // Height of vertex
    // Open: High (25px), Grin: Lower/Sharper (15px)
    float base_h = 20.0f;
    float dynamic_h = base_h + (5.0f * wave_for_eyes * intensity_scale);

    // Curvature (a coeff)
    // Sharper when Grin (Negative wave) -> Higher 'a' value (narrower parabola)
    // Flatter/Rounder when Open (Positive wave) -> Lower 'a' value
    float sharpness =
        1.0f - (0.3f * wave_for_eyes); // 1.3 (Sharp) to 0.7 (Round)

    // Parabola: y = a(x-h)^2
    // We want ends at y = dynamic_h? No, ends usually fixed or relative.
    // Let's fix ends at y=dynamic_h, Peak at y=0.
    // a = dynamic_h / (50^2) * sharpness

    float a_coeff = (dynamic_h / 2500.0f) * sharpness;

    for (int i = 0; i < LAUGH_EYE_POINTS; i++) {
      float t = (float)i / (LAUGH_EYE_POINTS - 1); // 0.0 to 1.0
      int16_t x = (int16_t)(t * 100.0f);

      // y = a * (x - 50)^2
      float dx = x - 50.0f;
      int16_t y = (int16_t)(a_coeff * dx * dx);

      // Add 'Sharpness' artifact? (V-shape vs U-shape)
      // If we want V-shape, we linerize near vertex.
      // But parabola is fine, just changing aspect ratio (a_coeff) does a lot.

      left_arrow_pts[i].x = x;
      left_arrow_pts[i].y = y;

      right_arrow_pts[i].x = x;
      right_arrow_pts[i].y = y;
    }

    if (left_laugh_eye)
      lv_line_set_points(left_laugh_eye, left_arrow_pts, LAUGH_EYE_POINTS);
    if (right_laugh_eye)
      lv_line_set_points(right_laugh_eye, right_arrow_pts, LAUGH_EYE_POINTS);

    // Line width - THICKER for better look
    int16_t line_thickness =
        16 + (int16_t)(4.0f * intensity_scale *
                       sinf(pulse_rad * 2)); // Thicker base (16)
    if (left_laugh_eye)
      lv_obj_set_style_line_width(left_laugh_eye, line_thickness, 0);
    if (right_laugh_eye)
      lv_obj_set_style_line_width(right_laugh_eye, line_thickness, 0);

    // --- 2. BIG MOUTH - COMPLETELY STATIC (No animation to avoid flicker) ---
    // The mouth just stays visible, eyes do all the animation
    // --- 2. NEW LAUGH ANIMATION: "Head Back" Laugh ---
    // --- 2. NEW LAUGH ANIMATION: "Jelly Mouth" Laugh ---
    laugh_cycle++;

    // Rhythm: Much slower, visible squash & stretch (Real Jelly feel)
    float laugh_speed = 1.5f; // ~2 Hz (Slower and Smoother)
    float laugh_wave = sinf(phase_rad * laugh_speed);

    // Soft bounce (no hard clipping)
    float bounce = (laugh_wave > 0) ? laugh_wave : (laugh_wave * 0.2f);
    shake_y = -(int16_t)(8.0f * bounce * intensity_scale);

    // Squint syncs with bounce
    squint_offset = (int16_t)(4.0f * bounce * intensity_scale);

    shake_x = 0;

  } else if (current_emotion == EMO_SLEEP) {
    // 1. ASYMMETRIC BREATH: Snore rhythm (Deep long breath, quick rest)
    float sleep_rad = (float)breath_phase * 0.01745f;

    // Create a "heaving" effect: slow buildup, quick drop
    // Using a biased sine wave for organic feel
    float wave = sinf(sleep_rad);
    float heave = (wave > 0) ? powf(wave, 1.5f) : (wave * 0.5f);

    // 2. YAWNING MOUTH LOGIC
    // Mouth opens during the "inhale" (heave > 0)
    if (sleep_mouth) {
      float mouth_open = (heave > 0) ? heave : 0.0f;
      int16_t m_h = (int16_t)(5.0f + 40.0f * mouth_open);
      int16_t m_w = (int16_t)(10.0f + 20.0f * mouth_open);
      lv_obj_set_size(sleep_mouth, m_w, m_h);
      lv_obj_align(sleep_mouth, LV_ALIGN_CENTER, (int16_t)current_gaze_x,
                   100 + (int16_t)current_gaze_y +
                       (int16_t)(heave * 5.0f)); // Lowered to 100
      lv_obj_set_style_opa(sleep_mouth, 100 + (int16_t)(155 * mouth_open), 0);
    }

    // 3. SQUASH & STRETCH: Eyes get wider and flatter when inhaling
    float width_mod = 0.9f + (0.25f * (heave + 1.0f) * 0.5f); // 0.9 to 1.15

    // Squint even harder when yawning wide!
    float squint_fac = (heave > 0.5f) ? (1.0f + (heave - 0.5f) * 0.8f) : 1.0f;
    float bend_mod =
        0.015f * (1.0f - 0.4f * heave * squint_fac); // Flatter when wide

    // 4. VERTICAL DIP: Face moves down more heavily
    float eye_dip =
        15.0f + (8.0f * heave); // Baseline 15px down, fluctuates 8px

    for (int i = 0; i < LAUGH_EYE_POINTS; i++) {
      float t = (float)i / (LAUGH_EYE_POINTS - 1);
      int16_t x = (int16_t)(t * 90.0f * width_mod); // Slightly wider sweep

      // y = peak_y - a*(x-center)^2
      // To get 'U' shape (center is lowest/largest Y):
      // Formula: y = eye_dip - bend_mod * dx * dx (Wait, Y increases downwards)
      // If center (dx=0) should be at bottom, it needs the LARGEST y.
      // So y = eye_dip + (bend_mod * dx * dx) makes center at top.
      // We want y = eye_dip - (bend_mod * dx * dx) ? No.

      // Let's be explicit:
      // center_y = 40 (peak bottom)
      // end_y = 20 (up)
      // y = peak_y - a * dx^2
      float center_x = 45.0f * width_mod;
      float dx = (float)x - center_x;
      // parabolic curve where center is the most bottom point (largest Y)
      // Actually, in image center is LOWER than ends.
      // Peak Y at center = eye_dip + constant
      int16_t y = (int16_t)eye_dip - (int16_t)(bend_mod * dx * dx);

      sleep_eye_pts[i].x = x;
      sleep_eye_pts[i].y = y;
    }

    if (left_sleep_eye) {
      lv_line_set_points(left_sleep_eye, sleep_eye_pts, LAUGH_EYE_POINTS);
      lv_obj_set_style_line_width(left_sleep_eye, 24, 0); // Fatter eyes!
      // Dim opacity slightly to simulate closed lids/low power
      lv_obj_set_style_line_opa(left_sleep_eye, 160 + (int16_t)(40 * heave), 0);
    }
    if (right_sleep_eye) {
      lv_line_set_points(right_sleep_eye, sleep_eye_pts, LAUGH_EYE_POINTS);
      lv_obj_set_style_line_width(right_sleep_eye, 24, 0); // Fatter eyes!
      lv_obj_set_style_line_opa(right_sleep_eye, 160 + (int16_t)(40 * heave),
                                0);
    }

  } else {
    // Reset
    laugh_intensity = 0;
    laugh_cycle = 0;

    int16_t def_h = 20;
    for (int i = 0; i < LAUGH_EYE_POINTS; i++) {
      left_arrow_pts[i].y = def_h;
      right_arrow_pts[i].y = def_h;
      // X doesn't matter much as hidden, but safe to keep range 0-100?
      // Or just let it be reused.
    }
  }

  // -----------------------------------------------------------------------------
  // 1. SPRING-DAMPER PHYSICS UPDATE (Elastic Gaze)
  // -----------------------------------------------------------------------------
  // Calculate force: F = k * (target - current)
  float force_x = (target_gaze_x - current_gaze_x) * GAZE_SPRING_K;
  float force_y = (target_gaze_y - current_gaze_y) * GAZE_SPRING_K;

  // Update velocity: V = (V + force) * damping
  gaze_vel_x = (gaze_vel_x + force_x) * GAZE_DAMPING;
  gaze_vel_y = (gaze_vel_y + force_y) * GAZE_DAMPING;

  // Update position
  current_gaze_x += gaze_vel_x;
  current_gaze_y += gaze_vel_y;

  // Precision stop to avoid infinite tiny updates
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

  // Convert float to int needed by LVGL
  int16_t final_gaze_x = (int16_t)current_gaze_x;
  int16_t final_gaze_y = (int16_t)current_gaze_y;

  // Backward compatibility alias for rest of the function
  int16_t gaze_x = final_gaze_x;
  int16_t gaze_y = final_gaze_y;

  if (left_eye && right_eye) {
    // 3D Rolling Effect: Eyes tilt slightly when looking to the sides
    // Max tilt ~8 degrees. Positive X looks right -> clockwise roll.
    int16_t roll = (int16_t)(final_gaze_x * 0.08f); // Simple scaling

    lv_obj_align(left_eye, LV_ALIGN_CENTER, -EYE_OFFSET_X + final_gaze_x,
                 breath + final_gaze_y);
    lv_obj_align(right_eye, LV_ALIGN_CENTER, EYE_OFFSET_X + final_gaze_x,
                 breath + final_gaze_y);

    // Apply rotation for depth feel
    lv_obj_set_style_transform_angle(left_eye, roll * 10, 0);
    lv_obj_set_style_transform_angle(right_eye, roll * 10, 0);
  }

  if (left_laugh_eye && right_laugh_eye && current_emotion == EMO_LAUGH) {
    // Eyes bounce with shake_y
    lv_obj_align(left_laugh_eye, LV_ALIGN_CENTER,
                 -EYE_OFFSET_X + gaze_x + shake_x,
                 LAUGH_EYE_OFFSET_Y + shake_y + gaze_y);
    lv_obj_align(right_laugh_eye, LV_ALIGN_CENTER,
                 EYE_OFFSET_X + gaze_x + shake_x,
                 LAUGH_EYE_OFFSET_Y + shake_y + gaze_y);
  }

  // BIG MOUTH Position (Laugh) - Animated Jaw Position
  // BIG MOUTH Position (Laugh) - Jelly Squash & Stretch Animation
  // BIG MOUTH / GRIN MOUTH Animation (2-Frame Toggle)
  // BIG MOUTH / GRIN MOUTH Animation (2-Frame Toggle)
  if (current_emotion == EMO_LAUGH) {
    // SYNC WITH WAVE (Calculated using same phase/speed as eyes)
    float wave_speed = 1.5f;
    float laugh_wave = sinf(laugh_shake_phase * 0.01745f * wave_speed);

    // Toggle Threshold
    // > -0.2 : Morph towards Open
    bool show_open = (laugh_wave > -0.2f);

    if (show_open) {
      // Frame 1: OPEN MOUTH (Pink)
      if (big_laugh_mouth) {
        lv_obj_clear_flag(big_laugh_mouth, LV_OBJ_FLAG_HIDDEN);

        lv_obj_align(big_laugh_mouth, LV_ALIGN_CENTER, gaze_x + shake_x,
                     BIG_MOUTH_OFFSET_Y + shake_y + gaze_y);
      }
      if (grin_laugh_mouth)
        lv_obj_add_flag(grin_laugh_mouth, LV_OBJ_FLAG_HIDDEN);
    } else {
      // Frame 2: GRIN MOUTH (Teeth)
      if (grin_laugh_mouth) {
        lv_obj_clear_flag(grin_laugh_mouth, LV_OBJ_FLAG_HIDDEN);
        lv_obj_align(grin_laugh_mouth, LV_ALIGN_CENTER, gaze_x + shake_x,
                     BIG_MOUTH_OFFSET_Y + shake_y + gaze_y);
      }
      if (big_laugh_mouth)
        lv_obj_add_flag(big_laugh_mouth, LV_OBJ_FLAG_HIDDEN);
    }
  }

  // Happy Idle Animation & Position Update
  if (current_emotion == EMO_HAPPY) {
    // 1. Gentle bounce
    float smile_rad = (float)breath_phase * 0.01745f;
    int16_t happy_bounce = (int16_t)(4.0f * sinf(smile_rad));

    // 2. DYNAMIC SMILE MORPHING (The "Organic" part)
    // Smile gets wider and deeper when "inhaling"
    float smile_intensity = 0.5f + 0.5f * sinf(smile_rad); // 0.0 to 1.0
    float width = 110.0f + (20.0f * smile_intensity);
    float depth = 12.0f + (8.0f * smile_intensity);
    float mid_peak = 6.0f * smile_intensity; // Center point moves up slightly

    for (int i = 0; i < HAPPY_MOUTH_POINTS; i++) {
      float t = (float)i / (HAPPY_MOUTH_POINTS - 1); // 0 to 1
      int16_t x = (int16_t)(t * width);

      // Two-part parabola for the "w" (cat smile) shape
      float y;
      if (t < 0.5f) {
        // Left half parabola: peak at 0.25
        float dt = (t - 0.25f) / 0.25f;
        y = depth * (1.0f - dt * dt);
      } else {
        // Right half parabola: peak at 0.75
        float dt = (t - 0.75f) / 0.25f;
        y = depth * (1.0f - dt * dt);
      }

      // Blend center to be a bit higher (Omega style)
      if (t > 0.4f && t < 0.6f) {
        float mid_t = (t - 0.4f) / 0.1f; // 0 to 2
        if (mid_t > 1.0f)
          mid_t = 2.0f - mid_t; // triangle 0->1->0
        y -= mid_peak * mid_t;
      }

      happy_mouth_pts[i].x = x;
      happy_mouth_pts[i].y = (int16_t)y;
    }

    if (happy_mouth) {
      lv_line_set_points(happy_mouth, happy_mouth_pts, HAPPY_MOUTH_POINTS);
      lv_obj_align(happy_mouth, LV_ALIGN_CENTER, (int16_t)current_gaze_x,
                   OMEGA_MOUTH_OFFSET_Y + happy_bounce +
                       (int16_t)current_gaze_y);
    }

    // Update Cheeks Position (Bounce with mouth)
    if (left_cheek)
      lv_obj_align(left_cheek, LV_ALIGN_CENTER, -EYE_OFFSET_X + gaze_x,
                   CHEEK_OFFSET_Y_VISIBLE + happy_bounce + gaze_y);
    if (right_cheek)
      lv_obj_align(right_cheek, LV_ALIGN_CENTER, EYE_OFFSET_X + gaze_x,
                   CHEEK_OFFSET_Y_VISIBLE + happy_bounce + gaze_y);
  }

  // Common Eye Update (Re-apply final coords for other components if needed)
  // (Left/Right eyes already handled at top of function)

  // Laugh Eye Update
  if (left_laugh_eye && right_laugh_eye && current_emotion == EMO_LAUGH) {
    lv_obj_align(left_laugh_eye, LV_ALIGN_CENTER,
                 -EYE_OFFSET_X + gaze_x + shake_x - squint_offset,
                 LAUGH_EYE_OFFSET_Y + shake_y + gaze_y);
    lv_obj_align(right_laugh_eye, LV_ALIGN_CENTER,
                 EYE_OFFSET_X + gaze_x + shake_x + squint_offset,
                 LAUGH_EYE_OFFSET_Y + shake_y + gaze_y);
  }

  // Sleep Eye Position
  if (current_emotion == EMO_SLEEP) {
    if (left_sleep_eye)
      lv_obj_align(left_sleep_eye, LV_ALIGN_CENTER,
                   -EYE_OFFSET_X + final_gaze_x, breath + final_gaze_y + 10);
    if (right_sleep_eye)
      lv_obj_align(right_sleep_eye, LV_ALIGN_CENTER,
                   EYE_OFFSET_X + final_gaze_x, breath + final_gaze_y + 10);
  }

  // -----------------------------------------------------------------------------
  // LOVE Emotion - Romantic Heartbeat Animation
  // -----------------------------------------------------------------------------
  if (current_emotion == EMO_LOVE) {
    // Build up love intensity smoothly
    if (love_intensity < 100)
      love_intensity += 3; // Slower fade in
    float intensity_scale = love_intensity / 100.0f;

    // Heartbeat phase - realistic heart rhythm
    heart_beat_phase += 8.0f; // Slower, more romantic pace
    if (heart_beat_phase >= 360.0f)
      heart_beat_phase = 0.0f;
    float beat_rad = heart_beat_phase * 0.01745f;

    // Heartbeat scale pulse (lub-dub rhythm)
    // Double-beat pattern like real heartbeat
    float beat_wave = 0.0f;
    float phase_norm = heart_beat_phase / 360.0f;

    if (phase_norm < 0.12f) {
      // First beat "LUB" (0-12%) - stronger
      float t = phase_norm / 0.12f;
      beat_wave = sinf(t * 3.14159f);
    } else if (phase_norm < 0.20f) {
      // Small pause
      beat_wave = 0.0f;
    } else if (phase_norm < 0.32f) {
      // Second beat "DUB" (20-32%) - slightly weaker
      float t = (phase_norm - 0.20f) / 0.12f;
      beat_wave = sinf(t * 3.14159f) * 0.65f;
    }
    // else: rest of cycle is rest pause (realistic)

    // Smooth the wave
    beat_wave = beat_wave * beat_wave; // Square for snappier pulse

    // Calculate scale: 256 = 100%, pulse between 100% and 120%
    float base_zoom = 256.0f;
    float pulse_amount = 50.0f * intensity_scale; // Max 50 zoom units pulse
    int16_t heart_zoom = (int16_t)(base_zoom + pulse_amount * beat_wave);

    // Gentle floating animation (dreamy up/down)
    float float_amount = 6.0f * sinf(beat_rad * 0.3f) * intensity_scale;
    int16_t float_y = (int16_t)float_amount;

    // Subtle wobble effect (hearts move slightly towards each other)
    float wobble = 3.0f * sinf(beat_rad * 0.5f) * intensity_scale;

    // Update heart eyes with all effects
    if (left_heart_eye) {
      lv_img_set_zoom(left_heart_eye, heart_zoom);
      lv_obj_align(left_heart_eye, LV_ALIGN_CENTER,
                   -HEART_EYE_OFFSET_X + gaze_x + (int16_t)wobble,
                   float_y + gaze_y);
    }
    if (right_heart_eye) {
      lv_img_set_zoom(right_heart_eye, heart_zoom);
      lv_obj_align(right_heart_eye, LV_ALIGN_CENTER,
                   HEART_EYE_OFFSET_X + gaze_x - (int16_t)wobble,
                   float_y + gaze_y);
    }
  } else {
    // Reset love state when not in LOVE emotion
    love_intensity = 0;
  }
}

// -----------------------------------------------------------------------------
// Animations
// -----------------------------------------------------------------------------
static void anim_height_cb(void *var, int32_t v) {
  if (left_eye)
    lv_obj_set_height(left_eye, v);
  if (right_eye)
    lv_obj_set_height(right_eye, v);
}

static void do_blink(void) {
  if (current_emotion == EMO_LAUGH)
    return;

  int32_t start_h = (current_emotion == EMO_SAD) ? EYE_HEIGHT_SAD : EYE_HEIGHT;
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, NULL);
  lv_anim_set_values(&a, start_h, 10);
  lv_anim_set_time(&a, 80);
  lv_anim_set_playback_time(&a, 100);
  lv_anim_set_exec_cb(&a, anim_height_cb);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
  lv_anim_start(&a);
}

static void anim_cheek_cb(void *var, int32_t v) {
  if (left_cheek)
    lv_obj_align(left_cheek, LV_ALIGN_CENTER, -EYE_OFFSET_X, v);
  if (right_cheek)
    lv_obj_align(right_cheek, LV_ALIGN_CENTER, EYE_OFFSET_X, v);
}

// Animation happy mouth opacity
static void anim_happy_mouth_opa_cb(void *var, int32_t v) {
  if (happy_mouth) {
    lv_obj_set_style_line_opa(happy_mouth, v, 0);
  }
}

// Animation happy mouth Y position (slide up)
static void anim_happy_mouth_y_cb(void *var, int32_t v) {
  if (happy_mouth) {
    lv_obj_align(happy_mouth, LV_ALIGN_CENTER, 0, v); // Fix X offset to 0
  }
}

static void show_happy(void) {
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, NULL);
  lv_anim_set_values(&a, CHEEK_OFFSET_Y_HIDDEN, CHEEK_OFFSET_Y_VISIBLE);
  lv_anim_set_time(&a, 400);
  lv_anim_set_exec_cb(&a, anim_cheek_cb);
  lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
  lv_anim_start(&a);

  // Show Happy Mouth
  if (happy_mouth) {
    lv_obj_set_style_line_opa(happy_mouth, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(happy_mouth, LV_OBJ_FLAG_HIDDEN);

    // Opacity
    lv_anim_t b;
    lv_anim_init(&b);
    lv_anim_set_var(&b, NULL);
    lv_anim_set_values(&b, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&b, 300);
    lv_anim_set_exec_cb(&b, anim_happy_mouth_opa_cb);
    lv_anim_set_path_cb(&b, lv_anim_path_ease_in);
    lv_anim_start(&b);

    // Slide Up (from lower position)
    lv_anim_t c;
    lv_anim_init(&c);
    lv_anim_set_var(&c, NULL);
    lv_anim_set_values(&c, OMEGA_MOUTH_OFFSET_Y + 80, OMEGA_MOUTH_OFFSET_Y);
    lv_anim_set_time(&c, 400);
    lv_anim_set_exec_cb(&c, anim_happy_mouth_y_cb);
    lv_anim_set_path_cb(&c, lv_anim_path_overshoot);
    lv_anim_start(&c);
  }
}

static void hide_happy_anim_done(lv_anim_t *a) {
  if (happy_mouth)
    lv_obj_add_flag(happy_mouth, LV_OBJ_FLAG_HIDDEN);
}

static void hide_happy(void) {
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, NULL);
  lv_anim_set_values(&a, CHEEK_OFFSET_Y_VISIBLE, CHEEK_OFFSET_Y_HIDDEN);
  lv_anim_set_time(&a, 300);
  lv_anim_set_exec_cb(&a, anim_cheek_cb);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
  lv_anim_start(&a);

  // Hide Happy Mouth
  if (happy_mouth) {
    // Opacity
    lv_anim_t b;
    lv_anim_init(&b);
    lv_anim_set_var(&b, NULL);
    lv_anim_set_values(&b, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&b, 200);
    lv_anim_set_exec_cb(&b, anim_happy_mouth_opa_cb);
    lv_anim_set_ready_cb(&b, hide_happy_anim_done);
    lv_anim_start(&b);

    // Slide Down
    lv_anim_t c;
    lv_anim_init(&c);
    lv_anim_set_var(&c, NULL);
    lv_anim_set_values(&c, OMEGA_MOUTH_OFFSET_Y, OMEGA_MOUTH_OFFSET_Y + 80);
    lv_anim_set_time(&c, 300);
    lv_anim_set_exec_cb(&c, anim_happy_mouth_y_cb);
    lv_anim_set_path_cb(&c, lv_anim_path_ease_in);
    lv_anim_start(&c);
  }
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

// Animation callbacks for laugh transition
static void anim_big_mouth_opa_cb(void *var, int32_t v) {
  if (big_laugh_mouth) {
    lv_obj_set_style_bg_opa(big_laugh_mouth, v, 0);
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

  // Hide normal eyes
  if (left_eye)
    lv_obj_add_flag(left_eye, LV_OBJ_FLAG_HIDDEN);
  if (right_eye)
    lv_obj_add_flag(right_eye, LV_OBJ_FLAG_HIDDEN);

  // Show laugh eyes with animation
  if (left_laugh_eye) {
    lv_obj_set_style_line_width(left_laugh_eye, 4, 0); // Start thin
    lv_obj_clear_flag(left_laugh_eye, LV_OBJ_FLAG_HIDDEN);
  }
  if (right_laugh_eye) {
    lv_obj_set_style_line_width(right_laugh_eye, 4, 0);
    lv_obj_clear_flag(right_laugh_eye, LV_OBJ_FLAG_HIDDEN);
  }

  // Animate eyes getting thicker (squint effect)
  lv_anim_t eye_anim;
  lv_anim_init(&eye_anim);
  lv_anim_set_var(&eye_anim, NULL);
  lv_anim_set_values(&eye_anim, 4, 14);
  lv_anim_set_time(&eye_anim, 200);
  lv_anim_set_exec_cb(&eye_anim, anim_laugh_eye_scale_cb);
  lv_anim_set_path_cb(&eye_anim, lv_anim_path_overshoot);
  lv_anim_start(&eye_anim);

  // Show BIG MOUTH with fade in
  if (big_laugh_mouth) {
    lv_obj_set_style_bg_opa(big_laugh_mouth, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(big_laugh_mouth, LV_OBJ_FLAG_HIDDEN);

    lv_anim_t mouth_anim;
    lv_anim_init(&mouth_anim);
    lv_anim_set_var(&mouth_anim, NULL);
    lv_anim_set_values(&mouth_anim, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&mouth_anim, 300);
    lv_anim_set_exec_cb(&mouth_anim, anim_big_mouth_opa_cb);
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

  // Show normal eyes ONLY if not transitioning to special emotions
  if (current_emotion != EMO_LOVE && current_emotion != EMO_SLEEP) {
    if (left_eye)
      lv_obj_clear_flag(left_eye, LV_OBJ_FLAG_HIDDEN);
    if (right_eye)
      lv_obj_clear_flag(right_eye, LV_OBJ_FLAG_HIDDEN);
  }
}

static void hide_laugh(void) {
  // Fade out BIG MOUTH
  if (big_laugh_mouth) {
    lv_anim_t mouth_anim;
    lv_anim_init(&mouth_anim);
    lv_anim_set_var(&mouth_anim, NULL);
    lv_anim_set_values(&mouth_anim, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&mouth_anim, 200);
    lv_anim_set_exec_cb(&mouth_anim, anim_big_mouth_opa_cb);
    lv_anim_set_ready_cb(&mouth_anim, laugh_exit_anim_done);
    lv_anim_set_path_cb(&mouth_anim, lv_anim_path_ease_in);
    lv_anim_start(&mouth_anim);
  } else {
    laugh_exit_anim_done(NULL);
  }

  if (grin_laugh_mouth)
    lv_obj_add_flag(grin_laugh_mouth, LV_OBJ_FLAG_HIDDEN);

  // Animate eyes getting thinner
  lv_anim_t eye_anim;
  lv_anim_init(&eye_anim);
  lv_anim_set_var(&eye_anim, NULL);
  lv_anim_set_values(&eye_anim, 14, 4);
  lv_anim_set_time(&eye_anim, 200);
  lv_anim_set_exec_cb(&eye_anim, anim_laugh_eye_scale_cb);
  lv_anim_set_path_cb(&eye_anim, lv_anim_path_ease_in);
  lv_anim_start(&eye_anim);
}

// -----------------------------------------------------------------------------
// LOVE Emotion - Heart Eyes with Heartbeat Animation
// -----------------------------------------------------------------------------
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

static void anim_heart_opa_cb(void *var, int32_t v) {
  if (left_heart_eye) {
    lv_obj_set_style_img_opa(left_heart_eye, v, 0);
  }
  if (right_heart_eye) {
    lv_obj_set_style_img_opa(right_heart_eye, v, 0);
  }
}

static void love_exit_anim_done(lv_anim_t *a) {
  // Hide heart eyes after animation
  if (left_heart_eye)
    lv_obj_add_flag(left_heart_eye, LV_OBJ_FLAG_HIDDEN);
  if (right_heart_eye)
    lv_obj_add_flag(right_heart_eye, LV_OBJ_FLAG_HIDDEN);

  // Show normal eyes ONLY if not in a special emotion state
  if (current_emotion != EMO_SLEEP && current_emotion != EMO_LAUGH &&
      current_emotion != EMO_LOVE) {
    if (left_eye)
      lv_obj_clear_flag(left_eye, LV_OBJ_FLAG_HIDDEN);
    if (right_eye)
      lv_obj_clear_flag(right_eye, LV_OBJ_FLAG_HIDDEN);
  }
}

static void show_love(void) {
  // Reset love state
  love_intensity = 0;
  heart_beat_phase = 0.0f;

  // Hide normal eyes
  if (left_eye)
    lv_obj_add_flag(left_eye, LV_OBJ_FLAG_HIDDEN);
  if (right_eye)
    lv_obj_add_flag(right_eye, LV_OBJ_FLAG_HIDDEN);

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
  lv_anim_set_var(&opa_anim, NULL);
  lv_anim_set_values(&opa_anim, LV_OPA_TRANSP, LV_OPA_COVER);
  lv_anim_set_time(&opa_anim, 300);
  lv_anim_set_exec_cb(&opa_anim, anim_heart_opa_cb);
  lv_anim_set_path_cb(&opa_anim, lv_anim_path_ease_out);
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
  // Fade out hearts
  lv_anim_t opa_anim;
  lv_anim_init(&opa_anim);
  lv_anim_set_var(&opa_anim, NULL);
  lv_anim_set_values(&opa_anim, LV_OPA_COVER, LV_OPA_TRANSP);
  lv_anim_set_time(&opa_anim, 200);
  lv_anim_set_exec_cb(&opa_anim, anim_heart_opa_cb);
  lv_anim_set_ready_cb(&opa_anim, love_exit_anim_done);
  lv_anim_set_path_cb(&opa_anim, lv_anim_path_ease_in);
  lv_anim_start(&opa_anim);

  lv_anim_t scale_anim;
  lv_anim_init(&scale_anim);
  lv_anim_set_var(&scale_anim, NULL);
  lv_anim_set_values(&scale_anim, 280, 180); // 110% to 70%
  lv_anim_set_time(&scale_anim, 200);
  lv_anim_set_exec_cb(&scale_anim, anim_heart_scale_cb);
  lv_anim_set_path_cb(&scale_anim, lv_anim_path_ease_in);
  lv_anim_start(&scale_anim);
}

// -----------------------------------------------------------------------------
// SLEEP Emotion - 'zzz' Animation
// -----------------------------------------------------------------------------
static void anim_z_cb(void *var, int32_t v) {
  lv_obj_t *z = (lv_obj_t *)var;
  if (!z)
    return;

  // v: 0 to 100 (percentage of path)
  float p = v / 100.0f;

  // Fly up and right with a gentle curve
  int16_t start_y = -30;
  int16_t end_y = -140;
  int16_t start_x = 0;
  int16_t end_x = 50;

  int16_t curr_y = start_y + (int16_t)((end_y - start_y) * p);
  int16_t curr_x = start_x + (int16_t)((end_x - start_x) * p) +
                   (int16_t)(15.0f * sinf(p * 6.28f));

  lv_obj_align(z, LV_ALIGN_CENTER, curr_x, curr_y);

  // Fade out towards end
  if (p > 0.7f) {
    lv_obj_set_style_text_opa(z, (uint8_t)(255 * (1.0f - (p - 0.7f) / 0.3f)),
                              0);
  } else {
    lv_obj_set_style_text_opa(z, 255, 0);
  }

  // Scale up significantly (Base 0.8x -> 1.5x, goes up to 2.2x -> 3.0x?)
  // LVGL Zoom: 256 is 1:1.
  // Let's start larger and end even larger.
  lv_obj_set_style_transform_zoom(z, (uint16_t)(256 * (1.2f + 1.2f * p)), 0);
}

static void start_z_anim(lv_obj_t *z, uint32_t delay) {
  if (!z)
    return;
  lv_obj_clear_flag(z, LV_OBJ_FLAG_HIDDEN);

  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, z);
  lv_anim_set_values(&a, 0, 100);
  lv_anim_set_time(&a, 2500);
  lv_anim_set_delay(&a, delay);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_exec_cb(&a, anim_z_cb);
  lv_anim_set_path_cb(&a, lv_anim_path_linear);
  lv_anim_start(&a);
}

static void show_sleep(void) {
  if (left_eye)
    lv_obj_add_flag(left_eye, LV_OBJ_FLAG_HIDDEN);
  if (right_eye)
    lv_obj_add_flag(right_eye, LV_OBJ_FLAG_HIDDEN);

  if (left_sleep_eye)
    lv_obj_clear_flag(left_sleep_eye, LV_OBJ_FLAG_HIDDEN);
  if (right_sleep_eye)
    lv_obj_clear_flag(right_sleep_eye, LV_OBJ_FLAG_HIDDEN);
  if (sleep_mouth)
    lv_obj_clear_flag(sleep_mouth, LV_OBJ_FLAG_HIDDEN);

  // Make sure ZZZ are on top
  if (sleep_z1)
    lv_obj_move_foreground(sleep_z1);
  if (sleep_z2)
    lv_obj_move_foreground(sleep_z2);
  if (sleep_z3)
    lv_obj_move_foreground(sleep_z3);

  start_z_anim(sleep_z1, 0);
  start_z_anim(sleep_z2, 800);
  start_z_anim(sleep_z3, 1600);
}

static void hide_sleep(void) {
  if (left_sleep_eye)
    lv_obj_add_flag(left_sleep_eye, LV_OBJ_FLAG_HIDDEN);
  if (right_sleep_eye)
    lv_obj_add_flag(right_sleep_eye, LV_OBJ_FLAG_HIDDEN);
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

  if (current_emotion != EMO_LAUGH && current_emotion != EMO_LOVE) {
    if (left_eye)
      lv_obj_clear_flag(left_eye, LV_OBJ_FLAG_HIDDEN);
    if (right_eye)
      lv_obj_clear_flag(right_eye, LV_OBJ_FLAG_HIDDEN);
  }
}

static void gesture_event_cb(lv_event_t *e) {
  lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
  if (dir == LV_DIR_TOP)
    ui_settings_show();
}

static void main_loop(lv_timer_t *timer) {
  update_positions();
  timer_ms += 50;

  // Saccade Update (Random micro-movements)
  // Only create saccades in IDLE or HAPPY to mimic looking around
  if (current_emotion == EMO_IDLE || current_emotion == EMO_HAPPY) {
    saccade_timer += 50;
    if (saccade_timer >= next_saccade_time) {
      saccade_timer = 0;
      // Pick next time: 0.5s to 3s
      next_saccade_time = 500 + (rand() % 2500);

      // Random tiny offset (-15 to 15)
      int16_t rand_x = (rand() % 31) - 15;
      int16_t rand_y = (rand() % 21) - 10;

      // Apply to target
      target_gaze_x = (float)rand_x;
      target_gaze_y = (float)rand_y;
    }
  }

  // Random Blink
  blink_timer += 50;
  if (current_emotion != EMO_LAUGH &&
      current_emotion != EMO_LOVE) { // Shake/Love overrides blink
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

  switch (current_emotion) {
  case EMO_IDLE:
    if (timer_ms >= 10000) { // Slower emotion switch
      show_happy();
      current_emotion = EMO_HAPPY;
      timer_ms = 0;
    }
    break;
  case EMO_HAPPY:
    if (timer_ms >= 5000) {
      hide_happy();
      show_sad();
      current_emotion = EMO_SAD;
      timer_ms = 0;
    }
    break;
  case EMO_SAD:
    if (timer_ms >= 4000) {
      hide_sad();
      show_laugh();
      current_emotion = EMO_LAUGH;
      timer_ms = 0;
    }
    break;
  case EMO_LAUGH:
    if (timer_ms >= 4000) {
      hide_laugh();
      show_love();
      current_emotion = EMO_LOVE;
      timer_ms = 0;
    }
    break;
  case EMO_LOVE:
    // Heartbeat animation runs in update_positions()
    if (timer_ms >= 4000) {
      hide_love();
      show_sleep();
      current_emotion = EMO_SLEEP;
      timer_ms = 0;
    }
    break;
  case EMO_SLEEP:
    if (timer_ms >= 8000) {
      hide_sleep();
      current_emotion = EMO_IDLE;
      timer_ms = 0;
    }
    break;
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
    hl_dsc.bg_opa = LV_OPA_80;
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

void ui_robo_eyes_init(void) {
  if (scr_eyes)
    return;

  scr_eyes = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr_eyes, BG_COLOR, 0);
  lv_obj_set_style_bg_opa(scr_eyes, LV_OPA_COVER, 0);
  lv_obj_clear_flag(scr_eyes, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(scr_eyes, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(scr_eyes, gesture_event_cb, LV_EVENT_GESTURE, NULL);

  lv_style_init(&style_eye);
  lv_style_set_radius(&style_eye, EYE_RADIUS);
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

  right_eye = lv_obj_create(scr_eyes);
  lv_obj_add_style(right_eye, &style_eye, 0);
  lv_obj_set_size(right_eye, EYE_WIDTH, EYE_HEIGHT);
  lv_obj_align(right_eye, LV_ALIGN_CENTER, EYE_OFFSET_X, 0);
  lv_obj_clear_flag(right_eye, LV_OBJ_FLAG_SCROLLABLE);

  left_laugh_eye = lv_line_create(scr_eyes);
  lv_line_set_points(left_laugh_eye, left_arrow_pts, LAUGH_EYE_POINTS);
  lv_obj_add_style(left_laugh_eye, &style_line, 0);
  lv_obj_align(left_laugh_eye, LV_ALIGN_CENTER, -EYE_OFFSET_X, 0);
  lv_obj_add_flag(left_laugh_eye, LV_OBJ_FLAG_HIDDEN);

  right_laugh_eye = lv_line_create(scr_eyes);
  lv_line_set_points(right_laugh_eye, right_arrow_pts, LAUGH_EYE_POINTS);
  lv_obj_add_style(right_laugh_eye, &style_line, 0);
  lv_obj_align(right_laugh_eye, LV_ALIGN_CENTER, EYE_OFFSET_X, 0);
  lv_obj_add_flag(right_laugh_eye, LV_OBJ_FLAG_HIDDEN);

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

  // Create right heart eye from canvas
  right_heart_eye = lv_img_create(scr_eyes);
  lv_img_set_src(right_heart_eye, lv_canvas_get_img(canvas_heart_ref));
  lv_obj_align(right_heart_eye, LV_ALIGN_CENTER, HEART_EYE_OFFSET_X, 0);
  lv_obj_add_flag(right_heart_eye, LV_OBJ_FLAG_HIDDEN);

  // Create omega mouth "ω" shape (Keep hidden/unused for laugh now)
  lv_style_init(&style_omega_mouth);
  lv_style_set_line_width(&style_omega_mouth, 12);
  lv_style_set_line_color(&style_omega_mouth, EYE_COLOR);
  lv_style_set_line_rounded(&style_omega_mouth, true);

  omega_mouth = lv_line_create(scr_eyes);
  lv_line_set_points(omega_mouth, omega_mouth_pts, 5);
  lv_obj_add_style(omega_mouth, &style_omega_mouth, 0);
  lv_obj_align(omega_mouth, LV_ALIGN_CENTER, -OMEGA_MOUTH_WIDTH / 2,
               OMEGA_MOUTH_OFFSET_Y);
  lv_obj_add_flag(omega_mouth, LV_OBJ_FLAG_HIDDEN);

  // Create cheeks FIRST (so mouth is on top)
  left_cheek = lv_obj_create(scr_eyes);
  lv_obj_add_style(left_cheek, &style_mask, 0);
  lv_obj_set_size(left_cheek, CHEEK_SIZE, CHEEK_SIZE);
  lv_obj_align(left_cheek, LV_ALIGN_CENTER, -EYE_OFFSET_X,
               CHEEK_OFFSET_Y_HIDDEN);
  lv_obj_clear_flag(left_cheek, LV_OBJ_FLAG_SCROLLABLE);

  right_cheek = lv_obj_create(scr_eyes);
  lv_obj_add_style(right_cheek, &style_mask, 0);
  lv_obj_set_size(right_cheek, CHEEK_SIZE, CHEEK_SIZE);
  lv_obj_align(right_cheek, LV_ALIGN_CENTER, EYE_OFFSET_X,
               CHEEK_OFFSET_Y_HIDDEN);
  lv_obj_clear_flag(right_cheek, LV_OBJ_FLAG_SCROLLABLE);

  // Create Happy Mouth "~" AFTER cheeks
  happy_mouth = lv_line_create(scr_eyes);
  // Points set in update_positions
  lv_obj_add_style(happy_mouth, &style_omega_mouth, 0); // Reuse style (rounded)
  lv_obj_add_flag(happy_mouth, LV_OBJ_FLAG_HIDDEN);

  left_sad_mask = lv_obj_create(scr_eyes);
  lv_obj_add_style(left_sad_mask, &style_mask, 0);
  lv_obj_set_size(left_sad_mask, SAD_MASK_SIZE, SAD_MASK_SIZE);
  lv_obj_align(left_sad_mask, LV_ALIGN_CENTER,
               -EYE_OFFSET_X - SAD_MASK_OFFSET_X, SAD_MASK_OFFSET_Y_HIDDEN);
  lv_obj_clear_flag(left_sad_mask, LV_OBJ_FLAG_SCROLLABLE);

  right_sad_mask = lv_obj_create(scr_eyes);
  lv_obj_add_style(right_sad_mask, &style_mask, 0);
  lv_obj_set_size(right_sad_mask, SAD_MASK_SIZE, SAD_MASK_SIZE);
  lv_obj_align(right_sad_mask, LV_ALIGN_CENTER,
               EYE_OFFSET_X + SAD_MASK_OFFSET_X, SAD_MASK_OFFSET_Y_HIDDEN);
  lv_obj_clear_flag(right_sad_mask, LV_OBJ_FLAG_SCROLLABLE);

  left_tear = create_tear_image(scr_eyes);
  // lv_obj_add_style(left_tear, &style_tear_part, 0); // Removed to fix square
  // shadow box
  lv_obj_align(left_tear, LV_ALIGN_CENTER, -EYE_OFFSET_X, TEAR_START_Y);
  lv_obj_clear_flag(left_tear, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(left_tear, LV_OBJ_FLAG_HIDDEN);

  right_tear = create_tear_image(scr_eyes);
  // lv_obj_add_style(right_tear, &style_tear_part, 0); // Removed to fix square
  // shadow box
  lv_obj_align(right_tear, LV_ALIGN_CENTER, EYE_OFFSET_X, TEAR_START_Y);
  lv_obj_clear_flag(right_tear, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(right_tear, LV_OBJ_FLAG_HIDDEN);

  // --- Sleep Elements ---
  left_sleep_eye = lv_line_create(scr_eyes);
  lv_obj_add_style(left_sleep_eye, &style_line, 0);
  lv_obj_add_flag(left_sleep_eye, LV_OBJ_FLAG_HIDDEN);

  right_sleep_eye = lv_line_create(scr_eyes);
  lv_obj_add_style(right_sleep_eye, &style_line, 0);
  lv_obj_add_flag(right_sleep_eye, LV_OBJ_FLAG_HIDDEN);

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

  // ZZZ Labels
  static lv_style_t style_z; // MUST BE STATIC
  lv_style_init(&style_z);
  lv_style_set_text_color(&style_z, EYE_COLOR);
  lv_style_set_text_font(
      &style_z, &lv_font_montserratMedium_23); // Standard LVGL font (larger)

  sleep_z1 = lv_label_create(scr_eyes);
  lv_label_set_text(sleep_z1, "Z");
  lv_obj_add_style(sleep_z1, &style_z, 0);
  lv_obj_add_flag(sleep_z1, LV_OBJ_FLAG_HIDDEN);

  sleep_z2 = lv_label_create(scr_eyes);
  lv_label_set_text(sleep_z2, "z");
  lv_obj_add_style(sleep_z2, &style_z, 0);
  lv_obj_add_flag(sleep_z2, LV_OBJ_FLAG_HIDDEN);

  sleep_z3 = lv_label_create(scr_eyes);
  lv_label_set_text(sleep_z3, "z");
  lv_obj_add_style(sleep_z3, &style_z, 0);
  lv_obj_add_flag(sleep_z3, LV_OBJ_FLAG_HIDDEN);

  lv_scr_load(scr_eyes);
  srand(12345);
  main_timer = lv_timer_create(main_loop, 50, NULL);
}

// --- API ---
void ui_robo_eyes_set_emotion_type(robot_emotion_t emotion) {}
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
robot_emotion_t ui_robo_eyes_get_emotion(void) { return EMOTION_NORMAL; }
