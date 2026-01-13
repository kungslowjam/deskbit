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

// Screen constants
#define SCREEN_WIDTH 466
#define SCREEN_HEIGHT 466

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
#define MOUTH_WIDTH 200
#define MOUTH_HEIGHT 100
#define MOUTH_OFFSET_Y 60
#define MOUTH_RADIUS LV_RADIUS_CIRCLE

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
static lv_obj_t *mouth_cont = NULL;
static lv_obj_t *mouth_circle = NULL;

// Canvas Teardrop Single Instance Cache
static lv_obj_t *canvas_tear_ref = NULL;

static lv_timer_t *main_timer = NULL;

// State
typedef enum { EMO_IDLE, EMO_HAPPY, EMO_SAD, EMO_LAUGH } emotion_t;
static emotion_t current_emotion = EMO_IDLE;
static int timer_ms = 0;
static int breath_phase = 0;
static int16_t gaze_x = 0;
static int16_t gaze_y = 0;
static int laugh_shake_phase = 0;

// Styles
static lv_style_t style_eye;
static lv_style_t style_mask;
static lv_style_t style_tear_part;
static lv_style_t style_line;

// Line Points for > < (Laugh Eyes - wide and flat)
static lv_point_t left_arrow_pts[] = {{0, 0}, {LAUGH_EYE_WIDTH, 15}, {0, 30}};
static lv_point_t right_arrow_pts[] = {
    {LAUGH_EYE_WIDTH, 0}, {0, 15}, {LAUGH_EYE_WIDTH, 30}};

// -----------------------------------------------------------------------------
// Position Update
// -----------------------------------------------------------------------------
static void update_positions(void) {
  breath_phase += 3;
  if (breath_phase >= 360)
    breath_phase = 0;

  int16_t breath = (int16_t)(3.0f * sinf(breath_phase * 0.01745f));

  // Laugh shake effect - faster oscillation
  int16_t shake_x = 0;
  int16_t shake_y = 0;
  if (current_emotion == EMO_LAUGH) {
    laugh_shake_phase += 25; // Fast shake
    if (laugh_shake_phase >= 360)
      laugh_shake_phase = 0;
    shake_x = (int16_t)(3.0f * sinf(laugh_shake_phase * 0.01745f));
    shake_y = (int16_t)(4.0f * sinf(laugh_shake_phase * 2.0f * 0.01745f));
  }

  if (left_eye && right_eye) {
    lv_obj_align(left_eye, LV_ALIGN_CENTER, -EYE_OFFSET_X + gaze_x,
                 breath + gaze_y);
    lv_obj_align(right_eye, LV_ALIGN_CENTER, EYE_OFFSET_X + gaze_x,
                 breath + gaze_y);
  }
  if (left_laugh_eye && right_laugh_eye && current_emotion == EMO_LAUGH) {
    lv_obj_align(left_laugh_eye, LV_ALIGN_CENTER,
                 -EYE_OFFSET_X + gaze_x + shake_x,
                 LAUGH_EYE_OFFSET_Y + shake_y + gaze_y);
    lv_obj_align(right_laugh_eye, LV_ALIGN_CENTER,
                 EYE_OFFSET_X + gaze_x + shake_x,
                 LAUGH_EYE_OFFSET_Y + shake_y + gaze_y);
  }
  if (mouth_cont && current_emotion == EMO_LAUGH) {
    lv_obj_align(mouth_cont, LV_ALIGN_CENTER, gaze_x + shake_x,
                 MOUTH_OFFSET_Y + shake_y + gaze_y);
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

static void show_happy(void) {
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, NULL);
  lv_anim_set_values(&a, CHEEK_OFFSET_Y_HIDDEN, CHEEK_OFFSET_Y_VISIBLE);
  lv_anim_set_time(&a, 400);
  lv_anim_set_exec_cb(&a, anim_cheek_cb);
  lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
  lv_anim_start(&a);
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
  int16_t x = (tear == left_tear) ? (-EYE_OFFSET_X + TEAR_OFFSET_X + gaze_x)
                                  : (EYE_OFFSET_X - TEAR_OFFSET_X + gaze_x);
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
  int16_t x = (tear == left_tear) ? (-EYE_OFFSET_X + TEAR_OFFSET_X + gaze_x)
                                  : (EYE_OFFSET_X - TEAR_OFFSET_X + gaze_x);
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

static void show_laugh(void) {
  if (left_eye)
    lv_obj_add_flag(left_eye, LV_OBJ_FLAG_HIDDEN);
  if (right_eye)
    lv_obj_add_flag(right_eye, LV_OBJ_FLAG_HIDDEN);
  if (left_laugh_eye)
    lv_obj_clear_flag(left_laugh_eye, LV_OBJ_FLAG_HIDDEN);
  if (right_laugh_eye)
    lv_obj_clear_flag(right_laugh_eye, LV_OBJ_FLAG_HIDDEN);
  if (mouth_cont)
    lv_obj_clear_flag(mouth_cont, LV_OBJ_FLAG_HIDDEN);
}

static void hide_laugh(void) {
  if (left_laugh_eye)
    lv_obj_add_flag(left_laugh_eye, LV_OBJ_FLAG_HIDDEN);
  if (right_laugh_eye)
    lv_obj_add_flag(right_laugh_eye, LV_OBJ_FLAG_HIDDEN);
  if (mouth_cont)
    lv_obj_add_flag(mouth_cont, LV_OBJ_FLAG_HIDDEN);
  if (left_eye)
    lv_obj_clear_flag(left_eye, LV_OBJ_FLAG_HIDDEN);
  if (right_eye)
    lv_obj_clear_flag(right_eye, LV_OBJ_FLAG_HIDDEN);
}

static void gesture_event_cb(lv_event_t *e) {
  lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
  if (dir == LV_DIR_TOP)
    ui_settings_show();
}

static void main_loop(lv_timer_t *timer) {
  update_positions();
  timer_ms += 50;
  switch (current_emotion) {
  case EMO_IDLE:
    if (timer_ms % 3000 == 0)
      do_blink();
    if (timer_ms >= 6000) {
      show_happy();
      current_emotion = EMO_HAPPY;
      timer_ms = 0;
    }
    break;
  case EMO_HAPPY:
    if (timer_ms % 2000 == 0)
      do_blink();
    if (timer_ms >= 3000) {
      hide_happy();
      show_sad();
      current_emotion = EMO_SAD;
      timer_ms = 0;
    }
    break;
  case EMO_SAD:
    if (timer_ms % 2500 == 0)
      do_blink();
    if (timer_ms >= 4000) {
      hide_sad();
      show_laugh();
      current_emotion = EMO_LAUGH;
      timer_ms = 0;
    }
    break;
  case EMO_LAUGH:
    if (timer_ms >= 3000) {
      hide_laugh();
      current_emotion = EMO_IDLE;
      timer_ms = 0;
    }
    break;
  }
}

// Draw Teardrop on Canvas to get perfect shape
static lv_obj_t *create_tear_image(lv_obj_t *parent) {
  if (canvas_tear_ref == NULL) {
    // Create a static canvas buffer
    static lv_color_t
        cbuf[LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA(TEAR_WIDTH, TEAR_HEIGHT)];
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
  lv_line_set_points(left_laugh_eye, left_arrow_pts, 3);
  lv_obj_add_style(left_laugh_eye, &style_line, 0);
  lv_obj_align(left_laugh_eye, LV_ALIGN_CENTER, -EYE_OFFSET_X, 0);
  lv_obj_add_flag(left_laugh_eye, LV_OBJ_FLAG_HIDDEN);

  right_laugh_eye = lv_line_create(scr_eyes);
  lv_line_set_points(right_laugh_eye, right_arrow_pts, 3);
  lv_obj_add_style(right_laugh_eye, &style_line, 0);
  lv_obj_align(right_laugh_eye, LV_ALIGN_CENTER, EYE_OFFSET_X, 0);
  lv_obj_add_flag(right_laugh_eye, LV_OBJ_FLAG_HIDDEN);

  mouth_cont = lv_obj_create(scr_eyes);
  lv_obj_set_size(mouth_cont, MOUTH_WIDTH, MOUTH_HEIGHT);
  lv_obj_set_style_bg_opa(mouth_cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(mouth_cont, 0, 0);
  lv_obj_align(mouth_cont, LV_ALIGN_CENTER, 0, MOUTH_OFFSET_Y);
  lv_obj_clear_flag(mouth_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_clip_corner(mouth_cont, true, 0);
  lv_obj_set_style_radius(mouth_cont, MOUTH_RADIUS, 0);
  lv_obj_add_flag(mouth_cont, LV_OBJ_FLAG_HIDDEN);

  mouth_circle = lv_obj_create(mouth_cont);
  lv_obj_set_size(mouth_circle, MOUTH_WIDTH, MOUTH_HEIGHT * 2);
  lv_obj_set_style_radius(mouth_circle, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(mouth_circle, EYE_COLOR, 0);
  lv_obj_set_style_bg_opa(mouth_circle, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(mouth_circle, 0, 0);
  lv_obj_align(mouth_circle, LV_ALIGN_TOP_MID, 0, -MOUTH_HEIGHT);

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

  lv_scr_load(scr_eyes);
  srand(12345);
  main_timer = lv_timer_create(main_loop, 50, NULL);
}

// --- API ---
void ui_robo_eyes_set_emotion_type(robot_emotion_t emotion) {}
void ui_robo_eyes_set_emotion(const char *emotion) {}
void ui_robo_eyes_look_at(int16_t x, int16_t y) {
  gaze_x = x;
  gaze_y = y;
}
void ui_robo_eyes_blink(void) { do_blink(); }
robot_emotion_t ui_robo_eyes_get_emotion(void) { return EMOTION_NORMAL; }
