#include "ui_squareline_screen.h"

#include "GUI.h"

static bool s_squareline_inited;
static lv_obj_t *s_squareline_root;

void ui_squareline_screen_create(lv_obj_t *parent)
{
    if (parent == NULL) {
        return;
    }

    if (!s_squareline_inited) {
        GUI_initGlobalStyles();
        GUI_initScreen__Thermostat(parent);
        GUI_initAnimations();
        s_squareline_root = GUI_Screen__Thermostat;
        s_squareline_inited = true;
    }

    if (s_squareline_root == NULL) {
        return;
    }

    lv_obj_set_size(s_squareline_root, LV_PCT(100), LV_PCT(100));
    lv_obj_center(s_squareline_root);
    lv_obj_clear_flag(s_squareline_root, LV_OBJ_FLAG_SCROLLABLE);
}
