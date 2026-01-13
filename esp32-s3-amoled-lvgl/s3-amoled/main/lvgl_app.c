#include "lvgl_app.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "lvgl_detect.h"
#include "board_lcd.h"
#include "ui_brightness_screen.h"
#include "ui_robo_eyes.h"
#include "ui_gif_screen.h"

#if LVGL_AVAILABLE

static const char *kTag = "lvgl_app";
static esp_timer_handle_t s_tick_timer;
static lv_obj_t *s_robo_tile;
static lv_obj_t *s_gif_tile;

static void tileview_update_active(lv_obj_t *tileview)
{
    lv_obj_t *active = lv_tileview_get_tile_active(tileview);
    bool robo_active = (active == s_robo_tile);
    bool gif_active = (active == s_gif_tile);
    ui_robo_eyes_set_active(robo_active);
    ui_gif_screen_set_active(gif_active);
}

static void tileview_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *tileview = lv_event_get_target(e);

    if (code == LV_EVENT_SCROLL_BEGIN) {
        ui_robo_eyes_set_active(false);
        ui_gif_screen_set_active(false);
        return;
    }

    if (code == LV_EVENT_SCROLL_END || code == LV_EVENT_VALUE_CHANGED) {
        tileview_update_active(tileview);
    }
}

static void lvgl_tick_cb(void *arg)
{
    (void)arg;
    lv_tick_inc(2);
}

static void lvgl_task(void *arg)
{
    (void)arg;
    lv_obj_t *scr = lv_screen_active();
    lv_obj_t *tileview = lv_tileview_create(scr);
    lv_obj_set_size(tileview, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(tileview, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(tileview, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(tileview, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(tileview, tileview_event_cb, LV_EVENT_ALL, NULL);

    s_robo_tile = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_VER);
    lv_obj_t *brightness_tile = lv_tileview_add_tile(tileview, 0, 1, LV_DIR_VER);
    s_gif_tile = lv_tileview_add_tile(tileview, 0, 2, LV_DIR_VER);
    ui_robo_eyes_create(s_robo_tile);
    ui_brightness_tile_create(brightness_tile);
    ui_gif_screen_create(s_gif_tile);

    lv_tileview_set_tile(tileview, s_robo_tile, LV_ANIM_OFF);
    tileview_update_active(tileview);
    while (true) {
        uint32_t delay_ms = lv_timer_handler();
        if (delay_ms == LV_NO_TIMER_READY || delay_ms < 20) {
            delay_ms = 20;
        }
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

bool lvgl_app_start(void)
{
    lv_init();

    if (!board_lcd_init()) {
        ESP_LOGW(kTag, "Display init failed; LVGL UI not started");
        return false;
    }
    board_lcd_set_brightness(50);

    lv_display_t *disp = lv_display_get_default();
    if (disp != NULL) {
        lv_display_set_antialiasing(disp, true);
    }

    const esp_timer_create_args_t tick_args = {
        .callback = &lvgl_tick_cb,
        .name = "lvgl_tick",
    };
    if (esp_timer_create(&tick_args, &s_tick_timer) != ESP_OK) {
        ESP_LOGE(kTag, "Failed to create LVGL tick timer");
        return false;
    }
    if (esp_timer_start_periodic(s_tick_timer, 2000) != ESP_OK) {
        ESP_LOGE(kTag, "Failed to start LVGL tick timer");
        return false;
    }

    if (xTaskCreate(lvgl_task, "lvgl", 8192, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(kTag, "Failed to start LVGL task");
        return false;
    }

    return true;
}

#else

bool lvgl_app_start(void)
{
    return false;
}

#endif
