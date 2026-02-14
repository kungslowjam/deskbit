#ifndef UI_LIQUID_H
#define UI_LIQUID_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_liquid_init(void);
void ui_liquid_show(lv_obj_t *parent);
void ui_liquid_hide(void);

#ifdef __cplusplus
}
#endif

#endif // UI_LIQUID_H
