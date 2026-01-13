#include "ui_cat_screen.h"

static lv_coord_t min_coord(lv_coord_t a, lv_coord_t b)
{
    return (a < b) ? a : b;
}

static void style_clear(lv_obj_t *obj)
{
    if (obj == NULL) {
        return;
    }
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

void ui_cat_screen_create(lv_obj_t *parent)
{
    if (parent == NULL) {
        return;
    }

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(parent, 0, LV_PART_MAIN);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_update_layout(parent);
    lv_coord_t parent_w = lv_obj_get_width(parent);
    lv_coord_t parent_h = lv_obj_get_height(parent);
    if (parent_w <= 0 || parent_h <= 0) {
        lv_display_t *disp = lv_display_get_default();
        if (disp != NULL) {
            parent_w = lv_display_get_horizontal_resolution(disp);
            parent_h = lv_display_get_vertical_resolution(disp);
        }
    }
    if (parent_w <= 0 || parent_h <= 0) {
        parent_w = 466;
        parent_h = 466;
    }

    lv_coord_t size = min_coord(parent_w, parent_h);
    lv_coord_t ring_size = (size * 92) / 100;
    lv_coord_t visor_w = (ring_size * 74) / 100;
    lv_coord_t visor_h = (ring_size * 68) / 100;
    lv_coord_t ear_w = (ring_size * 26) / 100;
    lv_coord_t ear_h = (ring_size * 20) / 100;

    lv_obj_t *ear_l = lv_obj_create(parent);
    lv_obj_t *ear_r = lv_obj_create(parent);
    if (ear_l != NULL && ear_r != NULL) {
        lv_obj_set_size(ear_l, ear_w, ear_h);
        lv_obj_set_size(ear_r, ear_w, ear_h);
        lv_obj_set_style_radius(ear_l, ear_h / 2, LV_PART_MAIN);
        lv_obj_set_style_radius(ear_r, ear_h / 2, LV_PART_MAIN);
        lv_obj_set_style_bg_color(ear_l, lv_color_hex(0x1A6CFF), LV_PART_MAIN);
        lv_obj_set_style_bg_color(ear_r, lv_color_hex(0x1A6CFF), LV_PART_MAIN);
        lv_obj_set_style_bg_grad_color(ear_l, lv_color_hex(0x3CF2FF), LV_PART_MAIN);
        lv_obj_set_style_bg_grad_color(ear_r, lv_color_hex(0x3CF2FF), LV_PART_MAIN);
        lv_obj_set_style_bg_grad_dir(ear_l, LV_GRAD_DIR_VER, LV_PART_MAIN);
        lv_obj_set_style_bg_grad_dir(ear_r, LV_GRAD_DIR_VER, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(ear_l, ear_h / 3, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(ear_r, ear_h / 3, LV_PART_MAIN);
        lv_obj_set_style_shadow_color(ear_l, lv_color_hex(0x26E6FF), LV_PART_MAIN);
        lv_obj_set_style_shadow_color(ear_r, lv_color_hex(0x26E6FF), LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(ear_l, LV_OPA_40, LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(ear_r, LV_OPA_40, LV_PART_MAIN);
        style_clear(ear_l);
        style_clear(ear_r);
        lv_obj_align(ear_l, LV_ALIGN_CENTER, -(ring_size / 3), -(ring_size / 2) + ear_h / 4);
        lv_obj_align(ear_r, LV_ALIGN_CENTER, ring_size / 3, -(ring_size / 2) + ear_h / 4);
    }

    lv_obj_t *ring = lv_obj_create(parent);
    if (ring == NULL) {
        return;
    }
    lv_obj_set_size(ring, ring_size, ring_size);
    lv_obj_align(ring, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ring, lv_color_hex(0x0B1020), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(ring, lv_color_hex(0x111B2F), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(ring, LV_GRAD_DIR_VER, LV_PART_MAIN);
    lv_obj_set_style_border_width(ring, 3, LV_PART_MAIN);
    lv_obj_set_style_border_color(ring, lv_color_hex(0x2E3B5A), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(ring, ring_size / 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(ring, lv_color_hex(0x20CFFF), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(ring, LV_OPA_30, LV_PART_MAIN);
    lv_obj_clear_flag(ring, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *visor = lv_obj_create(ring);
    if (visor == NULL) {
        return;
    }
    lv_obj_set_size(visor, visor_w, visor_h);
    lv_obj_align(visor, LV_ALIGN_CENTER, 0, ring_size / 30);
    lv_obj_set_style_bg_color(visor, lv_color_hex(0x05070B), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(visor, lv_color_hex(0x0B0F19), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(visor, LV_GRAD_DIR_VER, LV_PART_MAIN);
    lv_obj_set_style_radius(visor, visor_h / 4, LV_PART_MAIN);
    lv_obj_set_style_border_width(visor, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(visor, lv_color_hex(0x26314A), LV_PART_MAIN);
    lv_obj_set_style_clip_corner(visor, true, LV_PART_MAIN);
    lv_obj_clear_flag(visor, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *bar_l = lv_obj_create(ring);
    lv_obj_t *bar_r = lv_obj_create(ring);
    if (bar_l != NULL && bar_r != NULL) {
        lv_coord_t bar_w = ring_size / 20;
        lv_coord_t bar_h = ring_size / 4;
        lv_obj_set_size(bar_l, bar_w, bar_h);
        lv_obj_set_size(bar_r, bar_w, bar_h);
        lv_obj_set_style_radius(bar_l, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_radius(bar_r, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(bar_l, lv_color_hex(0x29F0FF), LV_PART_MAIN);
        lv_obj_set_style_bg_color(bar_r, lv_color_hex(0x29F0FF), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(bar_l, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(bar_r, LV_OPA_COVER, LV_PART_MAIN);
        style_clear(bar_l);
        style_clear(bar_r);
        lv_obj_align(bar_l, LV_ALIGN_LEFT_MID, ring_size / 18, 0);
        lv_obj_align(bar_r, LV_ALIGN_RIGHT_MID, -ring_size / 18, 0);
    }

    lv_coord_t eye_w = visor_w / 5;
    lv_coord_t eye_h = (visor_h * 35) / 100;
    lv_coord_t eye_y = -(visor_h / 10);
    lv_obj_t *eye_l = lv_obj_create(visor);
    lv_obj_t *eye_r = lv_obj_create(visor);
    if (eye_l != NULL && eye_r != NULL) {
        lv_obj_set_size(eye_l, eye_w, eye_h);
        lv_obj_set_size(eye_r, eye_w, eye_h);
        lv_obj_set_style_radius(eye_l, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_radius(eye_r, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(eye_l, lv_color_hex(0x42E8FF), LV_PART_MAIN);
        lv_obj_set_style_bg_color(eye_r, lv_color_hex(0x42E8FF), LV_PART_MAIN);
        lv_obj_set_style_bg_grad_color(eye_l, lv_color_hex(0xA2F6FF), LV_PART_MAIN);
        lv_obj_set_style_bg_grad_color(eye_r, lv_color_hex(0xA2F6FF), LV_PART_MAIN);
        lv_obj_set_style_bg_grad_dir(eye_l, LV_GRAD_DIR_VER, LV_PART_MAIN);
        lv_obj_set_style_bg_grad_dir(eye_r, LV_GRAD_DIR_VER, LV_PART_MAIN);
        style_clear(eye_l);
        style_clear(eye_r);
        lv_obj_align(eye_l, LV_ALIGN_CENTER, -(visor_w / 5), eye_y);
        lv_obj_align(eye_r, LV_ALIGN_CENTER, visor_w / 5, eye_y);

        lv_obj_t *shine_l = lv_obj_create(eye_l);
        lv_obj_t *shine_r = lv_obj_create(eye_r);
        if (shine_l != NULL && shine_r != NULL) {
            lv_coord_t shine = eye_w / 4;
            lv_obj_set_size(shine_l, shine, shine);
            lv_obj_set_size(shine_r, shine, shine);
            lv_obj_set_style_radius(shine_l, LV_RADIUS_CIRCLE, LV_PART_MAIN);
            lv_obj_set_style_radius(shine_r, LV_RADIUS_CIRCLE, LV_PART_MAIN);
            lv_obj_set_style_bg_color(shine_l, lv_color_hex(0xE9FDFF), LV_PART_MAIN);
            lv_obj_set_style_bg_color(shine_r, lv_color_hex(0xE9FDFF), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(shine_l, LV_OPA_70, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(shine_r, LV_OPA_70, LV_PART_MAIN);
            style_clear(shine_l);
            style_clear(shine_r);
            lv_obj_align(shine_l, LV_ALIGN_TOP_LEFT, shine / 3, shine / 3);
            lv_obj_align(shine_r, LV_ALIGN_TOP_LEFT, shine / 3, shine / 3);
        }
    }

    lv_obj_t *brow_l = lv_obj_create(visor);
    lv_obj_t *brow_r = lv_obj_create(visor);
    if (brow_l != NULL && brow_r != NULL) {
        lv_coord_t brow_w = visor_w / 7;
        lv_coord_t brow_h = visor_h / 20;
        lv_obj_set_size(brow_l, brow_w, brow_h);
        lv_obj_set_size(brow_r, brow_w, brow_h);
        lv_obj_set_style_radius(brow_l, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_radius(brow_r, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(brow_l, lv_color_hex(0x6FE7FF), LV_PART_MAIN);
        lv_obj_set_style_bg_color(brow_r, lv_color_hex(0x6FE7FF), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(brow_l, LV_OPA_80, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(brow_r, LV_OPA_80, LV_PART_MAIN);
        style_clear(brow_l);
        style_clear(brow_r);
        lv_obj_align(brow_l, LV_ALIGN_CENTER, -(visor_w / 5), eye_y - brow_h * 3);
        lv_obj_align(brow_r, LV_ALIGN_CENTER, visor_w / 5, eye_y - brow_h * 3);
    }

    lv_obj_t *cheek_l = lv_obj_create(visor);
    lv_obj_t *cheek_r = lv_obj_create(visor);
    if (cheek_l != NULL && cheek_r != NULL) {
        lv_coord_t cheek = visor_w / 6;
        lv_obj_set_size(cheek_l, cheek, cheek);
        lv_obj_set_size(cheek_r, cheek, cheek);
        lv_obj_set_style_radius(cheek_l, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_radius(cheek_r, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(cheek_l, lv_color_hex(0xFF6FAE), LV_PART_MAIN);
        lv_obj_set_style_bg_color(cheek_r, lv_color_hex(0xFF6FAE), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(cheek_l, LV_OPA_90, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(cheek_r, LV_OPA_90, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(cheek_l, cheek / 2, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(cheek_r, cheek / 2, LV_PART_MAIN);
        lv_obj_set_style_shadow_color(cheek_l, lv_color_hex(0xFF6FAE), LV_PART_MAIN);
        lv_obj_set_style_shadow_color(cheek_r, lv_color_hex(0xFF6FAE), LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(cheek_l, LV_OPA_40, LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(cheek_r, LV_OPA_40, LV_PART_MAIN);
        style_clear(cheek_l);
        style_clear(cheek_r);
        lv_obj_align(cheek_l, LV_ALIGN_CENTER, -(visor_w / 4), visor_h / 6);
        lv_obj_align(cheek_r, LV_ALIGN_CENTER, visor_w / 4, visor_h / 6);
    }

    lv_obj_t *nose = lv_obj_create(visor);
    if (nose != NULL) {
        lv_coord_t nose_w = visor_w / 10;
        lv_coord_t nose_h = visor_h / 18;
        lv_obj_set_size(nose, nose_w, nose_h);
        lv_obj_set_style_radius(nose, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(nose, lv_color_hex(0xF4E57A), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(nose, LV_OPA_COVER, LV_PART_MAIN);
        style_clear(nose);
        lv_obj_align(nose, LV_ALIGN_CENTER, 0, visor_h / 20);
    }

    lv_obj_t *mouth = lv_line_create(visor);
    if (mouth != NULL) {
        lv_coord_t mouth_w = visor_w / 4;
        lv_coord_t mouth_h = visor_h / 10;
        static lv_point_precise_t mouth_pts[9];
        mouth_pts[0].x = 0;
        mouth_pts[0].y = mouth_h / 2;
        mouth_pts[1].x = mouth_w / 8;
        mouth_pts[1].y = mouth_h / 6;
        mouth_pts[2].x = mouth_w / 4;
        mouth_pts[2].y = mouth_h * 4 / 5;
        mouth_pts[3].x = (mouth_w * 3) / 8;
        mouth_pts[3].y = mouth_h / 3;
        mouth_pts[4].x = mouth_w / 2;
        mouth_pts[4].y = mouth_h * 3 / 5;
        mouth_pts[5].x = (mouth_w * 5) / 8;
        mouth_pts[5].y = mouth_h / 3;
        mouth_pts[6].x = (mouth_w * 3) / 4;
        mouth_pts[6].y = mouth_h * 4 / 5;
        mouth_pts[7].x = (mouth_w * 7) / 8;
        mouth_pts[7].y = mouth_h / 6;
        mouth_pts[8].x = mouth_w;
        mouth_pts[8].y = mouth_h / 2;
        lv_line_set_points(mouth, mouth_pts, 9);
        lv_obj_set_style_line_width(mouth, 4, LV_PART_MAIN);
        lv_obj_set_style_line_color(mouth, lv_color_hex(0x6FE7FF), LV_PART_MAIN);
        lv_obj_set_style_line_rounded(mouth, true, LV_PART_MAIN);
        lv_obj_align(mouth, LV_ALIGN_CENTER, 0, visor_h / 5);
    }
}
