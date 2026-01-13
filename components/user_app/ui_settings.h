/**
 * @file ui_settings.h
 * @brief Modern Settings Screen with Brightness Control
 */

#ifndef UI_SETTINGS_H
#define UI_SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/**
 * @brief Initialize the settings screen
 */
void ui_settings_init(void);

/**
 * @brief Show the settings screen
 */
void ui_settings_show(void);

/**
 * @brief Hide the settings screen and return to previous screen
 */
void ui_settings_hide(void);

/**
 * @brief Check if settings screen is currently visible
 */
bool ui_settings_is_visible(void);

/**
 * @brief Set brightness callback function
 * @param cb Callback function that takes uint8_t brightness value (0-255)
 */
void ui_settings_set_brightness_cb(void (*cb)(uint8_t));

#ifdef __cplusplus
}
#endif

#endif // UI_SETTINGS_H
