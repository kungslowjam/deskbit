/**
 * @file ui_custom_anim.c
 * @brief Shape Animation Engine with Interpolation & Easing
 */

#include "ui_custom_anim.h"
#include "esp_heap_caps.h"
#include <math.h>
#include <stdlib.h>

#define CANVAS_WIDTH 466
#define CANVAS_HEIGHT 466
#define PROGRESS_MAX 1000 // 0-1000 = 0-100%

// Forward declarations
static void anim_exec_cb(void *var, int32_t value);
static void draw_frame(custom_anim_t *anim);
static void interpolate_and_draw(custom_anim_t *anim, float t);

custom_anim_t *ui_custom_anim_create(lv_obj_t *parent) {
  custom_anim_t *anim = (custom_anim_t *)malloc(sizeof(custom_anim_t));
  if (!anim)
    return NULL;

  anim->parent = parent;
  anim->keyframes = NULL;
  anim->shape_counts = NULL;
  anim->frame_count = 0;
  anim->current_progress = 0;
  anim->current_frame = 0;
  anim->next_frame = 0;
  anim->duration_ms = 1000;
  anim->easing = lv_anim_path_linear;
  anim->is_playing = false;
  anim->loop = false;

  // Create canvas
  size_t buf_size =
      LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA(CANVAS_WIDTH, CANVAS_HEIGHT);
  anim->canvas_buf = heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
  if (!anim->canvas_buf) {
    free(anim);
    return NULL;
  }

  anim->canvas = lv_canvas_create(parent);
  lv_canvas_set_buffer(anim->canvas, anim->canvas_buf, CANVAS_WIDTH,
                       CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR_ALPHA);
  lv_canvas_fill_bg(anim->canvas, lv_color_black(), LV_OPA_TRANSP);
  lv_obj_set_style_bg_opa(anim->canvas, LV_OPA_TRANSP, 0);
  lv_obj_align(anim->canvas, LV_ALIGN_CENTER, 0, 0);

  printf("[CustomAnim] Canvas created: %dx%d at %p\n", CANVAS_WIDTH,
         CANVAS_HEIGHT, anim->canvas);

  return anim;
}

void ui_custom_anim_set_shape_src(custom_anim_t *anim,
                                  const shape_keyframe_t **keyframes,
                                  const uint8_t *shape_counts,
                                  uint16_t frame_count, uint32_t duration_ms,
                                  lv_anim_path_cb_t easing) {
  if (!anim || !keyframes || !shape_counts || frame_count == 0) {
    printf("[CustomAnim] ERROR: set_shape_src failed - anim=%p, keyframes=%p, "
           "counts=%p, frames=%d\n",
           anim, keyframes, shape_counts, frame_count);
    return;
  }

  anim->keyframes = keyframes;
  anim->shape_counts = shape_counts;
  anim->frame_count = frame_count;
  anim->duration_ms = duration_ms;
  anim->easing = easing ? easing : lv_anim_path_linear;

  printf("[CustomAnim] Loaded %d frames: ", frame_count);
  for (int i = 0; i < frame_count; i++) {
    printf("f%d=%d shapes, ", i, shape_counts[i]);
  }
  printf("duration=%dms\n", duration_ms);

  // Draw initial frame
  anim->current_frame = 0;
  anim->next_frame = (frame_count > 1) ? 1 : 0;
  draw_frame(anim);
}

void ui_custom_anim_start(custom_anim_t *anim, uint16_t repeat_count) {
  if (!anim || !anim->keyframes) {
    printf("[CustomAnim] ERROR: Cannot start - anim=%p, keyframes=%p\n", anim,
           anim ? anim->keyframes : NULL);
    return;
  }

  printf("[CustomAnim] Starting animation: %d frames, %dms duration\n",
         anim->frame_count, anim->duration_ms);

  anim->loop = (repeat_count == LV_ANIM_REPEAT_INFINITE);
  anim->is_playing = true;

  // Show canvas
  lv_obj_clear_flag(anim->canvas, LV_OBJ_FLAG_HIDDEN);

  // Setup LVGL animation
  lv_anim_init(&anim->anim);
  lv_anim_set_var(&anim->anim, anim);
  lv_anim_set_values(&anim->anim, 0, PROGRESS_MAX);
  lv_anim_set_time(&anim->anim, anim->duration_ms);
  lv_anim_set_exec_cb(&anim->anim, anim_exec_cb);
  lv_anim_set_path_cb(&anim->anim, anim->easing);
  lv_anim_set_repeat_count(&anim->anim, repeat_count);
  lv_anim_start(&anim->anim);
}

void ui_custom_anim_stop(custom_anim_t *anim) {
  if (!anim)
    return;

  lv_anim_del(&anim->anim, NULL);
  lv_obj_add_flag(anim->canvas, LV_OBJ_FLAG_HIDDEN);
  anim->is_playing = false;
}

void ui_custom_anim_align(custom_anim_t *anim, lv_align_t align,
                          lv_coord_t x_ofs, lv_coord_t y_ofs) {
  if (!anim || !anim->canvas)
    return;
  lv_obj_align(anim->canvas, align, x_ofs, y_ofs);
}

void ui_custom_anim_delete(custom_anim_t *anim) {
  if (!anim)
    return;

  ui_custom_anim_stop(anim);
  if (anim->canvas)
    lv_obj_del(anim->canvas);
  if (anim->canvas_buf)
    free(anim->canvas_buf);
  free(anim);
}

bool ui_custom_anim_is_playing(custom_anim_t *anim) {
  return anim ? anim->is_playing : false;
}

// Animation callback - called by LVGL every frame
static void anim_exec_cb(void *var, int32_t value) {
  custom_anim_t *anim = (custom_anim_t *)var;
  if (!anim)
    return;

  anim->current_progress = value;

  // Calculate which keyframes to interpolate between
  if (anim->frame_count <= 1) {
    draw_frame(anim);
    return;
  }

  // Map progress (0-1000) to frame range
  float total_progress = (float)value / PROGRESS_MAX; // 0.0 - 1.0
  float frame_float = total_progress * (anim->frame_count - 1);

  anim->current_frame = (uint16_t)frame_float;
  anim->next_frame = (anim->current_frame + 1) % anim->frame_count;

  // Calculate interpolation factor (0.0 - 1.0 between current and next frame)
  float t = frame_float - anim->current_frame;

  // Interpolate and draw
  interpolate_and_draw(anim, t);
}

// Interpolate shapes between two keyframes and draw
static void interpolate_and_draw(custom_anim_t *anim, float t) {
  if (!anim || !anim->keyframes)
    return;

  printf("[CustomAnim] Drawing frame %d->%d, t=%.2f\n", anim->current_frame,
         anim->next_frame, t);

  // Clear canvas
  lv_canvas_fill_bg(anim->canvas, lv_color_black(), LV_OPA_TRANSP);

  const shape_keyframe_t *frame1 = anim->keyframes[anim->current_frame];
  const shape_keyframe_t *frame2 = anim->keyframes[anim->next_frame];
  uint8_t count1 = anim->shape_counts[anim->current_frame];
  uint8_t count2 = anim->shape_counts[anim->next_frame];

  // Use minimum count to avoid out-of-bounds
  uint8_t count = (count1 < count2) ? count1 : count2;

  printf("  Shapes: count1=%d, count2=%d, using=%d\n", count1, count2, count);

  for (uint8_t i = 0; i < count; i++) {
    const shape_keyframe_t *s1 = &frame1[i];
    const shape_keyframe_t *s2 = &frame2[i];

    // Interpolate properties
    int16_t x = s1->x + (int16_t)((s2->x - s1->x) * t);
    int16_t y = s1->y + (int16_t)((s2->y - s1->y) * t);
    int16_t w = s1->width + (int16_t)((s2->width - s1->width) * t);
    int16_t h = s1->height + (int16_t)((s2->height - s1->height) * t);

    // Use color from first frame
    lv_color_t color = lv_color_hex(s1->color);

    // Debug first shape only to avoid spam
    if (i == 0) {
      printf("  Shape[0]: x=%d, y=%d, w=%d, h=%d, color=0x%06x\n", x, y, w, h,
             s1->color);
    }

    if (s1->type == SHAPE_ELLIPSE) {
      // Draw ellipse by filling with circles (approximation)
      lv_draw_rect_dsc_t fill_dsc;
      lv_draw_rect_dsc_init(&fill_dsc);
      fill_dsc.bg_color = color;
      fill_dsc.bg_opa = LV_OPA_COVER;
      fill_dsc.border_width = 0;
      fill_dsc.radius = LV_RADIUS_CIRCLE;

      // Draw as rounded rect (circle if w==h)
      lv_canvas_draw_rect(anim->canvas, x, y, w, h, &fill_dsc);

    } else if (s1->type == SHAPE_RECT) {
      lv_draw_rect_dsc_t rect_dsc;
      lv_draw_rect_dsc_init(&rect_dsc);
      rect_dsc.bg_color = color;
      rect_dsc.bg_opa = LV_OPA_COVER;
      rect_dsc.border_width = 0;
      rect_dsc.radius = 0;

      lv_canvas_draw_rect(anim->canvas, x, y, w, h, &rect_dsc);
    }
  }

  // Force canvas redraw
  lv_obj_invalidate(anim->canvas);
}

// Draw single frame (no interpolation)
static void draw_frame(custom_anim_t *anim) {
  interpolate_and_draw(anim, 0.0f); // t=0 means use current_frame only
}

// Legacy API compatibility (uses static images, no interpolation)
void ui_custom_anim_set_src(custom_anim_t *anim, const lv_img_dsc_t **imgs,
                            uint16_t count, uint32_t frame_duration_ms) {
  // This legacy function is kept for compatibility but does nothing
  // Users should call ui_custom_anim_set_shape_src instead
  (void)anim;
  (void)imgs;
  (void)count;
  (void)frame_duration_ms;
}
