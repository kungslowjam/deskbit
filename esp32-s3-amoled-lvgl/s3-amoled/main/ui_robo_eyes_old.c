#include "ui_robo_eyes.h"

#include "assets/angry_png.h"
#include "assets/heart_png.h"
#include "assets/sleep_mode_png.h"

#include "esp_log.h"

#include <stdbool.h>

#define EYE_RADIUS_PCT 42
#define LAUGH_TEETH_COUNT 8
#define TALK_BAR_COUNT 7
#define DIZZY_POINT_COUNT 16

typedef struct {
  lv_obj_t *eye_l;
  lv_obj_t *eye_r;
  lv_obj_t *eye_cover_l;
  lv_obj_t *eye_cover_r;
  lv_obj_t *heart_l;
  lv_obj_t *heart_r;
  lv_obj_t *sleep_img;
  lv_obj_t *laugh_l;
  lv_obj_t *laugh_r;
  lv_obj_t *laugh_mouth;
  lv_obj_t *angry_mouth;
  lv_obj_t *angry_panel;
  lv_obj_t *tear;
  lv_obj_t *broken_img;
  lv_obj_t *talk_mouth;
  lv_obj_t *dizzy_l;
  lv_obj_t *dizzy_r;
  lv_timer_t *timer;
  lv_coord_t eye_w;
  lv_coord_t eye_h;
  lv_coord_t eye_gap;
  lv_coord_t eye_y;
  lv_coord_t base_w;
  lv_coord_t base_h;
  lv_coord_t base_gap;
  lv_coord_t base_y;
  lv_coord_t cover_w;
  lv_coord_t cover_h;
  lv_coord_t cover_y;
  lv_coord_t happy_eye_y_base;
  lv_coord_t happy_cover_y_base;
  lv_coord_t happy_y_offset;
  lv_coord_t heart_w;
  lv_coord_t heart_h;
  lv_coord_t heart_base_w;
  lv_coord_t heart_base_h;
  int32_t heart_scale_base;
  int32_t heart_scale_base_x;
  int32_t heart_scale_base_y;
  lv_coord_t heart_y_offset;
  lv_coord_t sleep_base_w;
  lv_coord_t sleep_base_h;
  lv_coord_t sleep_w;
  lv_coord_t sleep_h;
  lv_coord_t broken_base_w;
  lv_coord_t broken_base_h;
  lv_coord_t broken_w;
  lv_coord_t broken_h;
  lv_coord_t sleep_eye_h_base;
  lv_coord_t sleep_eye_h_min;
  lv_coord_t sleep_z_y_off[3];
  uint8_t sleep_z_opa[3];
  lv_coord_t tear_w;
  lv_coord_t tear_h;
  lv_coord_t tear_offset;
  lv_coord_t tear_base_w;
  lv_coord_t tear_base_h;
  lv_coord_t laugh_w;
  lv_coord_t laugh_h;
  lv_coord_t laugh_eye_y_offset;
  lv_coord_t angry_shake_offset;
  lv_coord_t laugh_mouth_y;
  lv_coord_t laugh_mouth_w_base;
  lv_coord_t laugh_mouth_h_base;
  lv_coord_t laugh_mouth_y_offset;
  lv_coord_t angry_mouth_w;
  lv_coord_t angry_mouth_h;
  lv_coord_t angry_mouth_y;
  lv_coord_t angry_panel_w;
  lv_coord_t angry_panel_h;
  lv_coord_t talk_mouth_w;
  lv_coord_t talk_mouth_h;
  lv_coord_t talk_mouth_y;
  lv_coord_t laugh_drop_w;
  lv_coord_t laugh_drop_h;
  lv_coord_t laugh_drop_y;
  lv_coord_t dizzy_size;
  lv_point_precise_t laugh_l_pts[3];
  lv_point_precise_t laugh_r_pts[3];
  lv_point_precise_t angry_mouth_pts[5];
  lv_point_precise_t dizzy_pts[DIZZY_POINT_COUNT];
  lv_obj_t *laugh_teeth[LAUGH_TEETH_COUNT];
  lv_obj_t *laugh_drop_l;
  lv_obj_t *laugh_drop_r;
  lv_obj_t *sleep_z[3];
  lv_obj_t *talk_bars[TALK_BAR_COUNT];
  lv_image_dsc_t tear_img;
  uint8_t *tear_buf;
  robo_eye_expr_t expr;
  uint8_t blink_count;
  uint8_t blink_state;
  uint8_t expr_ticks;
} robo_eyes_t;

typedef struct {
  lv_coord_t eye_w;
  lv_coord_t eye_h;
  lv_coord_t eye_y;
  lv_coord_t cover_w;
  lv_coord_t cover_h;
  lv_coord_t cover_y;
  lv_coord_t cover_radius;
  lv_coord_t gap_target;
  int16_t cover_angle_l;
  int16_t cover_angle_r;
  bool show_eyes;
  bool show_cover;
  bool show_love;
  bool show_sleep;
  bool show_broken;
  bool show_angry_mouth;
  bool show_talk;
  bool show_dizzy;
  bool show_tear;
  bool show_laugh;
  bool anim_happy;
  bool anim_love;
  bool anim_sleep;
  bool anim_angry;
  bool anim_talk;
  bool anim_dizzy;
  bool anim_tear;
  bool anim_laugh;
} robo_expr_profile_t;

static const char *kTag = "ui_robo_eyes";
static robo_eyes_t s_eyes;
static const lv_color_t kEyeColor = LV_COLOR_MAKE(0x28, 0xF3, 0xFF);
static const lv_color_t kAngryColor = LV_COLOR_MAKE(0xFF, 0x2E, 0x2E);
static const lv_color_t kAngryGradColor = LV_COLOR_MAKE(0xD0, 0x12, 0x12);
static const lv_color_t kTalkColor = LV_COLOR_MAKE(0x2C, 0xFF, 0x4A);
static const lv_color_t kBgColor = LV_COLOR_MAKE(0x00, 0x00, 0x00);

static lv_coord_t snap_even(lv_coord_t v) { return (v & 1) ? (v - 1) : v; }

static void robo_eyes_set_hidden(lv_obj_t *obj, bool hidden) {
  if (obj == NULL) {
    return;
  }
  if (hidden) {
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
  }
}

static void eye_set_size(lv_obj_t *eye, lv_coord_t w, lv_coord_t h) {
  w = snap_even(w);
  h = snap_even(h);
  lv_obj_set_size(eye, w, h);
  lv_obj_set_style_radius(eye, LV_RADIUS_CIRCLE, LV_PART_MAIN);
}

static void eye_apply_style(lv_obj_t *eye) {
  lv_obj_set_style_bg_color(eye, kEyeColor, LV_PART_MAIN);
  lv_obj_set_style_bg_grad_color(eye, lv_color_hex(0x1BD2F0), LV_PART_MAIN);
  lv_obj_set_style_bg_grad_dir(eye, LV_GRAD_DIR_VER, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(eye, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(eye, 0, LV_PART_MAIN);
  lv_obj_set_style_border_opa(eye, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_outline_width(eye, 0, LV_PART_MAIN);
  lv_obj_set_style_outline_opa(eye, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(eye, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_opa(eye, LV_OPA_TRANSP, LV_PART_MAIN);
}

static void eye_set_color(lv_obj_t *eye, lv_color_t main, lv_color_t grad) {
  lv_obj_set_style_bg_color(eye, main, LV_PART_MAIN);
  lv_obj_set_style_bg_grad_color(eye, grad, LV_PART_MAIN);
  lv_obj_set_style_bg_grad_dir(eye, LV_GRAD_DIR_VER, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(eye, LV_OPA_COVER, LV_PART_MAIN);
}

static void laugh_line_apply_style(lv_obj_t *line, lv_coord_t width) {
  lv_obj_set_style_line_color(line, kEyeColor, LV_PART_MAIN);
  lv_obj_set_style_line_width(line, width, LV_PART_MAIN);
  lv_obj_set_style_line_rounded(line, false, LV_PART_MAIN);
  lv_obj_set_style_line_opa(line, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(line, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(line, 0, LV_PART_MAIN);
  lv_obj_set_style_outline_width(line, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(line, 0, LV_PART_MAIN);
}

static void laugh_mouth_apply_style(lv_obj_t *mouth) {
  lv_obj_set_style_bg_color(mouth, lv_color_hex(0x0B0F1A), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(mouth, LV_OPA_80, LV_PART_MAIN);
  lv_obj_set_style_border_width(mouth, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(mouth, 10, LV_PART_MAIN);
}

static void angry_mouth_apply_style(lv_obj_t *mouth, lv_coord_t width) {
  lv_obj_set_style_line_color(mouth, kAngryColor, LV_PART_MAIN);
  lv_obj_set_style_line_width(mouth, width, LV_PART_MAIN);
  lv_obj_set_style_line_rounded(mouth, true, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(mouth, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(mouth, 0, LV_PART_MAIN);
  lv_obj_set_style_outline_width(mouth, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(mouth, 0, LV_PART_MAIN);
}

static void talk_mouth_apply_style(lv_obj_t *mouth) {
  lv_obj_set_style_bg_color(mouth, lv_color_hex(0x0B0F1A), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(mouth, LV_OPA_80, LV_PART_MAIN);
  lv_obj_set_style_border_width(mouth, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(mouth, LV_RADIUS_CIRCLE, LV_PART_MAIN);
}

static void dizzy_update_points(lv_coord_t size) {
  if (size <= 0) {
    return;
  }
  if (s_eyes.dizzy_size == size) {
    return;
  }
  s_eyes.dizzy_size = size;
  lv_coord_t cx = size / 2;
  lv_coord_t cy = size / 2;
  lv_coord_t r_start = size / 2 - 2;
  lv_coord_t r_end = size / 6;
  if (r_end < 4) {
    r_end = 4;
  }
  for (int i = 0; i < DIZZY_POINT_COUNT; i++) {
    int32_t angle = (int32_t)(i * 720 / (DIZZY_POINT_COUNT - 1));
    int32_t r = r_start - ((r_start - r_end) * i / (DIZZY_POINT_COUNT - 1));
    int32_t x = cx + (lv_trigo_cos((int16_t)angle) * r) / 32767;
    int32_t y = cy + (lv_trigo_sin((int16_t)angle) * r) / 32767;
    s_eyes.dizzy_pts[i].x = x;
    s_eyes.dizzy_pts[i].y = y;
  }
}

static void dizzy_apply_style(lv_obj_t *line, lv_coord_t size) {
  dizzy_update_points(size);
  lv_coord_t line_w = size / 8;
  if (line_w < 4) {
    line_w = 4;
  }
  lv_line_set_points(line, s_eyes.dizzy_pts, DIZZY_POINT_COUNT);
  lv_obj_set_size(line, size, size);
  lv_obj_set_style_line_width(line, line_w, LV_PART_MAIN);
  lv_obj_set_style_line_color(line, kEyeColor, LV_PART_MAIN);
  lv_obj_set_style_line_rounded(line, true, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(line, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(line, 0, LV_PART_MAIN);
  lv_obj_set_style_transform_pivot_x(line, size / 2, LV_PART_MAIN);
  lv_obj_set_style_transform_pivot_y(line, size / 2, LV_PART_MAIN);
  lv_obj_set_style_transform_rotation(line, 0, LV_PART_MAIN);
  lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
}

static void talk_mouth_layout_bars(lv_coord_t mouth_w, lv_coord_t mouth_h) {
  if (s_eyes.talk_mouth == NULL) {
    return;
  }
  lv_coord_t pad = snap_even((lv_coord_t)(mouth_w / 12));
  lv_coord_t gap = snap_even((lv_coord_t)(mouth_w / 28));
  if (gap < 2) {
    gap = 2;
  }
  lv_coord_t avail = mouth_w - (pad * 2) - (gap * (TALK_BAR_COUNT - 1));
  if (avail < TALK_BAR_COUNT) {
    avail = TALK_BAR_COUNT;
  }
  lv_coord_t bar_w = snap_even((lv_coord_t)(avail / TALK_BAR_COUNT));
  if (bar_w < 2) {
    bar_w = 2;
  }
  lv_coord_t x = pad;
  for (int i = 0; i < TALK_BAR_COUNT; i++) {
    lv_obj_t *bar = s_eyes.talk_bars[i];
    if (bar == NULL) {
      continue;
    }
    lv_obj_set_size(bar, bar_w, mouth_h / 2);
    lv_obj_set_style_bg_color(bar, kTalkColor, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, bar_w / 2, LV_PART_MAIN);
    lv_obj_set_style_border_width(bar, 0, LV_PART_MAIN);
    lv_obj_set_pos(bar, x, (mouth_h - (mouth_h / 2)) / 2);
    x += bar_w + gap;
  }
}

static void robo_eyes_realign(void);

static void laugh_anim_cb(void *obj, int32_t v) {
  LV_UNUSED(obj);
  s_eyes.laugh_eye_y_offset = (lv_coord_t)v;
  robo_eyes_realign();
}

static void laugh_anim_start(lv_obj_t *obj) {
  lv_anim_del(obj, laugh_anim_cb);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, obj);
  lv_anim_set_exec_cb(&a, laugh_anim_cb);
  lv_anim_set_values(&a, -4, 4);
  lv_anim_set_time(&a, 600);
  lv_anim_set_playback_time(&a, 600);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a);
}

static void laugh_anim_stop(lv_obj_t *obj) {
  lv_anim_del(obj, laugh_anim_cb);
  s_eyes.laugh_eye_y_offset = 0;
  lv_obj_set_style_line_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN);
  robo_eyes_realign();
}

static void love_anim_cb(void *obj, int32_t v) {
  LV_UNUSED(obj);
  if (v < 0) {
    v = 0;
  }
  if (v > 12) {
    v = 12;
  }
  s_eyes.heart_y_offset = (lv_coord_t)(-v);
  if (s_eyes.heart_l) {
    lv_obj_set_style_opa(s_eyes.heart_l, LV_OPA_COVER, LV_PART_MAIN);
  }
  if (s_eyes.heart_r) {
    lv_obj_set_style_opa(s_eyes.heart_r, LV_OPA_COVER, LV_PART_MAIN);
  }
  robo_eyes_realign();
}

static void love_fall_cb(void *obj, int32_t v) {
  LV_UNUSED(obj);
  s_eyes.heart_y_offset = (lv_coord_t)v;
  robo_eyes_realign();
}

static void happy_anim_cb(void *obj, int32_t v) {
  LV_UNUSED(obj);
  s_eyes.happy_y_offset = (lv_coord_t)v;
  s_eyes.eye_y = s_eyes.happy_eye_y_base;
  s_eyes.cover_y = s_eyes.happy_cover_y_base + s_eyes.happy_y_offset;
  robo_eyes_realign();
}

static void happy_anim_start(void) {
  lv_anim_del(NULL, happy_anim_cb);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, NULL);
  lv_anim_set_exec_cb(&a, happy_anim_cb);
  lv_anim_set_values(&a, 0, -16);
  lv_anim_set_time(&a, 700);
  lv_anim_set_playback_time(&a, 700);
  lv_anim_set_playback_delay(&a, 300);
  lv_anim_set_repeat_delay(&a, 900);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a);
}

static void happy_anim_stop(void) {
  lv_anim_del(NULL, happy_anim_cb);
  s_eyes.happy_y_offset = 0;
  s_eyes.eye_y = s_eyes.happy_eye_y_base;
  s_eyes.cover_y = s_eyes.happy_cover_y_base;
  robo_eyes_realign();
}

static void love_anim_start(void) {
  lv_anim_del(NULL, love_anim_cb);
  lv_anim_del(NULL, love_fall_cb);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, NULL);
  lv_anim_set_exec_cb(&a, love_anim_cb);
  lv_anim_set_values(&a, 0, 12);
  lv_anim_set_time(&a, 1400);
  lv_anim_set_playback_time(&a, 1400);
  lv_anim_set_playback_delay(&a, 0);
  lv_anim_set_repeat_delay(&a, 0);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a);
}

static void love_anim_stop(void) {
  lv_anim_del(NULL, love_anim_cb);
  lv_anim_del(NULL, love_fall_cb);
  s_eyes.heart_y_offset = 0;
  if (s_eyes.heart_scale_base_x > 0 && s_eyes.heart_scale_base_y > 0) {
    uint32_t scale_x = (uint32_t)s_eyes.heart_scale_base_x;
    uint32_t scale_y = (uint32_t)s_eyes.heart_scale_base_y;
    if (s_eyes.heart_l) {
      lv_image_set_scale_x(s_eyes.heart_l, scale_x);
      lv_image_set_scale_y(s_eyes.heart_l, scale_y);
      lv_obj_set_style_opa(s_eyes.heart_l, LV_OPA_COVER, LV_PART_MAIN);
    }
    if (s_eyes.heart_r) {
      lv_image_set_scale_x(s_eyes.heart_r, scale_x);
      lv_image_set_scale_y(s_eyes.heart_r, scale_y);
      lv_obj_set_style_opa(s_eyes.heart_r, LV_OPA_COVER, LV_PART_MAIN);
    }
  }
}

static void sleep_anim_cb(void *obj, int32_t v) {
  LV_UNUSED(obj);
  lv_coord_t h = (lv_coord_t)(s_eyes.sleep_eye_h_base + v);
  if (h < s_eyes.sleep_eye_h_min) {
    h = s_eyes.sleep_eye_h_min;
  }
  s_eyes.eye_h = h;
  eye_set_size(s_eyes.eye_l, s_eyes.eye_w, h);
  eye_set_size(s_eyes.eye_r, s_eyes.eye_w, h);
  robo_eyes_realign();
}

static void sleep_anim_start(void) {
  lv_anim_del(NULL, sleep_anim_cb);
  lv_coord_t delta = s_eyes.sleep_eye_h_base - s_eyes.sleep_eye_h_min;
  if (delta <= 0) {
    return;
  }
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, NULL);
  lv_anim_set_exec_cb(&a, sleep_anim_cb);
  lv_anim_set_values(&a, 0, -delta);
  lv_anim_set_time(&a, 2400);
  lv_anim_set_playback_time(&a, 2400);
  lv_anim_set_playback_delay(&a, 1200);
  lv_anim_set_repeat_delay(&a, 1800);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a);
}

static void sleep_anim_stop(void) {
  lv_anim_del(NULL, sleep_anim_cb);
  if (s_eyes.expr != ROBO_EYE_EXPR_SLEEP) {
    return;
  }
  if (s_eyes.sleep_eye_h_base <= 0) {
    return;
  }
  s_eyes.eye_h = s_eyes.sleep_eye_h_base;
  eye_set_size(s_eyes.eye_l, s_eyes.eye_w, s_eyes.eye_h);
  eye_set_size(s_eyes.eye_r, s_eyes.eye_w, s_eyes.eye_h);
  robo_eyes_realign();
}

static void sleep_z_anim_cb(void *var, int32_t v) {
  int idx = (int)(intptr_t)var;
  if (idx < 0 || idx > 2) {
    return;
  }
  if (v < 0) {
    v = 0;
  }
  if (v > 24) {
    v = 24;
  }
  s_eyes.sleep_z_y_off[idx] = (lv_coord_t)v;
  s_eyes.sleep_z_opa[idx] = (uint8_t)(255 - (v * 255) / 24);
  if (s_eyes.sleep_z[idx]) {
    lv_obj_set_style_opa(s_eyes.sleep_z[idx], s_eyes.sleep_z_opa[idx],
                         LV_PART_MAIN);
  }
  robo_eyes_realign();
}

static void sleep_z_anim_start(void) {
  for (int i = 0; i < 3; i++) {
    lv_anim_del((void *)(intptr_t)i, sleep_z_anim_cb);
    s_eyes.sleep_z_y_off[i] = 0;
    s_eyes.sleep_z_opa[i] = 255;
    if (s_eyes.sleep_z[i]) {
      lv_obj_set_style_opa(s_eyes.sleep_z[i], LV_OPA_COVER, LV_PART_MAIN);
    }
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, (void *)(intptr_t)i);
    lv_anim_set_exec_cb(&a, sleep_z_anim_cb);
    lv_anim_set_values(&a, 0, 48);
    lv_anim_set_time(&a, 2400);
    lv_anim_set_delay(&a, (uint32_t)(i * 700));
    lv_anim_set_repeat_delay(&a, 1200);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
  }
}

static void sleep_z_anim_stop(void) {
  for (int i = 0; i < 3; i++) {
    lv_anim_del((void *)(intptr_t)i, sleep_z_anim_cb);
    s_eyes.sleep_z_y_off[i] = 0;
    s_eyes.sleep_z_opa[i] = 0;
    if (s_eyes.sleep_z[i]) {
      lv_obj_set_style_opa(s_eyes.sleep_z[i], LV_OPA_TRANSP, LV_PART_MAIN);
    }
  }
}

static void angry_anim_cb(void *obj, int32_t v) {
  LV_UNUSED(obj);
  s_eyes.angry_shake_offset = (lv_coord_t)v;
  robo_eyes_realign();
}

static void angry_anim_start(void) {
  lv_anim_del(NULL, angry_anim_cb);
  s_eyes.angry_shake_offset = 0;
  robo_eyes_realign();
}

static void angry_anim_stop(void) {
  lv_anim_del(NULL, angry_anim_cb);
  s_eyes.angry_shake_offset = 0;
  robo_eyes_realign();
}

static void laugh_mouth_layout_teeth(lv_coord_t mouth_w, lv_coord_t mouth_h);

static void talk_anim_cb(void *obj, int32_t v) {
  LV_UNUSED(obj);
  static const uint8_t patterns[][TALK_BAR_COUNT] = {
      {20, 40, 70, 100, 70, 40, 20}, {30, 60, 90, 80, 90, 60, 30},
      {10, 30, 60, 90, 60, 30, 10},  {25, 50, 80, 100, 80, 50, 25},
      {15, 35, 65, 95, 65, 35, 15},
  };
  const int pattern_count = (int)(sizeof(patterns) / sizeof(patterns[0]));
  int idx = v;
  if (idx < 0) {
    idx = 0;
  }
  if (idx >= pattern_count) {
    idx = pattern_count - 1;
  }
  lv_coord_t h_base = s_eyes.talk_mouth_h;
  for (int i = 0; i < TALK_BAR_COUNT; i++) {
    lv_obj_t *bar = s_eyes.talk_bars[i];
    if (bar == NULL) {
      continue;
    }
    lv_coord_t bar_h = snap_even((lv_coord_t)(h_base * patterns[idx][i] / 100));
    if (bar_h < 4) {
      bar_h = 4;
    }
    lv_obj_set_height(bar, bar_h);
    lv_obj_set_y(bar, (h_base - bar_h) / 2);
  }
}

static void talk_anim_start(void) {
  lv_anim_del(NULL, talk_anim_cb);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, NULL);
  lv_anim_set_exec_cb(&a, talk_anim_cb);
  lv_anim_set_values(&a, 0, 4);
  lv_anim_set_time(&a, 600);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a);
}

static void dizzy_anim_cb(void *obj, int32_t v) {
  LV_UNUSED(obj);
  if (s_eyes.dizzy_l) {
    // lv_obj_set_style_transform_rotation(s_eyes.dizzy_l, v, LV_PART_MAIN);
  }
  if (s_eyes.dizzy_r) {
    // lv_obj_set_style_transform_rotation(s_eyes.dizzy_r, -v, LV_PART_MAIN);
  }
}

static void dizzy_anim_start(void) {
  lv_anim_del(NULL, dizzy_anim_cb);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, NULL);
  lv_anim_set_exec_cb(&a, dizzy_anim_cb);
  lv_anim_set_values(&a, 0, 3600);
  lv_anim_set_time(&a, 1400);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_path_cb(&a, lv_anim_path_linear);
  lv_anim_start(&a);
}

static void dizzy_anim_stop(void) {
  lv_anim_del(NULL, dizzy_anim_cb);
  if (s_eyes.dizzy_l) {
    lv_obj_set_style_transform_rotation(s_eyes.dizzy_l, 0, LV_PART_MAIN);
  }
  if (s_eyes.dizzy_r) {
    lv_obj_set_style_transform_rotation(s_eyes.dizzy_r, 0, LV_PART_MAIN);
  }
}

static void talk_anim_stop(void) { lv_anim_del(NULL, talk_anim_cb); }

static void laugh_mouth_bounce_cb(void *obj, int32_t v) {
  LV_UNUSED(obj);
  s_eyes.laugh_mouth_y_offset = (lv_coord_t)v;
  robo_eyes_realign();
}

static void laugh_mouth_width_cb(void *obj, int32_t v) {
  lv_obj_t *mouth = (lv_obj_t *)obj;
  lv_coord_t w = snap_even((lv_coord_t)(s_eyes.laugh_mouth_w_base + v));
  if (w < 10) {
    w = 10;
  }
  lv_coord_t h = s_eyes.laugh_mouth_h_base;
  lv_obj_set_size(mouth, w, h);
  laugh_mouth_layout_teeth(w, h);
  robo_eyes_realign();
}

static void laugh_mouth_anim_start(lv_obj_t *mouth) {
  lv_anim_del(mouth, laugh_mouth_bounce_cb);
  lv_anim_del(mouth, laugh_mouth_width_cb);

  lv_anim_t bounce;
  lv_anim_init(&bounce);
  lv_anim_set_var(&bounce, mouth);
  lv_anim_set_exec_cb(&bounce, laugh_mouth_bounce_cb);
  lv_anim_set_values(&bounce, -4, 4);
  lv_anim_set_time(&bounce, 420);
  lv_anim_set_playback_time(&bounce, 420);
  lv_anim_set_path_cb(&bounce, lv_anim_path_ease_in_out);
  lv_anim_set_repeat_count(&bounce, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&bounce);

  lv_anim_t width;
  lv_anim_init(&width);
  lv_anim_set_var(&width, mouth);
  lv_anim_set_exec_cb(&width, laugh_mouth_width_cb);
  lv_anim_set_values(&width, -8, 8);
  lv_anim_set_time(&width, 480);
  lv_anim_set_playback_time(&width, 480);
  lv_anim_set_path_cb(&width, lv_anim_path_ease_in_out);
  lv_anim_set_repeat_count(&width, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&width);
}

static void laugh_mouth_anim_stop(lv_obj_t *mouth) {
  lv_anim_del(mouth, laugh_mouth_bounce_cb);
  lv_anim_del(mouth, laugh_mouth_width_cb);
  s_eyes.laugh_mouth_y_offset = 0;
  if (s_eyes.laugh_mouth_w_base > 0 && s_eyes.laugh_mouth_h_base > 0) {
    lv_obj_set_size(mouth, s_eyes.laugh_mouth_w_base,
                    s_eyes.laugh_mouth_h_base);
    laugh_mouth_layout_teeth(s_eyes.laugh_mouth_w_base,
                             s_eyes.laugh_mouth_h_base);
  }
  robo_eyes_realign();
}

static void laugh_mouth_layout_teeth(lv_coord_t mouth_w, lv_coord_t mouth_h) {
  lv_coord_t gap = snap_even((lv_coord_t)(mouth_w / 28));
  lv_coord_t bar_w =
      (mouth_w - (gap * (LAUGH_TEETH_COUNT - 1))) / LAUGH_TEETH_COUNT;
  if (bar_w < 6) {
    bar_w = 6;
  }
  lv_coord_t bar_h = snap_even((lv_coord_t)(mouth_h * 3 / 4));
  for (int i = 0; i < LAUGH_TEETH_COUNT; i++) {
    lv_obj_t *bar = s_eyes.laugh_teeth[i];
    if (!bar) {
      continue;
    }
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(bar, bar_w, bar_h);
    lv_obj_set_style_bg_color(bar, kEyeColor, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 3, LV_PART_MAIN);
    lv_obj_set_style_border_width(bar, 0, LV_PART_MAIN);
    lv_obj_set_pos(bar, i * (bar_w + gap), (mouth_h - bar_h) / 2);
  }
}

static void tear_anim_cb(void *obj, int32_t v) {
  LV_UNUSED(obj);
  s_eyes.tear_offset = (lv_coord_t)v;
  lv_obj_set_style_opa((lv_obj_t *)obj, (lv_opa_t)(200 + (v * 3)),
                       LV_PART_MAIN);
  robo_eyes_realign();
}

static void tear_anim_start(lv_obj_t *tear) {
  lv_anim_del(tear, tear_anim_cb);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, tear);
  lv_anim_set_exec_cb(&a, tear_anim_cb);
  lv_anim_set_values(&a, 0, 8);
  lv_anim_set_time(&a, 800);
  lv_anim_set_playback_time(&a, 800);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a);
}

static void tear_anim_stop(lv_obj_t *tear) {
  lv_anim_del(tear, tear_anim_cb);
  s_eyes.tear_offset = 0;
  lv_obj_set_style_opa(tear, LV_OPA_COVER, LV_PART_MAIN);
  robo_eyes_realign();
}

static void tear_make_image(robo_eyes_t *eyes) {
  const lv_coord_t w = eyes->tear_base_w;
  const lv_coord_t h = eyes->tear_base_h;
  const size_t size = (size_t)w * (size_t)h * 4U;
  if (eyes->tear_buf == NULL) {
    eyes->tear_buf = lv_malloc(size);
  }
  if (eyes->tear_buf == NULL) {
    return;
  }

  lv_color32_t col = lv_color_to_32(kEyeColor, LV_OPA_COVER);
  for (lv_coord_t y = 0; y < h; y++) {
    for (lv_coord_t x = 0; x < w; x++) {
      float fx = ((float)x + 0.5f) / (float)w;
      float fy = ((float)y + 0.5f) / (float)h;
      float cx = 0.5f;
      float cy = 0.68f;
      float r = 0.33f;
      float dx = fx - cx;
      float dy = fy - cy;
      int in_circle = (dx * dx + dy * dy) <= (r * r);

      int in_tri = 0;
      const float top_y = 0.06f;
      const float base_y = 0.58f;
      if (fy >= top_y && fy <= base_y) {
        float t = (fy - top_y) / (base_y - top_y);
        float left = 0.5f - 0.28f * t;
        float right = 0.5f + 0.28f * t;
        in_tri = (fx >= left && fx <= right);
      }

      uint8_t *px = eyes->tear_buf + ((size_t)y * (size_t)w + (size_t)x) * 4U;
      if (in_circle || in_tri) {
        px[0] = col.blue;
        px[1] = col.green;
        px[2] = col.red;
        px[3] = col.alpha;
      } else {
        px[0] = 0;
        px[1] = 0;
        px[2] = 0;
        px[3] = 0;
      }
    }
  }

  eyes->tear_img.header.cf = LV_COLOR_FORMAT_ARGB8888;
  eyes->tear_img.header.magic = LV_IMAGE_HEADER_MAGIC;
  eyes->tear_img.header.w = (uint32_t)w;
  eyes->tear_img.header.h = (uint32_t)h;
  eyes->tear_img.data_size = (uint32_t)size;
  eyes->tear_img.data = eyes->tear_buf;
}

static void cover_config(lv_obj_t *cover, lv_coord_t w, lv_coord_t h,
                         lv_coord_t radius, int16_t angle_tenths) {
  lv_obj_set_size(cover, w, h);
  lv_obj_set_style_radius(cover, radius, LV_PART_MAIN);
  lv_obj_set_style_transform_pivot_x(cover, w / 2, LV_PART_MAIN);
  lv_obj_set_style_transform_pivot_y(cover, h / 2, LV_PART_MAIN);
  lv_obj_set_style_transform_angle(cover, angle_tenths, LV_PART_MAIN);
}

static robo_expr_profile_t robo_expr_profile_base(void) {
  robo_expr_profile_t p = {
      .eye_w = s_eyes.base_w,
      .eye_h = s_eyes.base_h,
      .eye_y = s_eyes.base_y,
      .cover_y = s_eyes.base_y,
      .gap_target = s_eyes.base_gap,
      .show_eyes = true,
  };
  return p;
}

static robo_expr_profile_t robo_expr_profile_normal(void) {
  return robo_expr_profile_base();
}

static robo_expr_profile_t robo_expr_profile_happy(void) {
  robo_expr_profile_t p = robo_expr_profile_base();
  p.eye_w = snap_even((lv_coord_t)((s_eyes.base_w * 11) / 10));
  p.eye_h = s_eyes.base_h;
  p.eye_y = snap_even((lv_coord_t)(s_eyes.base_y + 2));
  p.cover_w = snap_even((lv_coord_t)(p.eye_w + 10));
  p.cover_h = snap_even(p.eye_h);
  p.cover_y = snap_even((lv_coord_t)(p.eye_y + (p.eye_h / 2)));
  p.cover_radius = LV_RADIUS_CIRCLE;
  p.cover_angle_l = 0;
  p.cover_angle_r = 0;
  p.show_cover = true;
  p.anim_happy = true;
  lv_coord_t happy_gap = s_eyes.base_gap - 24;
  if (happy_gap < 0) {
    happy_gap = 0;
  }
  p.gap_target = s_eyes.base_gap == 0 ? 0 : happy_gap;
  return p;
}

static robo_expr_profile_t robo_expr_profile_sad(void) {
  robo_expr_profile_t p = robo_expr_profile_base();
  p.eye_y = snap_even((lv_coord_t)(s_eyes.base_y + 6));
  p.cover_w = snap_even((lv_coord_t)((p.eye_w * 14) / 10));
  p.cover_h = snap_even((lv_coord_t)((p.eye_h * 7) / 10));
  p.cover_y = snap_even((lv_coord_t)(p.eye_y - (p.eye_h / 2)));
  p.cover_radius = 0;
  p.cover_angle_l = -180;
  p.cover_angle_r = 180;
  p.show_cover = true;
  p.show_tear = true;
  p.anim_tear = true;
  return p;
}

static robo_expr_profile_t robo_expr_profile_laugh(void) {
  robo_expr_profile_t p = robo_expr_profile_base();
  p.show_eyes = false;
  p.show_laugh = true;
  p.anim_laugh = true;
  p.gap_target = s_eyes.base_gap;
  return p;
}

static robo_expr_profile_t robo_expr_profile_love(void) {
  robo_expr_profile_t p = robo_expr_profile_base();
  p.show_eyes = false;
  p.show_love = true;
  p.anim_love = true;
  return p;
}

static robo_expr_profile_t robo_expr_profile_sleep(void) {
  robo_expr_profile_t p = robo_expr_profile_base();
  p.eye_h = snap_even((lv_coord_t)(s_eyes.base_h / 4));
  if (p.eye_h < 6) {
    p.eye_h = 6;
  }
  p.eye_w = snap_even((lv_coord_t)((s_eyes.base_w * 9) / 10));
  p.eye_y = snap_even(
      (lv_coord_t)((s_eyes.base_y * 2 + s_eyes.base_h - p.eye_h) / 2));
  p.show_eyes = true;
  p.show_sleep = true;
  p.anim_sleep = true;
  return p;
}

static robo_expr_profile_t robo_expr_profile_angry(void) {
  robo_expr_profile_t p = robo_expr_profile_base();
  p.eye_w = snap_even((lv_coord_t)(s_eyes.base_w));
  p.eye_h = snap_even((lv_coord_t)((s_eyes.base_h * 2) / 5));
  if (p.eye_h < 8) {
    p.eye_h = 8;
  }
  p.eye_y = snap_even((lv_coord_t)(s_eyes.base_y + (s_eyes.base_h / 6)));
  p.cover_w = snap_even((lv_coord_t)(p.eye_w + (p.eye_w / 4)));
  p.cover_h = snap_even((lv_coord_t)((p.eye_h * 3) / 5));
  p.cover_y = snap_even((lv_coord_t)(p.eye_y - (p.eye_h / 3)));
  p.cover_radius = 0;
  p.cover_angle_l = -200;
  p.cover_angle_r = 200;
  p.show_eyes = true;
  p.show_cover = true;
  p.show_broken = false;
  p.show_angry_mouth = true;
  p.anim_angry = true;
  lv_coord_t angry_gap = s_eyes.base_gap - 16;
  if (angry_gap < 0) {
    angry_gap = 0;
  }
  p.gap_target = s_eyes.base_gap == 0 ? 0 : angry_gap;
  return p;
}

static robo_expr_profile_t robo_expr_profile_talk(void) {
  robo_expr_profile_t p = robo_expr_profile_base();
  p.show_eyes = true;
  p.show_talk = true;
  p.anim_talk = true;
  return p;
}

static robo_expr_profile_t robo_expr_profile_dizzy(void) {
  robo_expr_profile_t p = robo_expr_profile_base();
  p.show_eyes = false;
  p.show_dizzy = true;
  p.anim_dizzy = true;
  return p;
}

static robo_expr_profile_t (*const kExprProfiles[ROBO_EYE_EXPR_COUNT])(void) = {
    [ROBO_EYE_EXPR_NORMAL] = robo_expr_profile_normal,
    [ROBO_EYE_EXPR_HAPPY] = robo_expr_profile_happy,
    [ROBO_EYE_EXPR_SAD] = robo_expr_profile_sad,
    [ROBO_EYE_EXPR_LAUGH] = robo_expr_profile_laugh,
    [ROBO_EYE_EXPR_LOVE] = robo_expr_profile_love,
    [ROBO_EYE_EXPR_SLEEP] = robo_expr_profile_sleep,
    [ROBO_EYE_EXPR_ANGRY] = robo_expr_profile_angry,
    [ROBO_EYE_EXPR_TALK] = robo_expr_profile_talk,
    [ROBO_EYE_EXPR_DIZZY] = robo_expr_profile_dizzy,
};

static void robo_eyes_realign(void) {
  lv_coord_t x_off = snap_even(s_eyes.eye_gap / 2 + s_eyes.eye_w / 2);
  lv_coord_t y_off = snap_even(s_eyes.eye_y);
  lv_coord_t shake = s_eyes.angry_shake_offset;
  lv_obj_align(s_eyes.eye_l, LV_ALIGN_CENTER, -x_off + shake, y_off);
  lv_obj_align(s_eyes.eye_r, LV_ALIGN_CENTER, x_off + shake, y_off);
  if (s_eyes.heart_l != NULL && s_eyes.heart_r != NULL) {
    lv_obj_align(s_eyes.heart_l, LV_ALIGN_CENTER, -x_off,
                 y_off + s_eyes.heart_y_offset);
    lv_obj_align(s_eyes.heart_r, LV_ALIGN_CENTER, x_off,
                 y_off + s_eyes.heart_y_offset);
  }
  if (s_eyes.sleep_img != NULL && s_eyes.eye_r != NULL) {
    lv_coord_t off_x = -(s_eyes.sleep_w / 6);
    lv_coord_t off_y = -s_eyes.sleep_h / 6;
    lv_obj_align_to(s_eyes.sleep_img, s_eyes.eye_r, LV_ALIGN_OUT_TOP_RIGHT,
                    off_x, off_y);
  }
  if (s_eyes.broken_img != NULL && s_eyes.eye_r != NULL) {
    lv_coord_t off_x = -(s_eyes.broken_w / 10);
    lv_coord_t off_y = -(s_eyes.broken_h / 3);
    lv_obj_align_to(s_eyes.broken_img, s_eyes.eye_r, LV_ALIGN_OUT_TOP_RIGHT,
                    off_x, off_y);
  }
  for (int i = 0; i < 3; i++) {
    if (s_eyes.sleep_z[i] != NULL && s_eyes.eye_r != NULL) {
      lv_coord_t off_x = -(s_eyes.eye_w / 4) + (i * (s_eyes.eye_w / 8));
      lv_coord_t off_y = -(s_eyes.eye_h / 2) - (s_eyes.sleep_h / 5) - (i * 4) -
                         s_eyes.sleep_z_y_off[i];
      lv_obj_align_to(s_eyes.sleep_z[i], s_eyes.eye_r, LV_ALIGN_OUT_TOP_RIGHT,
                      off_x, off_y);
    }
  }
  if (s_eyes.eye_cover_l != NULL && s_eyes.eye_cover_r != NULL) {
    lv_obj_align(s_eyes.eye_cover_l, LV_ALIGN_CENTER, -x_off + shake,
                 s_eyes.cover_y);
    lv_obj_align(s_eyes.eye_cover_r, LV_ALIGN_CENTER, x_off + shake,
                 s_eyes.cover_y);
  }
  if (s_eyes.laugh_l != NULL && s_eyes.laugh_r != NULL) {
    lv_obj_align(s_eyes.laugh_l, LV_ALIGN_CENTER, -x_off,
                 y_off + s_eyes.laugh_eye_y_offset);
    lv_obj_align(s_eyes.laugh_r, LV_ALIGN_CENTER, x_off,
                 y_off + s_eyes.laugh_eye_y_offset);
  }
  if (s_eyes.laugh_mouth != NULL) {
    lv_obj_align(s_eyes.laugh_mouth, LV_ALIGN_CENTER, 0,
                 y_off + s_eyes.laugh_mouth_y + s_eyes.laugh_mouth_y_offset);
  }
  if (s_eyes.angry_mouth != NULL) {
    lv_obj_align(s_eyes.angry_mouth, LV_ALIGN_CENTER, shake,
                 y_off + s_eyes.angry_mouth_y);
  }
  if (s_eyes.talk_mouth != NULL) {
    lv_obj_align(s_eyes.talk_mouth, LV_ALIGN_CENTER, 0,
                 y_off + s_eyes.talk_mouth_y);
  }
  if (s_eyes.dizzy_l != NULL && s_eyes.dizzy_r != NULL) {
    lv_obj_align(s_eyes.dizzy_l, LV_ALIGN_CENTER, -x_off, y_off);
    lv_obj_align(s_eyes.dizzy_r, LV_ALIGN_CENTER, x_off, y_off);
  }
  if (s_eyes.laugh_drop_l != NULL && s_eyes.laugh_drop_r != NULL) {
    lv_obj_align(s_eyes.laugh_drop_l, LV_ALIGN_CENTER, -x_off,
                 y_off + s_eyes.laugh_drop_y);
    lv_obj_align(s_eyes.laugh_drop_r, LV_ALIGN_CENTER, x_off,
                 y_off + s_eyes.laugh_drop_y);
  }
  if (s_eyes.tear != NULL) {
    lv_obj_align_to(s_eyes.tear, s_eyes.eye_r, LV_ALIGN_OUT_BOTTOM_RIGHT,
                    -s_eyes.eye_w / 4,
                    -(s_eyes.eye_h / 12) + s_eyes.tear_offset);
  }
  lv_obj_t *parent = lv_obj_get_parent(s_eyes.eye_l);
  if (parent != NULL) {
    lv_obj_invalidate(parent);
  }
}

static void robo_eyes_apply(void) {
  eye_set_size(s_eyes.eye_l, s_eyes.eye_w, s_eyes.eye_h);
  eye_set_size(s_eyes.eye_r, s_eyes.eye_w, s_eyes.eye_h);
  robo_eyes_realign();
}

static void robo_eyes_anim_gap(void *var, int32_t v) {
  LV_UNUSED(var);
  s_eyes.eye_gap = (lv_coord_t)v;
  robo_eyes_realign();
}

static void robo_eyes_set_gap_target(lv_coord_t target_gap) {
  lv_anim_del(NULL, robo_eyes_anim_gap);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, NULL);
  lv_anim_set_exec_cb(&a, robo_eyes_anim_gap);
  lv_anim_set_values(&a, s_eyes.eye_gap, target_gap);
  lv_anim_set_time(&a, 260);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
  lv_anim_start(&a);
}

static void robo_eyes_apply_profile(const robo_expr_profile_t *profile) {
  s_eyes.eye_w = profile->eye_w;
  s_eyes.eye_h = profile->eye_h;
  s_eyes.eye_y = profile->eye_y;
  s_eyes.cover_y = profile->cover_y;
  s_eyes.cover_w = profile->cover_w;
  s_eyes.cover_h = profile->cover_h;
  s_eyes.happy_eye_y_base = s_eyes.eye_y;
  s_eyes.happy_cover_y_base = s_eyes.cover_y;

  robo_eyes_set_hidden(s_eyes.eye_l, !profile->show_eyes);
  robo_eyes_set_hidden(s_eyes.eye_r, !profile->show_eyes);
  if (profile->show_eyes) {
    if (profile->anim_angry) {
      eye_set_color(s_eyes.eye_l, kAngryColor, kAngryGradColor);
      eye_set_color(s_eyes.eye_r, kAngryColor, kAngryGradColor);
    } else {
      eye_set_color(s_eyes.eye_l, kEyeColor, lv_color_hex(0x1BD2F0));
      eye_set_color(s_eyes.eye_r, kEyeColor, lv_color_hex(0x1BD2F0));
    }
  }

  if (s_eyes.eye_cover_l && s_eyes.eye_cover_r) {
    if (profile->show_cover) {
      cover_config(s_eyes.eye_cover_l, profile->cover_w, profile->cover_h,
                   profile->cover_radius, profile->cover_angle_l);
      cover_config(s_eyes.eye_cover_r, profile->cover_w, profile->cover_h,
                   profile->cover_radius, profile->cover_angle_r);
      lv_obj_set_style_bg_opa(s_eyes.eye_cover_l, LV_OPA_COVER, LV_PART_MAIN);
      lv_obj_set_style_bg_opa(s_eyes.eye_cover_r, LV_OPA_COVER, LV_PART_MAIN);
      robo_eyes_set_hidden(s_eyes.eye_cover_l, false);
      robo_eyes_set_hidden(s_eyes.eye_cover_r, false);
    } else {
      robo_eyes_set_hidden(s_eyes.eye_cover_l, true);
      robo_eyes_set_hidden(s_eyes.eye_cover_r, true);
    }
  }

  if (s_eyes.tear) {
    if (profile->show_tear) {
      s_eyes.tear_w = snap_even((lv_coord_t)(s_eyes.eye_w / 2));
      s_eyes.tear_h = snap_even((lv_coord_t)((s_eyes.eye_h * 3) / 5));
      if (s_eyes.tear_base_w > 0 && s_eyes.tear_base_h > 0) {
        uint32_t sx =
            (uint32_t)s_eyes.tear_w * 256U / (uint32_t)s_eyes.tear_base_w;
        uint32_t sy =
            (uint32_t)s_eyes.tear_h * 256U / (uint32_t)s_eyes.tear_base_h;
        lv_image_set_scale_x(s_eyes.tear, sx);
        lv_image_set_scale_y(s_eyes.tear, sy);
      }
      robo_eyes_set_hidden(s_eyes.tear, false);
      if (profile->anim_tear) {
        tear_anim_start(s_eyes.tear);
      } else {
        tear_anim_stop(s_eyes.tear);
      }
    } else {
      tear_anim_stop(s_eyes.tear);
      robo_eyes_set_hidden(s_eyes.tear, true);
    }
  }

  if (profile->anim_happy) {
    happy_anim_start();
  } else {
    happy_anim_stop();
  }

  if (profile->anim_angry) {
    angry_anim_start();
  } else {
    angry_anim_stop();
  }

  if (profile->anim_talk) {
    talk_anim_start();
  } else {
    talk_anim_stop();
  }

  if (s_eyes.dizzy_l && s_eyes.dizzy_r) {
    if (profile->show_dizzy) {
      lv_coord_t size = snap_even((lv_coord_t)(s_eyes.eye_w));
      if (size < 16) {
        size = 16;
      }
      dizzy_apply_style(s_eyes.dizzy_l, size);
      dizzy_apply_style(s_eyes.dizzy_r, size);
      robo_eyes_set_hidden(s_eyes.dizzy_l, false);
      robo_eyes_set_hidden(s_eyes.dizzy_r, false);
      if (profile->anim_dizzy) {
        dizzy_anim_start();
      } else {
        dizzy_anim_stop();
      }
    } else {
      dizzy_anim_stop();
      robo_eyes_set_hidden(s_eyes.dizzy_l, true);
      robo_eyes_set_hidden(s_eyes.dizzy_r, true);
    }
  }

  if (profile->anim_sleep) {
    s_eyes.sleep_eye_h_base = s_eyes.eye_h;
    s_eyes.sleep_eye_h_min = snap_even((lv_coord_t)(s_eyes.eye_h / 2));
    if (s_eyes.sleep_eye_h_min < 4) {
      s_eyes.sleep_eye_h_min = 4;
    }
    sleep_anim_start();
    sleep_z_anim_start();
  } else {
    sleep_anim_stop();
    sleep_z_anim_stop();
  }

  if (s_eyes.sleep_img) {
    if (profile->show_sleep) {
      lv_coord_t target_w = s_eyes.eye_w + (s_eyes.eye_w / 2);
      lv_coord_t target_h =
          (lv_coord_t)((uint32_t)target_w * (uint32_t)s_eyes.sleep_base_h /
                       (uint32_t)s_eyes.sleep_base_w);
      if (target_h < (s_eyes.eye_h / 3)) {
        target_h = s_eyes.eye_h / 3;
      }
      s_eyes.sleep_w = snap_even(target_w);
      s_eyes.sleep_h = snap_even(target_h);
      uint32_t sx =
          (uint32_t)s_eyes.sleep_w * 256U / (uint32_t)s_eyes.sleep_base_w;
      uint32_t sy =
          (uint32_t)s_eyes.sleep_h * 256U / (uint32_t)s_eyes.sleep_base_h;
      lv_image_set_scale_x(s_eyes.sleep_img, sx);
      lv_image_set_scale_y(s_eyes.sleep_img, sy);
      lv_obj_set_size(s_eyes.sleep_img, s_eyes.sleep_w, s_eyes.sleep_h);
      robo_eyes_set_hidden(s_eyes.sleep_img, true);
      for (int i = 0; i < 3; i++) {
        if (s_eyes.sleep_z[i]) {
          robo_eyes_set_hidden(s_eyes.sleep_z[i], false);
        }
      }
    } else {
      robo_eyes_set_hidden(s_eyes.sleep_img, true);
      for (int i = 0; i < 3; i++) {
        if (s_eyes.sleep_z[i]) {
          robo_eyes_set_hidden(s_eyes.sleep_z[i], true);
        }
      }
    }
  }

  if (s_eyes.broken_img) {
    if (profile->show_broken) {
      lv_coord_t target_w = snap_even((lv_coord_t)((s_eyes.eye_w * 2) / 3));
      if (target_w < 32) {
        target_w = 32;
      }
      lv_coord_t target_h =
          (lv_coord_t)((uint32_t)target_w * (uint32_t)s_eyes.broken_base_h /
                       (uint32_t)s_eyes.broken_base_w);
      s_eyes.broken_w = snap_even(target_w);
      s_eyes.broken_h = snap_even(target_h);
      uint32_t sx =
          (uint32_t)s_eyes.broken_w * 256U / (uint32_t)s_eyes.broken_base_w;
      uint32_t sy =
          (uint32_t)s_eyes.broken_h * 256U / (uint32_t)s_eyes.broken_base_h;
      lv_image_set_scale_x(s_eyes.broken_img, sx);
      lv_image_set_scale_y(s_eyes.broken_img, sy);
      lv_obj_set_size(s_eyes.broken_img, s_eyes.broken_w, s_eyes.broken_h);
      robo_eyes_set_hidden(s_eyes.broken_img, false);
      lv_obj_move_foreground(s_eyes.broken_img);
    } else {
      robo_eyes_set_hidden(s_eyes.broken_img, true);
    }
  }

  if (s_eyes.heart_l && s_eyes.heart_r) {
    if (profile->show_love) {
      s_eyes.heart_w = snap_even((lv_coord_t)((s_eyes.eye_w * 9) / 10));
      s_eyes.heart_h = snap_even((lv_coord_t)((s_eyes.eye_h * 9) / 10));
      if (s_eyes.heart_w < 24) {
        s_eyes.heart_w = 24;
      }
      if (s_eyes.heart_h < 24) {
        s_eyes.heart_h = 24;
      }
      if (s_eyes.heart_base_w > 0 && s_eyes.heart_base_h > 0) {
        uint32_t sx =
            (uint32_t)s_eyes.heart_w * 256U / (uint32_t)s_eyes.heart_base_w;
        uint32_t sy =
            (uint32_t)s_eyes.heart_h * 256U / (uint32_t)s_eyes.heart_base_h;
        s_eyes.heart_scale_base = (int32_t)((sx + sy) / 2U);
        s_eyes.heart_scale_base_x = (int32_t)sx;
        s_eyes.heart_scale_base_y = (int32_t)sy;
        lv_image_set_scale_x(s_eyes.heart_l, sx);
        lv_image_set_scale_y(s_eyes.heart_l, sy);
        lv_image_set_scale_x(s_eyes.heart_r, sx);
        lv_image_set_scale_y(s_eyes.heart_r, sy);
      }
      robo_eyes_set_hidden(s_eyes.heart_l, false);
      robo_eyes_set_hidden(s_eyes.heart_r, false);
      if (profile->anim_love) {
        love_anim_start();
      } else {
        love_anim_stop();
      }
    } else {
      love_anim_stop();
      robo_eyes_set_hidden(s_eyes.heart_l, true);
      robo_eyes_set_hidden(s_eyes.heart_r, true);
    }
  }

  if (s_eyes.talk_mouth) {
    if (profile->show_talk) {
      lv_coord_t eye_span = (lv_coord_t)((s_eyes.eye_w * 2) + s_eyes.eye_gap);
      s_eyes.talk_mouth_w = snap_even((lv_coord_t)((eye_span * 9) / 10));
      s_eyes.talk_mouth_h = snap_even((lv_coord_t)(s_eyes.eye_h / 2));
      if (s_eyes.talk_mouth_h < 10) {
        s_eyes.talk_mouth_h = 10;
      }
      s_eyes.talk_mouth_y = snap_even((lv_coord_t)(s_eyes.eye_h / 3));
      lv_obj_set_size(s_eyes.talk_mouth, s_eyes.talk_mouth_w,
                      s_eyes.talk_mouth_h);
      talk_mouth_apply_style(s_eyes.talk_mouth);
      talk_mouth_layout_bars(s_eyes.talk_mouth_w, s_eyes.talk_mouth_h);
      robo_eyes_set_hidden(s_eyes.talk_mouth, false);
      for (int i = 0; i < TALK_BAR_COUNT; i++) {
        if (s_eyes.talk_bars[i]) {
          robo_eyes_set_hidden(s_eyes.talk_bars[i], false);
        }
      }
    } else {
      robo_eyes_set_hidden(s_eyes.talk_mouth, true);
      for (int i = 0; i < TALK_BAR_COUNT; i++) {
        if (s_eyes.talk_bars[i]) {
          robo_eyes_set_hidden(s_eyes.talk_bars[i], true);
        }
      }
    }
  }

  if (s_eyes.dizzy_l && s_eyes.dizzy_r) {
    if (!profile->show_dizzy) {
      robo_eyes_set_hidden(s_eyes.dizzy_l, true);
      robo_eyes_set_hidden(s_eyes.dizzy_r, true);
    }
  }

  if (s_eyes.angry_mouth) {
    if (profile->show_angry_mouth) {
      s_eyes.angry_mouth_w = snap_even((lv_coord_t)((s_eyes.eye_w * 11) / 10));
      s_eyes.angry_mouth_h = snap_even((lv_coord_t)(s_eyes.eye_h / 4));
      if (s_eyes.angry_mouth_w < 30) {
        s_eyes.angry_mouth_w = 30;
      }
      if (s_eyes.angry_mouth_h < 12) {
        s_eyes.angry_mouth_h = 12;
      }
      s_eyes.angry_mouth_y =
          snap_even((lv_coord_t)(s_eyes.eye_h + (s_eyes.eye_h / 4)));
      lv_coord_t mouth_edge = snap_even((lv_coord_t)(s_eyes.angry_mouth_h / 4));
      lv_coord_t mouth_mid =
          snap_even((lv_coord_t)((s_eyes.angry_mouth_h * 3) / 4));
      s_eyes.angry_mouth_pts[0].x = 0;
      s_eyes.angry_mouth_pts[0].y = mouth_edge;
      s_eyes.angry_mouth_pts[1].x = s_eyes.angry_mouth_w / 4;
      s_eyes.angry_mouth_pts[1].y = mouth_mid;
      s_eyes.angry_mouth_pts[2].x = s_eyes.angry_mouth_w / 2;
      s_eyes.angry_mouth_pts[2].y = s_eyes.angry_mouth_h;
      s_eyes.angry_mouth_pts[3].x = (s_eyes.angry_mouth_w * 3) / 4;
      s_eyes.angry_mouth_pts[3].y = mouth_mid;
      s_eyes.angry_mouth_pts[4].x = s_eyes.angry_mouth_w;
      s_eyes.angry_mouth_pts[4].y = mouth_edge;
      lv_line_set_points(s_eyes.angry_mouth, s_eyes.angry_mouth_pts, 5);
      lv_obj_set_size(s_eyes.angry_mouth, s_eyes.angry_mouth_w,
                      s_eyes.angry_mouth_h);
      lv_coord_t mouth_thick = snap_even((lv_coord_t)(s_eyes.eye_w / 7));
      if (mouth_thick < 4) {
        mouth_thick = 4;
      }
      angry_mouth_apply_style(s_eyes.angry_mouth, mouth_thick);
      robo_eyes_set_hidden(s_eyes.angry_mouth, false);
    } else {
      robo_eyes_set_hidden(s_eyes.angry_mouth, true);
    }
  }

  if (profile->show_laugh) {
    s_eyes.laugh_w = snap_even((lv_coord_t)((s_eyes.eye_w * 4) / 5));
    s_eyes.laugh_h = snap_even((lv_coord_t)(s_eyes.eye_h / 2));
    s_eyes.laugh_eye_y_offset = 0;
    if (s_eyes.laugh_w < 24) {
      s_eyes.laugh_w = 24;
    }
    if (s_eyes.laugh_h < 24) {
      s_eyes.laugh_h = 24;
    }
    lv_coord_t laugh_thick = snap_even((lv_coord_t)(s_eyes.eye_w / 5));
    if (laugh_thick < 6) {
      laugh_thick = 6;
    }
    s_eyes.laugh_mouth_y = snap_even((lv_coord_t)(s_eyes.eye_h / 2));
    s_eyes.laugh_drop_w = snap_even((lv_coord_t)(s_eyes.eye_w / 7));
    if (s_eyes.laugh_drop_w < 8) {
      s_eyes.laugh_drop_w = 8;
    }
    s_eyes.laugh_drop_h = snap_even(
        (lv_coord_t)(s_eyes.laugh_drop_w + (s_eyes.laugh_drop_w / 2)));
    s_eyes.laugh_drop_y = snap_even((lv_coord_t)(s_eyes.eye_h / 3));

    if (s_eyes.laugh_l && s_eyes.laugh_r) {
      s_eyes.laugh_l_pts[0].x = 0;
      s_eyes.laugh_l_pts[0].y = (lv_coord_t)(s_eyes.laugh_h / 8);
      s_eyes.laugh_l_pts[1].x = s_eyes.laugh_w;
      s_eyes.laugh_l_pts[1].y = s_eyes.laugh_h / 2;
      s_eyes.laugh_l_pts[2].x = 0;
      s_eyes.laugh_l_pts[2].y =
          (lv_coord_t)(s_eyes.laugh_h - (s_eyes.laugh_h / 8));
      lv_line_set_points(s_eyes.laugh_l, s_eyes.laugh_l_pts, 3);

      s_eyes.laugh_r_pts[0].x = s_eyes.laugh_w;
      s_eyes.laugh_r_pts[0].y = (lv_coord_t)(s_eyes.laugh_h / 8);
      s_eyes.laugh_r_pts[1].x = 0;
      s_eyes.laugh_r_pts[1].y = s_eyes.laugh_h / 2;
      s_eyes.laugh_r_pts[2].x = s_eyes.laugh_w;
      s_eyes.laugh_r_pts[2].y =
          (lv_coord_t)(s_eyes.laugh_h - (s_eyes.laugh_h / 8));
      lv_line_set_points(s_eyes.laugh_r, s_eyes.laugh_r_pts, 3);

      laugh_line_apply_style(s_eyes.laugh_l, laugh_thick);
      laugh_line_apply_style(s_eyes.laugh_r, laugh_thick);
    }

    if (s_eyes.laugh_mouth) {
      lv_coord_t mouth_w = snap_even((lv_coord_t)((s_eyes.eye_w * 11) / 10));
      lv_coord_t mouth_h = snap_even((lv_coord_t)(s_eyes.eye_h / 3));
      lv_obj_set_size(s_eyes.laugh_mouth, mouth_w, mouth_h);
      s_eyes.laugh_mouth_w_base = mouth_w;
      s_eyes.laugh_mouth_h_base = mouth_h;
      laugh_mouth_apply_style(s_eyes.laugh_mouth);
      laugh_mouth_layout_teeth(mouth_w, mouth_h);
    }
  }

  if (s_eyes.laugh_l && s_eyes.laugh_r) {
    if (profile->show_laugh) {
      robo_eyes_set_hidden(s_eyes.laugh_l, false);
      robo_eyes_set_hidden(s_eyes.laugh_r, false);
      if (profile->anim_laugh) {
        laugh_anim_start(s_eyes.laugh_l);
        laugh_anim_start(s_eyes.laugh_r);
      } else {
        laugh_anim_stop(s_eyes.laugh_l);
        laugh_anim_stop(s_eyes.laugh_r);
      }
    } else {
      laugh_anim_stop(s_eyes.laugh_l);
      laugh_anim_stop(s_eyes.laugh_r);
      robo_eyes_set_hidden(s_eyes.laugh_l, true);
      robo_eyes_set_hidden(s_eyes.laugh_r, true);
    }
  }

  if (s_eyes.laugh_drop_l && s_eyes.laugh_drop_r) {
    if (profile->show_laugh) {
      lv_obj_set_size(s_eyes.laugh_drop_l, s_eyes.laugh_drop_w,
                      s_eyes.laugh_drop_h);
      lv_obj_set_size(s_eyes.laugh_drop_r, s_eyes.laugh_drop_w,
                      s_eyes.laugh_drop_h);
      lv_obj_set_style_bg_color(s_eyes.laugh_drop_l, kEyeColor, LV_PART_MAIN);
      lv_obj_set_style_bg_color(s_eyes.laugh_drop_r, kEyeColor, LV_PART_MAIN);
      lv_obj_set_style_bg_opa(s_eyes.laugh_drop_l, LV_OPA_COVER, LV_PART_MAIN);
      lv_obj_set_style_bg_opa(s_eyes.laugh_drop_r, LV_OPA_COVER, LV_PART_MAIN);
      lv_obj_set_style_radius(s_eyes.laugh_drop_l, LV_RADIUS_CIRCLE,
                              LV_PART_MAIN);
      lv_obj_set_style_radius(s_eyes.laugh_drop_r, LV_RADIUS_CIRCLE,
                              LV_PART_MAIN);
      lv_obj_set_style_border_width(s_eyes.laugh_drop_l, 0, LV_PART_MAIN);
      lv_obj_set_style_border_width(s_eyes.laugh_drop_r, 0, LV_PART_MAIN);
      robo_eyes_set_hidden(s_eyes.laugh_drop_l, false);
      robo_eyes_set_hidden(s_eyes.laugh_drop_r, false);
    } else {
      robo_eyes_set_hidden(s_eyes.laugh_drop_l, true);
      robo_eyes_set_hidden(s_eyes.laugh_drop_r, true);
    }
  }

  if (s_eyes.laugh_mouth) {
    if (profile->show_laugh) {
      robo_eyes_set_hidden(s_eyes.laugh_mouth, false);
      if (profile->anim_laugh) {
        laugh_mouth_anim_start(s_eyes.laugh_mouth);
      } else {
        laugh_mouth_anim_stop(s_eyes.laugh_mouth);
      }
    } else {
      laugh_mouth_anim_stop(s_eyes.laugh_mouth);
      robo_eyes_set_hidden(s_eyes.laugh_mouth, true);
    }
  }

  robo_eyes_set_gap_target(profile->gap_target);
  robo_eyes_apply();
}

static void robo_eyes_apply_expr(void) {
  robo_expr_profile_t profile = robo_expr_profile_normal();
  if (s_eyes.expr < ROBO_EYE_EXPR_COUNT && kExprProfiles[s_eyes.expr]) {
    profile = kExprProfiles[s_eyes.expr]();
  }
  robo_eyes_apply_profile(&profile);
}

static void robo_eyes_set_expr_internal(robo_eye_expr_t expr,
                                        bool reset_ticks) {
  if (expr >= ROBO_EYE_EXPR_COUNT) {
    return;
  }
  s_eyes.expr = expr;
  if (reset_ticks) {
    s_eyes.expr_ticks = 0;
  }
  robo_eyes_apply_expr();
}

static void robo_eyes_blink_anim_cb(void *var, int32_t v) {
  LV_UNUSED(var);
  eye_set_size(s_eyes.eye_l, s_eyes.eye_w, (lv_coord_t)v);
  eye_set_size(s_eyes.eye_r, s_eyes.eye_w, (lv_coord_t)v);
  robo_eyes_realign();
}

static void robo_eyes_blink_ready_cb(lv_anim_t *anim) {
  LV_UNUSED(anim);
  s_eyes.blink_state = 0;
}

static void robo_eyes_timer_cb(lv_timer_t *timer) {
  LV_UNUSED(timer);
  if (s_eyes.blink_state != 0) {
    return;
  }

  if (s_eyes.expr != ROBO_EYE_EXPR_NORMAL) {
    s_eyes.expr_ticks++;
    if (s_eyes.expr_ticks >= 1) {
      robo_eyes_set_expr_internal(
          (robo_eye_expr_t)((s_eyes.expr + 1) % ROBO_EYE_EXPR_COUNT), true);
    }
    return;
  }

  s_eyes.blink_state = 1;
  s_eyes.blink_count++;
  if ((s_eyes.blink_count % 4) == 0) {
    robo_eyes_set_expr_internal(ROBO_EYE_EXPR_HAPPY, true);
  }

  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_exec_cb(&a, robo_eyes_blink_anim_cb);
  lv_anim_set_values(&a, s_eyes.eye_h, 6);
  lv_anim_set_time(&a, 160);
  lv_anim_set_playback_time(&a, 160);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
  lv_anim_set_ready_cb(&a, robo_eyes_blink_ready_cb);
  lv_anim_start(&a);
}

void ui_robo_eyes_create(lv_obj_t *parent) {
  if (parent == NULL) {
    return;
  }

  lv_obj_set_style_bg_color(parent, kBgColor, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_pad_all(parent, 0, LV_PART_MAIN);
  lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

  s_eyes.eye_w = 120;
  s_eyes.eye_h = 150;
  s_eyes.eye_gap = 70;
  s_eyes.eye_y = 0;
  s_eyes.base_w = s_eyes.eye_w;
  s_eyes.base_h = s_eyes.eye_h;
  s_eyes.base_gap = s_eyes.eye_gap;
  s_eyes.base_y = s_eyes.eye_y;
  s_eyes.cover_w = s_eyes.eye_w;
  s_eyes.cover_h = s_eyes.eye_h;
  s_eyes.cover_y = s_eyes.eye_y;
  s_eyes.heart_base_w = (lv_coord_t)heart_png.header.w;
  s_eyes.heart_base_h = (lv_coord_t)heart_png.header.h;
  s_eyes.heart_scale_base = 0;
  s_eyes.heart_scale_base_x = 0;
  s_eyes.heart_scale_base_y = 0;
  s_eyes.heart_y_offset = 0;
  s_eyes.sleep_base_w = (lv_coord_t)sleep_mode_png.header.w;
  s_eyes.sleep_base_h = (lv_coord_t)sleep_mode_png.header.h;
  s_eyes.sleep_w = 0;
  s_eyes.sleep_h = 0;
  s_eyes.sleep_eye_h_base = 0;
  s_eyes.sleep_eye_h_min = 0;
  s_eyes.broken_base_w = (lv_coord_t)angry_png.header.w;
  s_eyes.broken_base_h = (lv_coord_t)angry_png.header.h;
  s_eyes.broken_w = 0;
  s_eyes.broken_h = 0;
  for (int i = 0; i < 3; i++) {
    s_eyes.sleep_z_y_off[i] = 0;
    s_eyes.sleep_z_opa[i] = 0;
  }
  s_eyes.happy_eye_y_base = s_eyes.eye_y;
  s_eyes.happy_cover_y_base = s_eyes.cover_y;
  s_eyes.happy_y_offset = 0;
  s_eyes.angry_shake_offset = 0;
  s_eyes.angry_mouth_w = 0;
  s_eyes.angry_mouth_h = 0;
  s_eyes.angry_mouth_y = 0;
  s_eyes.talk_mouth_w = 0;
  s_eyes.talk_mouth_h = 0;
  s_eyes.talk_mouth_y = 0;
  s_eyes.tear_offset = 0;
  s_eyes.tear_base_w = 64;
  s_eyes.tear_base_h = 64;
  s_eyes.tear_buf = NULL;
  s_eyes.expr = ROBO_EYE_EXPR_NORMAL;
  s_eyes.blink_count = 0;
  s_eyes.expr_ticks = 0;

  s_eyes.eye_l = lv_obj_create(parent);
  s_eyes.eye_r = lv_obj_create(parent);
  if (s_eyes.eye_l == NULL || s_eyes.eye_r == NULL) {
    ESP_LOGE(kTag, "Failed to create eye objects");
    return;
  }

  eye_apply_style(s_eyes.eye_l);
  eye_apply_style(s_eyes.eye_r);
  lv_obj_clear_flag(s_eyes.eye_l, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(s_eyes.eye_r, LV_OBJ_FLAG_SCROLLABLE);

  s_eyes.eye_cover_l = lv_obj_create(parent);
  s_eyes.eye_cover_r = lv_obj_create(parent);
  if (s_eyes.eye_cover_l == NULL || s_eyes.eye_cover_r == NULL) {
    ESP_LOGE(kTag, "Failed to create eye cover objects");
    return;
  }
  lv_obj_clear_flag(s_eyes.eye_cover_l, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(s_eyes.eye_cover_r, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(s_eyes.eye_cover_l, kBgColor, LV_PART_MAIN);
  lv_obj_set_style_bg_color(s_eyes.eye_cover_r, kBgColor, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(s_eyes.eye_cover_l, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(s_eyes.eye_cover_r, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(s_eyes.eye_cover_l, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(s_eyes.eye_cover_r, 0, LV_PART_MAIN);
  lv_obj_add_flag(s_eyes.eye_cover_l, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_eyes.eye_cover_r, LV_OBJ_FLAG_HIDDEN);

  s_eyes.heart_l = lv_image_create(parent);
  s_eyes.heart_r = lv_image_create(parent);
  if (s_eyes.heart_l == NULL || s_eyes.heart_r == NULL) {
    ESP_LOGE(kTag, "Failed to create heart objects");
    return;
  }
  lv_image_set_src(s_eyes.heart_l, &heart_png);
  lv_image_set_src(s_eyes.heart_r, &heart_png);
  lv_image_set_pivot(s_eyes.heart_l, heart_png.header.w / 2,
                     heart_png.header.h / 2);
  lv_image_set_pivot(s_eyes.heart_r, heart_png.header.w / 2,
                     heart_png.header.h / 2);
  lv_image_set_inner_align(s_eyes.heart_l, LV_IMAGE_ALIGN_CENTER);
  lv_image_set_inner_align(s_eyes.heart_r, LV_IMAGE_ALIGN_CENTER);
  lv_image_set_scale_x(s_eyes.heart_l, LV_SCALE_NONE);
  lv_image_set_scale_y(s_eyes.heart_l, LV_SCALE_NONE);
  lv_image_set_scale_x(s_eyes.heart_r, LV_SCALE_NONE);
  lv_image_set_scale_y(s_eyes.heart_r, LV_SCALE_NONE);
  lv_obj_set_style_opa(s_eyes.heart_l, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_opa(s_eyes.heart_r, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(s_eyes.heart_l, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(s_eyes.heart_r, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(s_eyes.heart_l, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(s_eyes.heart_r, 0, LV_PART_MAIN);
  lv_obj_set_style_outline_width(s_eyes.heart_l, 0, LV_PART_MAIN);
  lv_obj_set_style_outline_width(s_eyes.heart_r, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(s_eyes.heart_l, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(s_eyes.heart_r, 0, LV_PART_MAIN);
  lv_obj_clear_flag(s_eyes.heart_l, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(s_eyes.heart_r, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(s_eyes.heart_l, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_eyes.heart_r, LV_OBJ_FLAG_HIDDEN);

  s_eyes.sleep_img = lv_image_create(parent);
  if (s_eyes.sleep_img == NULL) {
    ESP_LOGE(kTag, "Failed to create sleep image");
    return;
  }
  lv_image_set_src(s_eyes.sleep_img, &sleep_mode_png);
  lv_image_set_scale_x(s_eyes.sleep_img, LV_SCALE_NONE);
  lv_image_set_scale_y(s_eyes.sleep_img, LV_SCALE_NONE);
  lv_obj_set_style_opa(s_eyes.sleep_img, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_image_recolor(s_eyes.sleep_img, kEyeColor, LV_PART_MAIN);
  lv_obj_set_style_image_recolor_opa(s_eyes.sleep_img, LV_OPA_COVER,
                                     LV_PART_MAIN);
  lv_obj_clear_flag(s_eyes.sleep_img, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(s_eyes.sleep_img, LV_OBJ_FLAG_HIDDEN);

  s_eyes.broken_img = lv_image_create(parent);
  if (s_eyes.broken_img == NULL) {
    ESP_LOGE(kTag, "Failed to create broken image");
    return;
  }
  lv_image_set_src(s_eyes.broken_img, &angry_png);
  lv_image_set_scale_x(s_eyes.broken_img, LV_SCALE_NONE);
  lv_image_set_scale_y(s_eyes.broken_img, LV_SCALE_NONE);
  lv_obj_set_style_opa(s_eyes.broken_img, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(s_eyes.broken_img, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(s_eyes.broken_img, 0, LV_PART_MAIN);
  lv_obj_set_style_outline_width(s_eyes.broken_img, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(s_eyes.broken_img, 0, LV_PART_MAIN);
  lv_obj_set_style_image_recolor(s_eyes.broken_img, kAngryColor, LV_PART_MAIN);
  lv_obj_set_style_image_recolor_opa(s_eyes.broken_img, LV_OPA_COVER,
                                     LV_PART_MAIN);
  lv_obj_clear_flag(s_eyes.broken_img, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(s_eyes.broken_img, LV_OBJ_FLAG_HIDDEN);

  for (int i = 0; i < 3; i++) {
    s_eyes.sleep_z[i] = lv_label_create(parent);
    if (s_eyes.sleep_z[i] == NULL) {
      ESP_LOGE(kTag, "Failed to create sleep z label");
      return;
    }
    lv_label_set_text(s_eyes.sleep_z[i], "Z");
    lv_obj_set_style_text_color(s_eyes.sleep_z[i], kEyeColor, LV_PART_MAIN);
    lv_obj_set_style_text_opa(s_eyes.sleep_z[i], LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_transform_scale_x(s_eyes.sleep_z[i], 768 + (i * 160),
                                       LV_PART_MAIN);
    lv_obj_set_style_transform_scale_y(s_eyes.sleep_z[i], 768 + (i * 160),
                                       LV_PART_MAIN);
    lv_obj_set_style_transform_pivot_x(s_eyes.sleep_z[i], 0, LV_PART_MAIN);
    lv_obj_set_style_transform_pivot_y(s_eyes.sleep_z[i], 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_eyes.sleep_z[i], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_eyes.sleep_z[i], LV_OBJ_FLAG_HIDDEN);
  }

  s_eyes.tear = lv_image_create(parent);
  if (s_eyes.tear == NULL) {
    ESP_LOGE(kTag, "Failed to create tear object");
    return;
  }
  tear_make_image(&s_eyes);
  if (s_eyes.tear_buf != NULL) {
    lv_image_set_src(s_eyes.tear, &s_eyes.tear_img);
    lv_image_set_scale_x(s_eyes.tear, LV_SCALE_NONE);
    lv_image_set_scale_y(s_eyes.tear, LV_SCALE_NONE);
  }
  lv_obj_set_style_opa(s_eyes.tear, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_clear_flag(s_eyes.tear, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(s_eyes.tear, LV_OBJ_FLAG_HIDDEN);

  s_eyes.laugh_l = lv_line_create(parent);
  s_eyes.laugh_r = lv_line_create(parent);
  if (s_eyes.laugh_l == NULL || s_eyes.laugh_r == NULL) {
    ESP_LOGE(kTag, "Failed to create laugh eye objects");
    return;
  }
  laugh_line_apply_style(s_eyes.laugh_l, 10);
  laugh_line_apply_style(s_eyes.laugh_r, 10);
  lv_obj_add_flag(s_eyes.laugh_l, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_eyes.laugh_r, LV_OBJ_FLAG_HIDDEN);

  s_eyes.laugh_mouth = lv_obj_create(parent);
  if (s_eyes.laugh_mouth == NULL) {
    ESP_LOGE(kTag, "Failed to create laugh mouth object");
    return;
  }
  lv_obj_clear_flag(s_eyes.laugh_mouth, LV_OBJ_FLAG_SCROLLABLE);
  laugh_mouth_apply_style(s_eyes.laugh_mouth);
  for (int i = 0; i < LAUGH_TEETH_COUNT; i++) {
    s_eyes.laugh_teeth[i] = lv_obj_create(s_eyes.laugh_mouth);
    if (s_eyes.laugh_teeth[i]) {
      lv_obj_clear_flag(s_eyes.laugh_teeth[i], LV_OBJ_FLAG_SCROLLABLE);
    }
  }
  lv_obj_add_flag(s_eyes.laugh_mouth, LV_OBJ_FLAG_HIDDEN);

  s_eyes.angry_mouth = lv_line_create(parent);
  if (s_eyes.angry_mouth == NULL) {
    ESP_LOGE(kTag, "Failed to create angry mouth object");
    return;
  }
  angry_mouth_apply_style(s_eyes.angry_mouth, 6);
  lv_obj_add_flag(s_eyes.angry_mouth, LV_OBJ_FLAG_HIDDEN);

  s_eyes.talk_mouth = lv_obj_create(parent);
  if (s_eyes.talk_mouth == NULL) {
    ESP_LOGE(kTag, "Failed to create talk mouth object");
    return;
  }
  lv_obj_clear_flag(s_eyes.talk_mouth, LV_OBJ_FLAG_SCROLLABLE);
  talk_mouth_apply_style(s_eyes.talk_mouth);
  for (int i = 0; i < TALK_BAR_COUNT; i++) {
    s_eyes.talk_bars[i] = lv_obj_create(s_eyes.talk_mouth);
    if (s_eyes.talk_bars[i]) {
      lv_obj_clear_flag(s_eyes.talk_bars[i], LV_OBJ_FLAG_SCROLLABLE);
    }
  }
  lv_obj_add_flag(s_eyes.talk_mouth, LV_OBJ_FLAG_HIDDEN);

  s_eyes.dizzy_l = lv_line_create(parent);
  s_eyes.dizzy_r = lv_line_create(parent);
  if (s_eyes.dizzy_l == NULL || s_eyes.dizzy_r == NULL) {
    ESP_LOGE(kTag, "Failed to create dizzy objects");
    return;
  }
  lv_obj_clear_flag(s_eyes.dizzy_l, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(s_eyes.dizzy_r, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(s_eyes.dizzy_l, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_eyes.dizzy_r, LV_OBJ_FLAG_HIDDEN);

  s_eyes.laugh_drop_l = lv_obj_create(parent);
  s_eyes.laugh_drop_r = lv_obj_create(parent);
  if (s_eyes.laugh_drop_l == NULL || s_eyes.laugh_drop_r == NULL) {
    ESP_LOGE(kTag, "Failed to create laugh drop objects");
    return;
  }
  lv_obj_clear_flag(s_eyes.laugh_drop_l, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(s_eyes.laugh_drop_r, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(s_eyes.laugh_drop_l, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_eyes.laugh_drop_r, LV_OBJ_FLAG_HIDDEN);

  robo_eyes_apply();

  if (s_eyes.timer == NULL) {
    s_eyes.timer = lv_timer_create(robo_eyes_timer_cb, 2200, NULL);
  }
}

void ui_robo_eyes_set_expr(robo_eye_expr_t expr) {
  robo_eyes_set_expr_internal(expr, true);
}

void ui_robo_eyes_set_active(bool active) {
  if (active) {
    if (s_eyes.timer) {
      lv_timer_resume(s_eyes.timer);
    }
    robo_eyes_apply_expr();
    return;
  }

  if (s_eyes.timer) {
    lv_timer_pause(s_eyes.timer);
  }
  lv_anim_del(NULL, robo_eyes_blink_anim_cb);
  s_eyes.blink_state = 0;

  if (s_eyes.laugh_l) {
    laugh_anim_stop(s_eyes.laugh_l);
  }
  if (s_eyes.laugh_r) {
    laugh_anim_stop(s_eyes.laugh_r);
  }
  if (s_eyes.laugh_mouth) {
    laugh_mouth_anim_stop(s_eyes.laugh_mouth);
  }
  if (s_eyes.angry_mouth) {
    robo_eyes_set_hidden(s_eyes.angry_mouth, true);
  }
  if (s_eyes.tear) {
    tear_anim_stop(s_eyes.tear);
  }
  if (s_eyes.sleep_img) {
    robo_eyes_set_hidden(s_eyes.sleep_img, true);
  }
  if (s_eyes.broken_img) {
    robo_eyes_set_hidden(s_eyes.broken_img, true);
  }
  if (s_eyes.talk_mouth) {
    robo_eyes_set_hidden(s_eyes.talk_mouth, true);
  }
  if (s_eyes.dizzy_l) {
    robo_eyes_set_hidden(s_eyes.dizzy_l, true);
  }
  if (s_eyes.dizzy_r) {
    robo_eyes_set_hidden(s_eyes.dizzy_r, true);
  }
  for (int i = 0; i < TALK_BAR_COUNT; i++) {
    if (s_eyes.talk_bars[i]) {
      robo_eyes_set_hidden(s_eyes.talk_bars[i], true);
    }
  }
  happy_anim_stop();
  love_anim_stop();
  sleep_anim_stop();
  sleep_z_anim_stop();
  angry_anim_stop();
  talk_anim_stop();
  dizzy_anim_stop();
}
