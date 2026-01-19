#include "esp_wifi_bsp.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "WIFI_BSP";
#define NVS_NAMESPACE "wifi_config"
#define NVS_SSID_KEY "ssid"
#define NVS_PASSWORD_KEY "password"

// WiFi state
static bool wifi_connected = false;
static char current_ssid[33] = {0};
static char current_ip[16] = {0};

// UI Callbacks
static wifi_scan_result_cb_t g_scan_result_cb = NULL;
static wifi_status_cb_t g_status_cb = NULL;
static wifi_clear_networks_cb_t g_clear_cb = NULL;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data);

// Save WiFi credentials to NVS
static void save_wifi_credentials(const char *ssid, const char *password) {
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
    return;
  }

  // Save SSID
  err = nvs_set_str(nvs_handle, NVS_SSID_KEY, ssid);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to save SSID: %s", esp_err_to_name(err));
  }

  // Save Password
  err = nvs_set_str(nvs_handle, NVS_PASSWORD_KEY, password);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to save password: %s", esp_err_to_name(err));
  }

  // Commit
  err = nvs_commit(nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
  } else {
    ESP_LOGI(TAG, "WiFi credentials saved to NVS");
  }

  nvs_close(nvs_handle);
}

// Load WiFi credentials from NVS and auto-connect
static bool load_and_connect_saved_wifi(void) {
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGI(TAG, "No saved WiFi credentials");
    return false;
  }

  char ssid[33] = {0};
  char password[64] = {0};
  size_t ssid_len = sizeof(ssid);
  size_t pwd_len = sizeof(password);

  // Load SSID
  err = nvs_get_str(nvs_handle, NVS_SSID_KEY, ssid, &ssid_len);
  if (err != ESP_OK) {
    ESP_LOGI(TAG, "No saved SSID");
    nvs_close(nvs_handle);
    return false;
  }

  // Load Password
  err = nvs_get_str(nvs_handle, NVS_PASSWORD_KEY, password, &pwd_len);
  if (err != ESP_OK) {
    ESP_LOGI(TAG, "No saved password");
    nvs_close(nvs_handle);
    return false;
  }

  nvs_close(nvs_handle);

  ESP_LOGI(TAG, "Found saved WiFi: %s, attempting to connect...", ssid);
  espwifi_connect(ssid, password);
  return true;
}

void espwifi_set_callbacks(wifi_scan_result_cb_t scan_result_cb,
                           wifi_status_cb_t status_cb,
                           wifi_clear_networks_cb_t clear_cb) {
  g_scan_result_cb = scan_result_cb;
  g_status_cb = status_cb;
  g_clear_cb = clear_cb;
}

void nvs_flash_Init(void) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_create_default_wifi_sta();
}

void espwifi_Init(void) {
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);

  esp_event_handler_instance_t instance_wifi;
  esp_event_handler_instance_t instance_ip;

  esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                      &event_handler, NULL, &instance_wifi);
  esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                      &event_handler, NULL, &instance_ip);

  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_start();

  ESP_LOGI(TAG, "WiFi initialized in STA mode");

  // Auto-connect to saved WiFi if exists
  load_and_connect_saved_wifi();
}

void espwifi_Deinit(void) {
  esp_wifi_disconnect();
  esp_wifi_stop();
  esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
  esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);
  esp_wifi_deinit();
  wifi_connected = false;
  ESP_LOGI(TAG, "WiFi deinitialized");
}

void espwifi_scan(void) {
  ESP_LOGI(TAG, "Starting WiFi scan...");

  // Clear previous results in UI
  if (g_clear_cb) {
    g_clear_cb();
  }

  // Configure scan
  wifi_scan_config_t scan_config = {
      .ssid = NULL,
      .bssid = NULL,
      .channel = 0,
      .show_hidden = false,
      .scan_type = WIFI_SCAN_TYPE_ACTIVE,
      .scan_time.active.min = 100,
      .scan_time.active.max = 300,
  };

  // Start scan (blocking)
  esp_err_t err = esp_wifi_scan_start(&scan_config, true);

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "WiFi scan failed: %s", esp_err_to_name(err));
    return;
  }

  // Get scan results
  uint16_t ap_count = 0;
  esp_wifi_scan_get_ap_num(&ap_count);

  ESP_LOGI(TAG, "Found %d access points", ap_count);

  if (ap_count == 0) {
    return;
  }

  // Limit to prevent memory issues
  if (ap_count > 20) {
    ap_count = 20;
  }

  wifi_ap_record_t *ap_list = malloc(sizeof(wifi_ap_record_t) * ap_count);
  if (ap_list == NULL) {
    ESP_LOGE(TAG, "Failed to allocate memory for AP list");
    return;
  }

  esp_wifi_scan_get_ap_records(&ap_count, ap_list);

  // Add each network via callback
  for (int i = 0; i < ap_count; i++) {
    // Skip hidden networks
    if (strlen((char *)ap_list[i].ssid) == 0) {
      continue;
    }

    ESP_LOGI(TAG, "AP[%d]: %s (RSSI: %d)", i, ap_list[i].ssid, ap_list[i].rssi);

    if (g_scan_result_cb) {
      g_scan_result_cb((char *)ap_list[i].ssid, ap_list[i].rssi);
    }
  }

  free(ap_list);
  ESP_LOGI(TAG, "Scan complete");
}

void espwifi_connect(const char *ssid, const char *password) {
  if (ssid == NULL) {
    ESP_LOGE(TAG, "SSID is NULL");
    return;
  }

  ESP_LOGI(TAG, "Connecting to: %s", ssid);

  // Disconnect from current network first
  esp_wifi_disconnect();

  // Configure new connection
  wifi_config_t wifi_config = {0};
  strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);

  if (password != NULL && strlen(password) > 0) {
    strncpy((char *)wifi_config.sta.password, password,
            sizeof(wifi_config.sta.password) - 1);
  }

  // Store SSID for status updates
  strncpy(current_ssid, ssid, sizeof(current_ssid) - 1);

  esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
  esp_err_t err = esp_wifi_connect();

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "WiFi connect failed: %s", esp_err_to_name(err));
    if (g_status_cb) {
      g_status_cb(false, ssid, NULL);
    }
  } else {
    // Save credentials to NVS for auto-reconnect
    save_wifi_credentials(ssid, password ? password : "");
  }
}

bool espwifi_is_connected(void) { return wifi_connected; }

void espwifi_get_ip(char *ip_str) {
  if (ip_str != NULL) {
    strncpy(ip_str, current_ip, 15);
  }
}

void espwifi_get_ssid(char *ssid_str) {
  if (ssid_str != NULL) {
    strncpy(ssid_str, current_ssid, 32);
  }
}

void espwifi_forget_network(void) {
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
  if (err == ESP_OK) {
    nvs_erase_key(nvs_handle, NVS_SSID_KEY);
    nvs_erase_key(nvs_handle, NVS_PASSWORD_KEY);
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "Saved WiFi credentials erased");
  }

  // Disconnect current connection
  esp_wifi_disconnect();
}

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT) {
    switch (event_id) {
    case WIFI_EVENT_STA_START:
      ESP_LOGI(TAG, "WiFi STA started");
      break;

    case WIFI_EVENT_STA_CONNECTED:
      ESP_LOGI(TAG, "Connected to AP");
      break;

    case WIFI_EVENT_STA_DISCONNECTED:
      ESP_LOGW(TAG, "Disconnected from AP");
      wifi_connected = false;
      current_ip[0] = '\0';
      if (g_status_cb) {
        g_status_cb(false, current_ssid, NULL);
      }
      break;

    case WIFI_EVENT_SCAN_DONE:
      ESP_LOGI(TAG, "Scan done event");
      break;
    }
  } else if (event_base == IP_EVENT) {
    if (event_id == IP_EVENT_STA_GOT_IP) {
      ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
      uint32_t ip = event->ip_info.ip.addr;

      snprintf(current_ip, sizeof(current_ip), "%d.%d.%d.%d", (uint8_t)(ip),
               (uint8_t)(ip >> 8), (uint8_t)(ip >> 16), (uint8_t)(ip >> 24));

      ESP_LOGI(TAG, "Got IP: %s", current_ip);
      wifi_connected = true;

      // Update UI
      if (g_status_cb) {
        g_status_cb(true, current_ssid, current_ip);
      }
    }
  }
}
