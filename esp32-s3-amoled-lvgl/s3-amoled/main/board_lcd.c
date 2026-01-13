#include "board_lcd.h"

#include <stdint.h>
#include <stdlib.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"

#include "esp_lcd_sh8601.h"
#include "lvgl.h"
#include "draw/sw/lv_draw_sw_utils.h"
#include "read_lcd_id_bsp.h"
#include "touch_bsp.h"

static const char *kTag = "board_lcd";

#define LCD_HOST SPI2_HOST

#define BOARD_LCD_ID_SH8601 0x86

#define BOARD_PIN_LCD_CS    (GPIO_NUM_9)
#define BOARD_PIN_LCD_PCLK  (GPIO_NUM_10)
#define BOARD_PIN_LCD_DATA0 (GPIO_NUM_11)
#define BOARD_PIN_LCD_DATA1 (GPIO_NUM_12)
#define BOARD_PIN_LCD_DATA2 (GPIO_NUM_13)
#define BOARD_PIN_LCD_DATA3 (GPIO_NUM_14)
#define BOARD_PIN_LCD_RST   (GPIO_NUM_21)
#define BOARD_PIN_LCD_EN    (GPIO_NUM_42)

#define BOARD_LCD_H_RES 466
#define BOARD_LCD_V_RES 466
#define BOARD_LVGL_BUF_HEIGHT 32

#define BOARD_DISPLAY_ROTATION LV_DISPLAY_ROTATION_0
#define BOARD_RGB565_SWAP 1
#define BOARD_TOUCH_SWAP_XY 0
#define BOARD_TOUCH_INVERT_X 0
#define BOARD_TOUCH_INVERT_Y 0
#define BOARD_TOUCH_RAW_MIN_X 0
#define BOARD_TOUCH_RAW_MAX_X (BOARD_LCD_H_RES - 1)
#define BOARD_TOUCH_RAW_MIN_Y 0
#define BOARD_TOUCH_RAW_MAX_Y (BOARD_LCD_V_RES - 1)
#define BOARD_TOUCH_OFFSET_X 0
#define BOARD_TOUCH_OFFSET_Y 0
#define BOARD_TOUCH_DEBUG 0

#if LV_COLOR_DEPTH == 32
#define BOARD_LCD_BIT_PER_PIXEL 24
#elif LV_COLOR_DEPTH == 16
#define BOARD_LCD_BIT_PER_PIXEL 16
#else
#error "Unsupported LV_COLOR_DEPTH for SH8601"
#endif

static uint8_t s_lcd_id;
static esp_lcd_panel_io_handle_t s_panel_io;
static lv_display_t *s_disp;
static esp_lcd_panel_handle_t s_panel_handle;

static void touch_map_coordinates(lv_display_t *disp, uint16_t *x, uint16_t *y)
{
    if (disp == NULL || x == NULL || y == NULL) {
        return;
    }

    const int32_t phys_w = lv_display_get_original_horizontal_resolution(disp);
    const int32_t phys_h = lv_display_get_original_vertical_resolution(disp);

    int32_t raw_x = *x;
    int32_t raw_y = *y;

#if BOARD_TOUCH_SWAP_XY
    {
        int32_t tmp = raw_x;
        raw_x = raw_y;
        raw_y = tmp;
    }
#endif

#if BOARD_TOUCH_INVERT_X
    raw_x = phys_w - 1 - raw_x;
#endif

#if BOARD_TOUCH_INVERT_Y
    raw_y = phys_h - 1 - raw_y;
#endif

    int32_t out_x = raw_x;
    int32_t out_y = raw_y;
    const int32_t range_x = BOARD_TOUCH_RAW_MAX_X - BOARD_TOUCH_RAW_MIN_X;
    const int32_t range_y = BOARD_TOUCH_RAW_MAX_Y - BOARD_TOUCH_RAW_MIN_Y;
    if (range_x > 0) {
        out_x = (raw_x - BOARD_TOUCH_RAW_MIN_X) * (phys_w - 1) / range_x;
        out_x += BOARD_TOUCH_OFFSET_X;
    }
    if (range_y > 0) {
        out_y = (raw_y - BOARD_TOUCH_RAW_MIN_Y) * (phys_h - 1) / range_y;
        out_y += BOARD_TOUCH_OFFSET_Y;
    }

    const lv_display_rotation_t rot = lv_display_get_rotation(disp);

    const int32_t rot_x = out_x;
    const int32_t rot_y = out_y;
    switch (rot) {
    case LV_DISPLAY_ROTATION_90:
        out_x = phys_h - 1 - rot_y;
        out_y = rot_x;
        break;
    case LV_DISPLAY_ROTATION_180:
        out_x = phys_w - 1 - rot_x;
        out_y = phys_h - 1 - rot_y;
        break;
    case LV_DISPLAY_ROTATION_270:
        out_x = rot_y;
        out_y = phys_w - 1 - rot_x;
        break;
    case LV_DISPLAY_ROTATION_0:
    default:
        break;
    }

    const int32_t disp_w = lv_display_get_horizontal_resolution(disp);
    const int32_t disp_h = lv_display_get_vertical_resolution(disp);
    if (out_x < 0) out_x = 0;
    if (out_y < 0) out_y = 0;
    if (out_x >= disp_w) out_x = disp_w - 1;
    if (out_y >= disp_h) out_y = disp_h - 1;

    *x = (uint16_t)out_x;
    *y = (uint16_t)out_y;

#if BOARD_TOUCH_DEBUG
    ESP_LOGI(kTag, "Touch raw(%ld,%ld) -> (%u,%u)", (long)raw_x, (long)raw_y, *x, *y);
#endif
}

static const sh8601_lcd_init_cmd_t s_sh8601_init_cmds[] = {
    {0x11, (uint8_t []){0x00}, 0, 120},
    {0x44, (uint8_t []){0x01, 0xD1}, 2, 0},
    {0x35, (uint8_t []){0x00}, 1, 0},
    {0x53, (uint8_t []){0x20}, 1, 10},
    {0x51, (uint8_t []){0x00}, 1, 10},
    {0x29, (uint8_t []){0x00}, 0, 10},
    {0x51, (uint8_t []){0xFF}, 1, 0},
};

static const sh8601_lcd_init_cmd_t s_co5300_init_cmds[] = {
    {0x11, (uint8_t []){0x00}, 0, 80},
    {0xC4, (uint8_t []){0x80}, 1, 0},
    {0x53, (uint8_t []){0x20}, 1, 1},
    {0x63, (uint8_t []){0xFF}, 1, 1},
    {0x51, (uint8_t []){0x00}, 1, 1},
    {0x29, (uint8_t []){0x00}, 0, 10},
    {0x51, (uint8_t []){0xFF}, 1, 0},
};

static bool lcd_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io,
                                        esp_lcd_panel_io_event_data_t *edata,
                                        void *user_ctx)
{
    (void)panel_io;
    (void)edata;
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

static void lcd_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);
    const int offsetx1 = (s_lcd_id == BOARD_LCD_ID_SH8601) ? area->x1 : area->x1 + 0x06;
    const int offsetx2 = (s_lcd_id == BOARD_LCD_ID_SH8601) ? area->x2 : area->x2 + 0x06;
    const int offsety1 = area->y1;
    const int offsety2 = area->y2;
    const uint32_t pixel_count = (uint32_t)(offsetx2 - offsetx1 + 1) * (uint32_t)(offsety2 - offsety1 + 1);

#if LV_COLOR_DEPTH == 32
    lv_color_t *color_map = (lv_color_t *)px_map;
    uint8_t *to = (uint8_t *)px_map;
    uint16_t pixel_num = (offsetx2 - offsetx1 + 1) * (offsety2 - offsety1 + 1);

    uint8_t temp = color_map[0].ch.blue;
    *to++ = color_map[0].ch.red;
    *to++ = color_map[0].ch.green;
    *to++ = temp;

    for (int i = 1; i < pixel_num; i++) {
        *to++ = color_map[i].ch.red;
        *to++ = color_map[i].ch.green;
        *to++ = color_map[i].ch.blue;
    }
#elif LV_COLOR_DEPTH == 16
#if BOARD_RGB565_SWAP
    lv_draw_sw_rgb565_swap(px_map, pixel_count);
#endif
#endif

    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);
}

static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    uint16_t tp_x = 0;
    uint16_t tp_y = 0;
    uint8_t pressed = getTouch(&tp_x, &tp_y);
    if (pressed) {
        touch_map_coordinates(s_disp, &tp_x, &tp_y);
        data->point.x = tp_x;
        data->point.y = tp_y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

bool board_lcd_init(void)
{
    if (BOARD_PIN_LCD_EN >= 0) {
        gpio_config_t en_gpio_config = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << BOARD_PIN_LCD_EN,
        };
        ESP_ERROR_CHECK(gpio_config(&en_gpio_config));
        gpio_set_level(BOARD_PIN_LCD_EN, 1);
    }

    s_lcd_id = read_lcd_id();

    lv_display_t *disp = lv_display_create(BOARD_LCD_H_RES, BOARD_LCD_V_RES);
    if (disp == NULL) {
        ESP_LOGE(kTag, "Failed to create LVGL display");
        return false;
    }
    s_disp = disp;

#if LV_COLOR_DEPTH == 16
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
#elif LV_COLOR_DEPTH == 32
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_XRGB8888);
#endif

    size_t buf_size = BOARD_LCD_H_RES * BOARD_LVGL_BUF_HEIGHT * sizeof(lv_color_t);
    lv_color_t *buf1 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    lv_color_t *buf2 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    if (buf1 == NULL) {
        ESP_LOGE(kTag, "Failed to allocate LVGL draw buffer");
        free(buf2);
        return false;
    }
    if (buf2 == NULL) {
        ESP_LOGW(kTag, "Second LVGL buffer alloc failed; using single buffer");
    }

    lv_display_set_buffers(disp, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, lcd_lvgl_flush_cb);

    lv_display_set_rotation(disp, BOARD_DISPLAY_ROTATION);

    const spi_bus_config_t buscfg = SH8601_PANEL_BUS_QSPI_CONFIG(
        BOARD_PIN_LCD_PCLK,
        BOARD_PIN_LCD_DATA0,
        BOARD_PIN_LCD_DATA1,
        BOARD_PIN_LCD_DATA2,
        BOARD_PIN_LCD_DATA3,
        BOARD_LCD_H_RES * BOARD_LCD_V_RES * BOARD_LCD_BIT_PER_PIXEL / 8);
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    const esp_lcd_panel_io_spi_config_t io_config = SH8601_PANEL_IO_QSPI_CONFIG(
        BOARD_PIN_LCD_CS,
        lcd_notify_lvgl_flush_ready,
        disp);
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));
    s_panel_io = io_handle;

    sh8601_vendor_config_t vendor_config = {
        .flags = {
            .use_qspi_interface = 1,
        },
    };
    vendor_config.init_cmds = (s_lcd_id == BOARD_LCD_ID_SH8601) ? s_sh8601_init_cmds : s_co5300_init_cmds;
    vendor_config.init_cmds_size = (s_lcd_id == BOARD_LCD_ID_SH8601)
        ? (sizeof(s_sh8601_init_cmds) / sizeof(s_sh8601_init_cmds[0]))
        : (sizeof(s_co5300_init_cmds) / sizeof(s_co5300_init_cmds[0]));

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BOARD_PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = BOARD_LCD_BIT_PER_PIXEL,
        .vendor_config = &vendor_config,
    };

    esp_lcd_panel_handle_t panel_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_sh8601(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    s_panel_handle = panel_handle;

    lv_display_set_user_data(disp, panel_handle);

    Touch_Init();
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read_cb);
    lv_indev_set_display(indev, disp);

    ESP_LOGI(kTag, "LCD initialized (id=0x%02X)", s_lcd_id);
    return true;
}

void board_lcd_set_brightness(uint8_t level)
{
    if (s_panel_io == NULL) {
        ESP_LOGW(kTag, "Brightness set skipped; panel IO not ready");
        return;
    }

    uint8_t value = level;
    uint32_t cmd = 0x51;
    cmd = (cmd & 0xffu) << 8;
    cmd |= (0x02u << 24);
    esp_err_t err = esp_lcd_panel_io_tx_param(s_panel_io, cmd, &value, 1);
    if (err != ESP_OK) {
        ESP_LOGW(kTag, "Brightness set failed: %s", esp_err_to_name(err));
    }
}

bool board_lcd_draw_bitmap(int x, int y, int w, int h, const void *color_data)
{
    if (s_panel_handle == NULL || color_data == NULL) {
        return false;
    }
    if (w <= 0 || h <= 0) {
        return false;
    }

    const int offsetx1 = (s_lcd_id == BOARD_LCD_ID_SH8601) ? x : x + 0x06;
    const int offsetx2 = offsetx1 + w;
    const int offsety1 = y;
    const int offsety2 = y + h;
    if (esp_lcd_panel_draw_bitmap(s_panel_handle, offsetx1, offsety1, offsetx2, offsety2, color_data) != ESP_OK) {
        ESP_LOGW(kTag, "Draw bitmap failed");
        return false;
    }
    return true;
}

void board_lcd_get_resolution(uint16_t *width, uint16_t *height)
{
    if (width != NULL) {
        *width = BOARD_LCD_H_RES;
    }
    if (height != NULL) {
        *height = BOARD_LCD_V_RES;
    }
}

bool board_lcd_fill_color(uint16_t color)
{
    if (s_panel_handle == NULL) {
        return false;
    }

    uint16_t width = BOARD_LCD_H_RES;
    uint16_t height = BOARD_LCD_V_RES;
    uint16_t *line = heap_caps_malloc(width * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (line == NULL) {
        ESP_LOGW(kTag, "Fill color alloc failed");
        return false;
    }

    for (uint16_t x = 0; x < width; ++x) {
        line[x] = color;
    }

    for (uint16_t y = 0; y < height; ++y) {
        if (!board_lcd_draw_bitmap(0, y, width, 1, line)) {
            heap_caps_free(line);
            return false;
        }
    }

    heap_caps_free(line);
    return true;
}
