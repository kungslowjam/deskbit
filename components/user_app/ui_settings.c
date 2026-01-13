/**
 * @file ui_settings.c
 * @brief Modern Settings Screen with Brightness Control
 *
 * Features:
 * - Glassmorphism design
 * - Animated brightness slider
 * - Sun icon that changes with brightness
 * - Smooth transitions
 */

#include "ui_settings.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// Screen dimensions
#define SCREEN_W 466
#define SCREEN_H 466
#define SCREEN_CENTER_X (SCREEN_W / 2)
#define SCREEN_CENTER_Y (SCREEN_H / 2)

// Colors - Modern dark theme
#define COLOR_BG lv_color_hex(0x0D1117)
#define COLOR_CARD_BG lv_color_hex(0x161B22)
#define COLOR_ACCENT lv_color_hex(0x00E5FF)
#define COLOR_ACCENT_DIM lv_color_hex(0x006680)
#define COLOR_TEXT lv_color_hex(0xE6EDF3)
#define COLOR_TEXT_DIM lv_color_hex(0x8B949E)
#define COLOR_SLIDER_BG lv_color_hex(0x21262D)
#define COLOR_SUN lv_color_hex(0xFFD93D)

// Objects
static lv_obj_t *settings_scr = NULL;
static lv_obj_t *bg_panel = NULL;
static lv_obj_t *title_label = NULL;
static lv_obj_t *brightness_card = NULL;
static lv_obj_t *sun_icon = NULL;
static lv_obj_t *brightness_slider = NULL;
static lv_obj_t *brightness_value_label = NULL;
// static lv_obj_t *back_btn = NULL; // Removed: using gesture instead
static lv_obj_t *prev_screen = NULL;

static void (*brightness_callback)(uint8_t) = NULL;
static bool is_visible = false;

// ============================================================================
// Custom Slider Style
// ============================================================================

static lv_style_t style_slider_main;
static lv_style_t style_slider_indicator;
static lv_style_t style_slider_knob;

static void init_slider_styles(void) {
  // Main track style
  lv_style_init(&style_slider_main);
  lv_style_set_bg_color(&style_slider_main, COLOR_SLIDER_BG);
  lv_style_set_bg_opa(&style_slider_main, LV_OPA_COVER);
  lv_style_set_radius(&style_slider_main, 10);
  lv_style_set_height(&style_slider_main, 20);

  // Indicator (filled part) style
  lv_style_init(&style_slider_indicator);
  lv_style_set_bg_color(&style_slider_indicator, COLOR_ACCENT);
  lv_style_set_bg_opa(&style_slider_indicator, LV_OPA_COVER);
  lv_style_set_radius(&style_slider_indicator, 10);
  lv_style_set_bg_grad_color(&style_slider_indicator, COLOR_SUN);
  lv_style_set_bg_grad_dir(&style_slider_indicator, LV_GRAD_DIR_HOR);

  // Knob style
  lv_style_init(&style_slider_knob);
  lv_style_set_bg_color(&style_slider_knob, lv_color_white());
  lv_style_set_bg_opa(&style_slider_knob, LV_OPA_COVER);
  lv_style_set_radius(&style_slider_knob, LV_RADIUS_CIRCLE);
  lv_style_set_pad_all(&style_slider_knob, 8);
  lv_style_set_shadow_width(&style_slider_knob, 15);
  lv_style_set_shadow_color(&style_slider_knob, COLOR_ACCENT);
  lv_style_set_shadow_opa(&style_slider_knob, LV_OPA_50);
}

// ============================================================================
// Animation Helpers
// ============================================================================

static void anim_opa_cb(void *var, int32_t v) {
  lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, 0);
}

static void animate_fade_in(lv_obj_t *obj, uint32_t delay) {
  lv_obj_set_style_opa(obj, LV_OPA_TRANSP, 0);

  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, obj);
  lv_anim_set_values(&a, 0, 255);
  lv_anim_set_time(&a, 300);
  lv_anim_set_delay(&a, delay);
  lv_anim_set_exec_cb(&a, anim_opa_cb);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
  lv_anim_start(&a);
}

// ============================================================================
// Event Handlers
// ============================================================================

static void slider_event_cb(lv_event_t *e) {
  lv_obj_t *slider = lv_event_get_target(e);
  int32_t value = lv_slider_get_value(slider);

  // Update label
  char buf[16];
  lv_snprintf(buf, sizeof(buf), "%d%%", (value * 100) / 255);
  lv_label_set_text(brightness_value_label, buf);

  // Update sun icon size based on brightness
  int32_t sun_size = 30 + (value * 30) / 255; // 30 to 60
  lv_obj_set_size(sun_icon, sun_size, sun_size);

  // Update sun color (dimmer = more orange, brighter = more yellow)
  lv_color_t sun_color = lv_color_mix(COLOR_SUN, lv_color_hex(0xFF6B35),
                                      255 - (value * 255) / 255);
  lv_obj_set_style_bg_color(sun_icon, sun_color, 0);

  // Update shadow intensity
  lv_obj_set_style_shadow_opa(sun_icon, (value * LV_OPA_80) / 255, 0);

  // Call brightness callback
  if (brightness_callback) {
    brightness_callback((uint8_t)value);
  }
}

// static void back_btn_event_cb(lv_event_t *e) { ui_settings_hide(); } //
// Removed

static void settings_gesture_cb(lv_event_t *e) {
  lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
  if (dir == LV_DIR_BOTTOM) { // Swipe down
    ui_settings_hide();
  }
}

// ============================================================================
// UI Creation
// ============================================================================

static lv_obj_t *create_sun_icon(lv_obj_t *parent) {
  lv_obj_t *sun = lv_obj_create(parent);
  lv_obj_set_size(sun, 50, 50);
  lv_obj_set_style_radius(sun, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(sun, COLOR_SUN, 0);
  lv_obj_set_style_bg_opa(sun, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(sun, 0, 0);
  lv_obj_set_style_shadow_width(sun, 30, 0);
  lv_obj_set_style_shadow_color(sun, COLOR_SUN, 0);
  lv_obj_set_style_shadow_opa(sun, LV_OPA_60, 0);
  lv_obj_set_style_shadow_spread(sun, 10, 0);
  lv_obj_clear_flag(sun, LV_OBJ_FLAG_SCROLLABLE);

  return sun;
}

static lv_obj_t *create_card(lv_obj_t *parent, lv_coord_t w, lv_coord_t h) {
  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_set_size(card, w, h);
  lv_obj_set_style_bg_color(card, COLOR_CARD_BG, 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_90, 0);
  lv_obj_set_style_radius(card, 25, 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0x30363D), 0);
  lv_obj_set_style_border_opa(card, LV_OPA_50, 0);
  lv_obj_set_style_shadow_width(card, 20, 0);
  lv_obj_set_style_shadow_color(card, lv_color_black(), 0);
  lv_obj_set_style_shadow_opa(card, LV_OPA_30, 0);
  lv_obj_set_style_pad_all(card, 20, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  return card;
}

// create_back_button removed

void ui_settings_init(void) {
  if (settings_scr)
    return;

  init_slider_styles();

  // Create screen
  settings_scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(settings_scr, COLOR_BG, 0);
  lv_obj_set_style_bg_opa(settings_scr, LV_OPA_COVER, 0);
  lv_obj_clear_flag(settings_scr, LV_OBJ_FLAG_SCROLLABLE);

  // Title
  title_label = lv_label_create(settings_scr);
  lv_label_set_text(title_label, "Settings");
  lv_obj_set_style_text_color(title_label, COLOR_TEXT, 0);
  lv_obj_set_style_text_font(title_label, LV_FONT_DEFAULT, 0);
  lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 50);

  // Brightness Card
  brightness_card = create_card(settings_scr, 340, 200);
  lv_obj_align(brightness_card, LV_ALIGN_CENTER, 0, 0);

  // Card title
  lv_obj_t *card_title = lv_label_create(brightness_card);
  lv_label_set_text(card_title, "Brightness");
  lv_obj_set_style_text_color(card_title, COLOR_TEXT, 0);
  lv_obj_set_style_text_font(card_title, LV_FONT_DEFAULT, 0);
  lv_obj_align(card_title, LV_ALIGN_TOP_LEFT, 0, 0);

  // Sun icon
  sun_icon = create_sun_icon(brightness_card);
  lv_obj_align(sun_icon, LV_ALIGN_TOP_MID, 0, 35);

  // Brightness value label
  brightness_value_label = lv_label_create(brightness_card);
  lv_label_set_text(brightness_value_label, "100%");
  lv_obj_set_style_text_color(brightness_value_label, COLOR_ACCENT, 0);
  lv_obj_set_style_text_font(brightness_value_label, LV_FONT_DEFAULT, 0);
  lv_obj_align(brightness_value_label, LV_ALIGN_TOP_RIGHT, 0, 0);

  // Slider
  brightness_slider = lv_slider_create(brightness_card);
  lv_obj_set_width(brightness_slider, 280);
  lv_obj_align(brightness_slider, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_slider_set_range(brightness_slider, 10,
                      255); // Min 10 to avoid completely dark
  lv_slider_set_value(brightness_slider, 128,
                      LV_ANIM_OFF); // Start at ~50% (128/255)

  // Apply custom styles
  lv_obj_add_style(brightness_slider, &style_slider_main, LV_PART_MAIN);
  lv_obj_add_style(brightness_slider, &style_slider_indicator,
                   LV_PART_INDICATOR);
  lv_obj_add_style(brightness_slider, &style_slider_knob, LV_PART_KNOB);

  lv_obj_add_event_cb(brightness_slider, slider_event_cb,
                      LV_EVENT_VALUE_CHANGED, NULL);

  // Trigger update for initial brightness (50%)
  lv_event_send(brightness_slider, LV_EVENT_VALUE_CHANGED, NULL);

  // Back button removed - Use Swipe Down gesture

  // Add swipe down gesture to go back
  lv_obj_add_flag(settings_scr, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(settings_scr, settings_gesture_cb, LV_EVENT_GESTURE,
                      NULL);

  // Initial state hidden
  lv_obj_add_flag(settings_scr, LV_OBJ_FLAG_HIDDEN);
}

void ui_settings_show(void) {
  if (!settings_scr) {
    ui_settings_init();
  }

  prev_screen = lv_scr_act();
  is_visible = true;

  // Show with animation
  lv_obj_clear_flag(settings_scr, LV_OBJ_FLAG_HIDDEN);
  lv_scr_load_anim(settings_scr, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, false);

  // Animate elements
  animate_fade_in(title_label, 100);
  animate_fade_in(brightness_card, 200);
  animate_fade_in(brightness_card, 200);
}

void ui_settings_hide(void) {
  if (!settings_scr || !is_visible)
    return;

  is_visible = false;

  if (prev_screen) {
    lv_scr_load_anim(prev_screen, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, false);
  }
}

bool ui_settings_is_visible(void) { return is_visible; }

void ui_settings_set_brightness_cb(void (*cb)(uint8_t)) {
  brightness_callback = cb;
}
