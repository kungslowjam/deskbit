#ifndef ESP_WIFI_BSP_H
#define ESP_WIFI_BSP_H

#include <stdbool.h>

// Callback types for UI updates (to avoid circular dependency)
typedef void (*wifi_scan_result_cb_t)(const char *ssid, int rssi);
typedef void (*wifi_status_cb_t)(bool connected, const char *ssid,
                                 const char *ip);
typedef void (*wifi_clear_networks_cb_t)(void);

/**
 * @brief Initialize NVS flash and network interfaces
 */
void nvs_flash_Init(void);

/**
 * @brief Initialize WiFi in STA mode
 */
void espwifi_Init(void);

/**
 * @brief Deinitialize WiFi
 */
void espwifi_Deinit(void);

/**
 * @brief Start WiFi scan for available networks
 * Results will be reported via the registered scan callback
 */
void espwifi_scan(void);

/**
 * @brief Connect to a WiFi network
 * @param ssid Network name
 * @param password Network password
 */
void espwifi_connect(const char *ssid, const char *password);

/**
 * @brief Check if WiFi is connected
 * @return true if connected
 */
bool espwifi_is_connected(void);

/**
 * @brief Get current IP address as string
 * @param ip_str Buffer to store IP string (at least 16 bytes)
 */
void espwifi_get_ip(char *ip_str);

/**
 * @brief Get current connected SSID
 * @param ssid_str Buffer to store SSID (at least 33 bytes)
 */
void espwifi_get_ssid(char *ssid_str);

/**
 * @brief Set callbacks for UI updates
 */
void espwifi_set_callbacks(wifi_scan_result_cb_t scan_result_cb,
                           wifi_status_cb_t status_cb,
                           wifi_clear_networks_cb_t clear_cb);

/**
 * @brief Forget saved WiFi credentials
 * Removes saved SSID and password from NVS
 */
void espwifi_forget_network(void);

#endif