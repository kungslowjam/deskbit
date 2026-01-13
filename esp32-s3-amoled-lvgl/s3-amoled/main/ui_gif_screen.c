#include "ui_gif_screen.h"

#include "lvgl.h"
#if LV_USE_GIF
#include "src/libs/gif/lv_gif.h"
#endif
#include "assets/baby_ugh_gif.h"

static lv_obj_t *s_gif;
static lv_obj_t *s_gif_mask;

void ui_gif_screen_create(lv_obj_t *parent) {
  if (parent == NULL) {
    return;
  }

  lv_obj_set_style_bg_color(parent, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

  s_gif_mask = lv_obj_create(parent);
  if (s_gif_mask == NULL) {
    return;
  }
  lv_obj_clear_flag(s_gif_mask, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(s_gif_mask, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(s_gif_mask, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(s_gif_mask, 0, LV_PART_MAIN);
  lv_obj_set_style_outline_width(s_gif_mask, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(s_gif_mask, 0, LV_PART_MAIN);

#if LV_USE_GIF
  // GIF support is enabled - create animated GIF
  s_gif = lv_gif_create(s_gif_mask);
  if (s_gif == NULL) {
    return;
  }
  lv_gif_set_src(s_gif, &baby_ugh_gif);
  lv_obj_set_size(s_gif_mask, baby_ugh_gif.header.w, baby_ugh_gif.header.h);
#else
  // GIF support not enabled - use static image as fallback
  s_gif = lv_image_create(s_gif_mask);
  if (s_gif == NULL) {
    return;
  }
  lv_image_set_src(s_gif, &baby_ugh_gif);
  lv_obj_set_size(s_gif_mask, baby_ugh_gif.header.w, baby_ugh_gif.header.h);
#endif

  lv_obj_set_style_radius(s_gif_mask, LV_RADIUS_CIRCLE, LV_PART_MAIN);
  lv_obj_set_style_clip_corner(s_gif_mask, true, LV_PART_MAIN);
  lv_obj_center(s_gif_mask);
  lv_obj_center(s_gif);
}

void ui_gif_screen_set_active(bool active) {
  if (s_gif == NULL) {
    return;
  }
#if LV_USE_GIF
  if (active) {
    lv_gif_resume(s_gif);
  } else {
    lv_gif_pause(s_gif);
  }
#else
  // GIF not enabled, nothing to pause/resume for static image
  (void)active;
#endif
}
