/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl_app.h"

void app_main(void)
{
    static const char *tag = "app";

    ESP_LOGI(tag, "Starting LVGL UI");
    if (!lvgl_app_start()) {
        ESP_LOGW(tag, "LVGL not available or display init missing");
        while (true) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}
