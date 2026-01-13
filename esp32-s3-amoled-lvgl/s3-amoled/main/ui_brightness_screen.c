#include "ui_brightness_screen.h"

#include <inttypes.h>

#include "board_lcd.h"
#include "lvgl.h"

static void brightness_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);

    int32_t percent = lv_slider_get_value(slider);
    uint8_t level = (uint8_t)((percent * 255) / 100);

    board_lcd_set_brightness(level);

    if (label != NULL) {
        lv_label_set_text_fmt(label, "Brightness: %" PRIi32 "%%", percent);
    }
}

static void brightness_tile_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_opa(parent, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Brightness");
    lv_obj_set_style_text_color(title, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);

    lv_obj_t *value_label = lv_label_create(parent);
    lv_label_set_text(value_label, "Brightness: 50%");
    lv_obj_set_style_text_color(value_label, lv_color_hex(0x94A3B8), LV_PART_MAIN);
    lv_obj_set_style_text_align(value_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
#if LV_FONT_MONTSERRAT_20
    lv_obj_set_style_text_font(value_label, &lv_font_montserrat_20, LV_PART_MAIN);
#endif
    lv_obj_set_width(value_label, LV_PCT(100));
    lv_label_set_long_mode(value_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(value_label, LV_ALIGN_CENTER, 0, -40);

    lv_obj_t *slider = lv_slider_create(parent);
    lv_obj_set_size(slider, 320, 36);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 20);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 50, LV_ANIM_OFF);

    lv_obj_set_style_radius(slider, 18, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(slider, 18, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x38BDF8), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_INDICATOR);

    lv_obj_set_style_radius(slider, 18, LV_PART_KNOB);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0xF8FAFC), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_width(slider, 36, LV_PART_KNOB);
    lv_obj_set_style_height(slider, 36, LV_PART_KNOB);
    lv_obj_clear_flag(slider, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(slider, brightness_event_cb, LV_EVENT_VALUE_CHANGED, value_label);

    board_lcd_set_brightness((uint8_t)((50 * 255) / 100));
}

void ui_brightness_tile_create(lv_obj_t *parent)
{
    brightness_tile_create(parent);
}

void ui_brightness_screen_create(void)
{
    lv_obj_t *scr = lv_screen_active();

    lv_obj_t *tileview = lv_tileview_create(scr);
    lv_obj_set_size(tileview, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(tileview, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(tileview, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(tileview, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(tileview, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *brightness_tile = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_VER);
    brightness_tile_create(brightness_tile);

    lv_tileview_set_tile(tileview, brightness_tile, LV_ANIM_OFF);
}
