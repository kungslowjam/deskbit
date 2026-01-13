#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ROBO_EYE_EXPR_NORMAL = 0,
    ROBO_EYE_EXPR_HAPPY,
    ROBO_EYE_EXPR_SAD,
    ROBO_EYE_EXPR_LAUGH,
    ROBO_EYE_EXPR_LOVE,
    ROBO_EYE_EXPR_SLEEP,
    ROBO_EYE_EXPR_ANGRY,
    ROBO_EYE_EXPR_TALK,
    ROBO_EYE_EXPR_DIZZY,
    ROBO_EYE_EXPR_COUNT,
} robo_eye_expr_t;

void ui_robo_eyes_create(lv_obj_t *parent);
void ui_robo_eyes_set_expr(robo_eye_expr_t expr);
void ui_robo_eyes_set_active(bool active);

#ifdef __cplusplus
}
#endif
