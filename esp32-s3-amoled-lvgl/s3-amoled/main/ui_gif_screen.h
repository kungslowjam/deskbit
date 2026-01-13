#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_gif_screen_create(lv_obj_t *parent);
void ui_gif_screen_set_active(bool active);

#ifdef __cplusplus
}
#endif
