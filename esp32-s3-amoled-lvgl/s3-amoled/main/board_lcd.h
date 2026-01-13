#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool board_lcd_init(void);
void board_lcd_set_brightness(uint8_t level);
bool board_lcd_draw_bitmap(int x, int y, int w, int h, const void *color_data);
void board_lcd_get_resolution(uint16_t *width, uint16_t *height);
bool board_lcd_fill_color(uint16_t color);

#ifdef __cplusplus
}
#endif
