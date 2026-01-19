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

/**
 * @brief Update WiFi status display
 * @param connected True if connected
 * @param ssid Network name (can be NULL)
 * @param ip IP address string (can be NULL)
 */
void ui_settings_wifi_update_status(bool connected, const char *ssid,
                                    const char *ip);

/**
 * @brief Add a WiFi network to the available list
 * @param ssid Network name
 * @param rssi Signal strength
 */
void ui_settings_wifi_add_network(const char *ssid, int rssi);

/**
 * @brief Clear the WiFi network list
 */
void ui_settings_wifi_clear_networks(void);

/**
 * @brief Set callback for WiFi scan trigger
 */
void ui_settings_wifi_set_scan_cb(void (*cb)(void));

/**
 * @brief Set callback when user selects a WiFi network
 * @param cb Callback function that receives the selected SSID and password
 */
void ui_settings_wifi_set_connect_cb(void (*cb)(const char *ssid,
                                                const char *password));

/**
 * @brief Create WiFi status indicator on any screen
 * @param parent Parent screen object to attach indicator to
 */
void ui_settings_create_wifi_indicator_on(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif

#endif // UI_SETTINGS_H
