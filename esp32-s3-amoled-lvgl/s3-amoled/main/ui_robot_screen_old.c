#include "ui_robot_screen.h"

#include "lvgl.h"

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *eye_l;
    lv_obj_t *eye_r;
    int16_t base_x_l;
    int16_t base_x_r;
    int16_t base_y;
    int16_t eye_w;
    int16_t eye_h;
    int16_t gap;
} face_t;

static face_t g_face;
static int16_t s_look_offset;
static int16_t s_look_limit = 16;
static bool s_face_ready;

#define DISPLAY_DIAMETER_DEFAULT 466
#define FACE_SCALE_PCT 94
#define FACE_CENTER_OFFSET_X 0
#define FACE_CENTER_OFFSET_Y 0
#define FACE_EYE_Y_PCT -8
#define EYE_W_PCT 18
#define EYE_H_PCT 18
#define EYE_GAP_PCT 12
#define LOOK_LIMIT_PCT 3
#define CHEEK_SIZE_PCT 9
#define CHEEK_Y_PCT 22
#define CHEEK_X_PCT 25
#define NOSE_W_PCT 5
#define NOSE_H_PCT 2
#define MOUTH_W_PCT 14
#define MOUTH_H_PCT 6
#define FACE_ONLY_EYES 1

static void set_eye_h(void *obj, int32_t v)
{
    lv_obj_set_height((lv_obj_t *)obj, (lv_coord_t)v);
}

static void set_eye_y(void *obj, int32_t v)
{
    lv_obj_set_y((lv_obj_t *)obj, (lv_coord_t)v);
}

static void set_look(void *unused, int32_t v)
{
    if (!s_face_ready) {
        return;
    }
    LV_UNUSED(unused);
    lv_obj_set_x(g_face.eye_l, g_face.base_x_l + (lv_coord_t)v);
    lv_obj_set_x(g_face.eye_r, g_face.base_x_r + (lv_coord_t)v);
}

static void face_create(lv_obj_t *parent)
{
    s_face_ready = false;
    lv_obj_update_layout(parent);
    lv_coord_t parent_w = lv_obj_get_width(parent);
    lv_coord_t parent_h = lv_obj_get_height(parent);
    if (parent_w == 0 || parent_h == 0) {
        lv_display_t *disp = lv_display_get_default();
        if (disp != NULL) {
            parent_w = lv_display_get_horizontal_resolution(disp);
            parent_h = lv_display_get_vertical_resolution(disp);
            if (parent_w > 0 && parent_h > 0) {
                lv_obj_set_size(parent, parent_w, parent_h);
                lv_obj_update_layout(parent);
            }
        }
    }
    if (parent_w == 0 || parent_h == 0) {
        parent_w = DISPLAY_DIAMETER_DEFAULT;
        parent_h = DISPLAY_DIAMETER_DEFAULT;
    }
    lv_coord_t min_side = parent_w < parent_h ? parent_w : parent_h;
    lv_coord_t face_size = (lv_coord_t)((min_side * FACE_SCALE_PCT) / 100);
    lv_coord_t eye_offset_y = (lv_coord_t)((face_size * FACE_EYE_Y_PCT) / 100);
    lv_coord_t face_radius = LV_RADIUS_CIRCLE;

    g_face.screen = lv_obj_create(parent);
    if (g_face.screen == NULL) {
        return;
    }
    lv_obj_set_size(g_face.screen, face_size, face_size);
    lv_obj_align(g_face.screen, LV_ALIGN_CENTER, FACE_CENTER_OFFSET_X, FACE_CENTER_OFFSET_Y);
    lv_obj_set_style_bg_color(g_face.screen, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(g_face.screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_face.screen, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(g_face.screen, lv_color_hex(0x2B324A), LV_PART_MAIN);
    lv_obj_set_style_radius(g_face.screen, face_radius, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(g_face.screen, 18, LV_PART_MAIN);
    lv_obj_set_style_shadow_spread(g_face.screen, 2, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(g_face.screen, lv_color_hex(0x0B1227), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(g_face.screen, LV_OPA_40, LV_PART_MAIN);
    lv_obj_clear_flag(g_face.screen, LV_OBJ_FLAG_SCROLLABLE);

    g_face.eye_w = (int16_t)((face_size * EYE_W_PCT) / 100);
    g_face.eye_h = (int16_t)((face_size * EYE_H_PCT) / 100);
    g_face.gap = (int16_t)((face_size * EYE_GAP_PCT) / 100);
    g_face.eye_l = lv_obj_create(g_face.screen);
    if (g_face.eye_l == NULL) {
        return;
    }
    lv_obj_set_size(g_face.eye_l, g_face.eye_w, g_face.eye_h);
    lv_obj_set_style_bg_color(g_face.eye_l, lv_color_hex(0x24D6FF), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(g_face.eye_l, lv_color_hex(0x6CF6FF), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(g_face.eye_l, LV_GRAD_DIR_VER, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(g_face.eye_l, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_face.eye_l, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(g_face.eye_l, LV_RADIUS_CIRCLE, LV_PART_MAIN);

    g_face.eye_r = lv_obj_create(g_face.screen);
    if (g_face.eye_r == NULL) {
        return;
    }
    lv_obj_set_size(g_face.eye_r, g_face.eye_w, g_face.eye_h);
    lv_obj_set_style_bg_color(g_face.eye_r, lv_color_hex(0x24D6FF), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(g_face.eye_r, lv_color_hex(0x6CF6FF), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(g_face.eye_r, LV_GRAD_DIR_VER, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(g_face.eye_r, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_face.eye_r, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(g_face.eye_r, LV_RADIUS_CIRCLE, LV_PART_MAIN);

    if (g_face.eye_l == NULL || g_face.eye_r == NULL) {
        return;
    }

    lv_coord_t shine_size = g_face.eye_w / 3;
    lv_coord_t shine_offset = g_face.eye_w / 7;

    lv_obj_t *shine_l = lv_obj_create(g_face.eye_l);
    if (shine_l != NULL) {
        lv_obj_set_size(shine_l, shine_size, shine_size);
        lv_obj_set_style_bg_color(shine_l, lv_color_hex(0xE7FDFF), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(shine_l, LV_OPA_80, LV_PART_MAIN);
        lv_obj_set_style_radius(shine_l, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_border_width(shine_l, 0, LV_PART_MAIN);
        lv_obj_align(shine_l, LV_ALIGN_TOP_LEFT, shine_offset, shine_offset);
    }

    lv_obj_t *shine_r = lv_obj_create(g_face.eye_r);
    if (shine_r != NULL) {
        lv_obj_set_size(shine_r, shine_size, shine_size);
        lv_obj_set_style_bg_color(shine_r, lv_color_hex(0xE7FDFF), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(shine_r, LV_OPA_80, LV_PART_MAIN);
        lv_obj_set_style_radius(shine_r, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_border_width(shine_r, 0, LV_PART_MAIN);
        lv_obj_align(shine_r, LV_ALIGN_TOP_LEFT, shine_offset, shine_offset);
    }

    lv_obj_set_style_shadow_width(g_face.eye_l, 16, LV_PART_MAIN);
    lv_obj_set_style_shadow_spread(g_face.eye_l, 2, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(g_face.eye_l, lv_color_hex(0x34E7FF), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(g_face.eye_l, LV_OPA_60, LV_PART_MAIN);

    lv_obj_set_style_shadow_width(g_face.eye_r, 16, LV_PART_MAIN);
    lv_obj_set_style_shadow_spread(g_face.eye_r, 2, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(g_face.eye_r, lv_color_hex(0x34E7FF), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(g_face.eye_r, LV_OPA_60, LV_PART_MAIN);

    lv_coord_t eye_offset = (g_face.gap / 2) + (g_face.eye_w / 2);
    if ((eye_offset * 2 + g_face.eye_w) > face_size) {
        g_face.gap = 0;
        eye_offset = g_face.eye_w / 2;
    }
    lv_obj_align(g_face.eye_l, LV_ALIGN_CENTER, -eye_offset, eye_offset_y);
    lv_obj_align(g_face.eye_r, LV_ALIGN_CENTER, eye_offset, eye_offset_y);
    g_face.base_x_l = lv_obj_get_x(g_face.eye_l);
    g_face.base_x_r = lv_obj_get_x(g_face.eye_r);
    g_face.base_y = lv_obj_get_y(g_face.eye_l);

#if !FACE_ONLY_EYES
    lv_obj_t *brow_l = lv_arc_create(g_face.screen);
    lv_obj_t *brow_r = lv_arc_create(g_face.screen);
    if (brow_l != NULL && brow_r != NULL) {
        lv_coord_t brow_w = g_face.eye_w + (g_face.eye_w / 2);
        lv_coord_t brow_h = g_face.eye_h;
        lv_obj_set_size(brow_l, brow_w, brow_h);
        lv_obj_set_size(brow_r, brow_w, brow_h);
        lv_obj_remove_style(brow_l, NULL, LV_PART_KNOB);
        lv_obj_remove_style(brow_r, NULL, LV_PART_KNOB);
        lv_obj_set_style_bg_opa(brow_l, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(brow_r, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_arc_width(brow_l, 4, LV_PART_MAIN);
        lv_obj_set_style_arc_width(brow_r, 4, LV_PART_MAIN);
        lv_obj_set_style_arc_color(brow_l, lv_color_hex(0x7ADFFF), LV_PART_MAIN);
        lv_obj_set_style_arc_color(brow_r, lv_color_hex(0x7ADFFF), LV_PART_MAIN);
        lv_arc_set_start_angle(brow_l, 200);
        lv_arc_set_end_angle(brow_l, 300);
        lv_arc_set_start_angle(brow_r, 240);
        lv_arc_set_end_angle(brow_r, 340);
        lv_obj_align(brow_l, LV_ALIGN_CENTER, -eye_offset, eye_offset_y - (g_face.eye_h / 2) - (face_size / 20));
        lv_obj_align(brow_r, LV_ALIGN_CENTER, eye_offset, eye_offset_y - (g_face.eye_h / 2) - (face_size / 20));
    }
#endif

#if !FACE_ONLY_EYES
    lv_coord_t cheek_size = (lv_coord_t)((face_size * CHEEK_SIZE_PCT) / 100);
    lv_coord_t cheek_offset_x = (lv_coord_t)((face_size * CHEEK_X_PCT) / 100);
    lv_coord_t cheek_offset_y = (lv_coord_t)((face_size * CHEEK_Y_PCT) / 100);
#endif

#if !FACE_ONLY_EYES
    lv_obj_t *cheek_l = lv_obj_create(g_face.screen);
    lv_obj_t *cheek_r = lv_obj_create(g_face.screen);
    if (cheek_l != NULL && cheek_r != NULL) {
        lv_obj_set_size(cheek_l, cheek_size, cheek_size);
        lv_obj_set_size(cheek_r, cheek_size, cheek_size);
        lv_obj_set_style_bg_color(cheek_l, lv_color_hex(0xFF7AA8), LV_PART_MAIN);
        lv_obj_set_style_bg_color(cheek_r, lv_color_hex(0xFF7AA8), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(cheek_l, LV_OPA_90, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(cheek_r, LV_OPA_90, LV_PART_MAIN);
        lv_obj_set_style_radius(cheek_l, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_radius(cheek_r, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_border_width(cheek_l, 0, LV_PART_MAIN);
        lv_obj_set_style_border_width(cheek_r, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(cheek_l, 14, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(cheek_r, 14, LV_PART_MAIN);
        lv_obj_set_style_shadow_color(cheek_l, lv_color_hex(0xFF7AA8), LV_PART_MAIN);
        lv_obj_set_style_shadow_color(cheek_r, lv_color_hex(0xFF7AA8), LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(cheek_l, LV_OPA_40, LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(cheek_r, LV_OPA_40, LV_PART_MAIN);
        lv_obj_align(cheek_l, LV_ALIGN_CENTER, -cheek_offset_x, cheek_offset_y);
        lv_obj_align(cheek_r, LV_ALIGN_CENTER, cheek_offset_x, cheek_offset_y);
    }

    lv_obj_t *nose = lv_obj_create(g_face.screen);
    if (nose != NULL) {
        lv_coord_t nose_w = (lv_coord_t)((face_size * NOSE_W_PCT) / 100);
        lv_coord_t nose_h = (lv_coord_t)((face_size * NOSE_H_PCT) / 100);
        lv_coord_t nose_offset_y = cheek_offset_y - (cheek_size / 2) - (face_size / 44);
        lv_obj_set_size(nose, nose_w, nose_h);
        lv_obj_set_style_bg_color(nose, lv_color_hex(0xFFC857), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(nose, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_radius(nose, nose_h / 2, LV_PART_MAIN);
        lv_obj_set_style_border_width(nose, 0, LV_PART_MAIN);
        lv_obj_align(nose, LV_ALIGN_CENTER, 0, nose_offset_y);
    }

    lv_obj_t *mouth = lv_arc_create(g_face.screen);
    if (mouth != NULL) {
        lv_coord_t mouth_w = (lv_coord_t)((face_size * MOUTH_W_PCT) / 100);
        lv_coord_t mouth_h = (lv_coord_t)((face_size * MOUTH_H_PCT) / 100);
        lv_coord_t mouth_offset_y = cheek_offset_y + (cheek_size / 2) + (face_size / 36);
        lv_obj_set_size(mouth, mouth_w, mouth_h);
        lv_obj_remove_style(mouth, NULL, LV_PART_KNOB);
        lv_obj_set_style_bg_opa(mouth, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(mouth, 0, LV_PART_MAIN);
        lv_obj_set_style_arc_width(mouth, 3, LV_PART_MAIN);
        lv_obj_set_style_arc_color(mouth, lv_color_hex(0x7CF4FF), LV_PART_MAIN);
        lv_obj_set_style_arc_rounded(mouth, true, LV_PART_MAIN);
        lv_arc_set_start_angle(mouth, 200);
        lv_arc_set_end_angle(mouth, 340);
        lv_obj_align(mouth, LV_ALIGN_CENTER, 0, mouth_offset_y);
    }
#endif
    {
        const int16_t max_offset = (int16_t)((face_size / 2) - (g_face.eye_w / 2) - eye_offset);
        int16_t limit = (int16_t)((face_size * LOOK_LIMIT_PCT) / 100);
        if (max_offset < limit) {
            limit = max_offset;
        }
        if (limit < 0) {
            limit = 0;
        }
        s_look_limit = limit;
    }
    s_look_offset = 0;
    s_face_ready = true;
}

static void face_blink(uint16_t ms)
{
    if (!s_face_ready) {
        return;
    }

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_time(&a, ms);
    lv_anim_set_playback_time(&a, ms);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);

    lv_anim_set_exec_cb(&a, set_eye_h);
    lv_anim_set_values(&a, g_face.eye_h, 3);
    lv_anim_set_var(&a, g_face.eye_l);
    lv_anim_start(&a);
    lv_anim_set_var(&a, g_face.eye_r);
    lv_anim_start(&a);

    lv_anim_init(&a);
    lv_anim_set_time(&a, ms);
    lv_anim_set_playback_time(&a, ms);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);

    lv_anim_set_exec_cb(&a, set_eye_y);
    lv_anim_set_values(&a, g_face.base_y, g_face.base_y + 10);
    lv_anim_set_var(&a, g_face.eye_l);
    lv_anim_start(&a);
    lv_anim_set_var(&a, g_face.eye_r);
    lv_anim_start(&a);
}

static void face_look(int16_t v, uint16_t ms)
{
    if (!s_face_ready) {
        return;
    }
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, NULL);
    lv_anim_set_exec_cb(&a, set_look);
    lv_anim_set_time(&a, ms);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_set_values(&a, s_look_offset, v);
    lv_anim_start(&a);
    s_look_offset = v;
}

static void look_timer_cb(lv_timer_t *timer)
{
    static int16_t target = 0;

    LV_UNUSED(timer);
    if (!s_face_ready) {
        return;
    }
    if (target == 0) {
        target = s_look_limit;
    }
    face_look(target, 220);
    face_blink(90);
    target = (int16_t)-target;
}

void ui_robot_screen_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_opa(parent, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_pad_all(parent, 0, LV_PART_MAIN);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    face_create(parent);
    if (!s_face_ready) {
        return;
    }
    face_look(0, 0);
    face_blink(90);

    if (s_look_limit > 0) {
        lv_timer_create(look_timer_cb, 1600, NULL);
    }
}
