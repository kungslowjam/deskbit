/**
 * @file anim_manager.c
 * @brief Animation Manager Implementation
 */

#include "anim_manager.h"
#include "esp_heap_caps.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
// Helper: Draw filled ellipse - per-pixel check (guaranteed no dropout)
static void draw_ellipse_filled(lv_color_t *buf, int canvas_w, int canvas_h,
                                float x, float y, float w, float h,
                                lv_color_t color, uint8_t opa) {
  float cx = x + w / 2.0f;
  float cy = y + h / 2.0f;
  float rx = w / 2.0f;
  float ry = h / 2.0f;

  if (rx < 1 || ry < 1)
    return;

  // Bounding box with 1 pixel padding
  int x1 = (int)floorf(x) - 1;
  int y1 = (int)floorf(y) - 1;
  int x2 = (int)ceilf(x + w) + 1;
  int y2 = (int)ceilf(y + h) + 1;

  if (x1 < 0)
    x1 = 0;
  if (y1 < 0)
    y1 = 0;
  if (x2 >= canvas_w)
    x2 = canvas_w - 1;
  if (y2 >= canvas_h)
    y2 = canvas_h - 1;

  float rx2 = rx * rx;
  float ry2 = ry * ry;

  // Check every pixel in bounding box
  for (int py = y1; py <= y2; py++) {
    float dy = (float)py + 0.5f - cy; // Use pixel center
    float dy2_norm = (dy * dy) / ry2;

    if (dy2_norm > 1.0f)
      continue; // Skip entire row if outside

    int row_start = py * canvas_w;

    for (int px = x1; px <= x2; px++) {
      float dx = (float)px + 0.5f - cx; // Use pixel center
      float dx2_norm = (dx * dx) / rx2;

      // Check if pixel center is inside ellipse
      if (dx2_norm + dy2_norm <= 1.0f) {
        buf[row_start + px] = color;
      }
    }
  }
}

// Maximum number of registered animations
#define MAX_ANIMATIONS 20

// Animation registry
static anim_info_t anim_registry[MAX_ANIMATIONS];
static uint8_t anim_count = 0;

// Current state
static lv_obj_t *parent_obj = NULL;
static lv_obj_t *current_animimg = NULL;
static const char *current_anim_name = NULL;
static bool is_playing = false;
static void (*finish_callback)(const char *) = NULL;

// Vector playback state
static lv_obj_t *vector_canvas = NULL;
static lv_color_t *canvas_buffer = NULL;
static lv_color_t *back_buffer =
    NULL; // Double buffering - back buffer for drawing
static uint32_t vector_start_time = 0;
static uint16_t current_vector_frame = 0;
static uint16_t vector_loop_count = 0;
static anim_vector_t *current_vector_data = NULL;

// ========================================
// Public Functions
// ========================================

void anim_manager_init(lv_obj_t *parent) {
  parent_obj = parent;
  anim_count = 0;
  current_animimg = NULL;
  current_anim_name = NULL;
  is_playing = false;
  finish_callback = NULL;

  printf("[AnimMgr] Initialized\n");
}

bool anim_manager_register(const char *name, const lv_img_dsc_t **frames,
                           uint8_t frame_count, uint16_t duration_ms) {
  if (anim_count >= MAX_ANIMATIONS) {
    printf("[AnimMgr] ERROR: Registry full! Max %d animations\n",
           MAX_ANIMATIONS);
    return false;
  }

  if (!name || !frames || frame_count == 0) {
    printf("[AnimMgr] ERROR: Invalid parameters\n");
    return false;
  }

  // Check for duplicate
  for (uint8_t i = 0; i < anim_count; i++) {
    if (strcmp(anim_registry[i].name, name) == 0) {
      printf(
          "[AnimMgr] WARNING: Animation '%s' already registered, overwriting\n",
          name);
      anim_registry[i].frames = frames;
      anim_registry[i].frame_count = frame_count;
      anim_registry[i].default_duration_ms = duration_ms;
      return true;
    }
  }

  // Register new animation
  anim_registry[anim_count].name = name;
  anim_registry[anim_count].frames = frames;
  anim_registry[anim_count].frame_count = frame_count;
  anim_registry[anim_count].default_duration_ms = duration_ms;
  anim_count++;

  printf("[AnimMgr] Registered '%s' (%d frames, %dms)\n", name, frame_count,
         duration_ms);

  return true;
}

bool anim_manager_register_vector(const anim_vector_t *vector) {
  if (anim_count >= MAX_ANIMATIONS)
    return false;
  if (!vector || !vector->name)
    return false;

  // Check for duplicate
  for (uint8_t i = 0; i < anim_count; i++) {
    if (strcmp(anim_registry[i].name, vector->name) == 0) {
      anim_registry[i].vector = vector;
      anim_registry[i].is_vector = true;
      anim_registry[i].frame_count = vector->frame_count;
      return true;
    }
  }

  anim_registry[anim_count].name = vector->name;
  anim_registry[anim_count].vector = vector;
  anim_registry[anim_count].is_vector = true;
  anim_registry[anim_count].frame_count = vector->frame_count;
  anim_count++;

  printf("[AnimMgr] Registered Vector '%s' (%d frames)\n", vector->name,
         vector->frame_count);
  return true;
}

bool anim_manager_play(const char *name, uint16_t loop) {
  if (!parent_obj) {
    printf(
        "[AnimMgr] ERROR: Not initialized! Call anim_manager_init() first\n");
    return false;
  }

  // Find animation
  anim_info_t *anim = NULL;
  for (uint8_t i = 0; i < anim_count; i++) {
    if (strcmp(anim_registry[i].name, name) == 0) {
      anim = &anim_registry[i];
      break;
    }
  }

  if (!anim) {
    printf("[AnimMgr] ERROR: Animation '%s' not found\n", name);
    return false;
  }

  // Stop current
  anim_manager_stop();

  if (anim->is_vector) {
    // Setup Vector Playback with Double Buffering
    size_t buf_size = 466 * 466 * sizeof(lv_color_t);

    if (!canvas_buffer) {
      canvas_buffer = (lv_color_t *)heap_caps_malloc(
          buf_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
      if (!canvas_buffer) {
        canvas_buffer = (lv_color_t *)malloc(buf_size);
      }
    }

    if (!back_buffer) {
      back_buffer = (lv_color_t *)heap_caps_malloc(
          buf_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
      if (!back_buffer) {
        back_buffer = (lv_color_t *)malloc(buf_size);
      }
    }

    if (!vector_canvas) {
      vector_canvas = lv_canvas_create(parent_obj);
      lv_canvas_set_buffer(vector_canvas, canvas_buffer, 466, 466,
                           LV_IMG_CF_TRUE_COLOR);
    }

    current_vector_data = (anim_vector_t *)anim->vector;
    current_vector_frame = 0;
    vector_loop_count = loop;
    vector_start_time = lv_tick_get();
    is_playing = true;
    current_anim_name = name;

    lv_obj_clear_flag(vector_canvas, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(vector_canvas);

    printf("[AnimMgr] Playing Vector '%s'\n", name);
    return true;
  } else {
    // Create animation image (Existing BITMAP logic)
    current_animimg = lv_animimg_create(parent_obj);
    if (!current_animimg)
      return false;

    lv_animimg_set_src(current_animimg, (const void **)anim->frames,
                       anim->frame_count);
    lv_animimg_set_duration(current_animimg, anim->default_duration_ms);

    if (loop == 0)
      lv_animimg_set_repeat_count(current_animimg, LV_ANIM_REPEAT_INFINITE);
    else
      lv_animimg_set_repeat_count(current_animimg, loop);

    lv_obj_align(current_animimg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(current_animimg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(current_animimg);
    lv_animimg_start(current_animimg);

    current_anim_name = name;
    is_playing = true;
    return true;
  }
}

void anim_manager_stop(void) {
  if (current_animimg) {
    lv_obj_del(current_animimg);
    current_animimg = NULL;
  }

  if (vector_canvas) {
    lv_obj_add_flag(vector_canvas, LV_OBJ_FLAG_HIDDEN);
  }

  if (is_playing) {
    printf("[AnimMgr] Stopped '%s'\n", current_anim_name);
  }

  current_anim_name = NULL;
  is_playing = false;
  current_vector_data = NULL;
}

bool anim_manager_is_playing(void) { return is_playing; }

const char *anim_manager_get_current(void) { return current_anim_name; }

void anim_manager_update(void) {
  if (!is_playing)
    return;

  if (current_vector_data && vector_canvas) {
    uint32_t elapsed = lv_tick_get() - vector_start_time;
    uint32_t total_duration = 0;
    for (int i = 0; i < current_vector_data->frame_count; i++)
      total_duration += current_vector_data->frames[i].duration_ms;

    if (total_duration == 0)
      return;

    if (elapsed >= total_duration) {
      if (vector_loop_count == 1 ||
          (vector_loop_count > 1 && --vector_loop_count == 0)) {
        anim_manager_stop();
        if (finish_callback)
          finish_callback(current_anim_name);
        return;
      }
      vector_start_time = lv_tick_get();
      elapsed = 0;
    }

    // Find current frame and calculate interpolation factor
    uint32_t time_acc = 0;
    uint16_t found_frame = 0;
    uint32_t local_time = 0;

    for (int i = 0; i < current_vector_data->frame_count; i++) {
      if (elapsed >= time_acc &&
          elapsed < time_acc + current_vector_data->frames[i].duration_ms) {
        found_frame = i;
        local_time = elapsed - time_acc;
        break;
      }
      time_acc += current_vector_data->frames[i].duration_ms;
    }

    // Calculate interpolation factor (0.0 to 1.0)
    uint32_t frame_dur = current_vector_data->frames[found_frame].duration_ms;
    float t = (frame_dur > 0) ? (float)local_time / (float)frame_dur : 0.0f;
    if (t > 1.0f)
      t = 1.0f;

    // Get current and next frame
    const anim_vector_frame_t *cur_frame =
        &current_vector_data->frames[found_frame];
    uint16_t next_idx = (found_frame + 1) % current_vector_data->frame_count;
    const anim_vector_frame_t *next_frame =
        &current_vector_data->frames[next_idx];

    // Always redraw for smooth interpolation
    current_vector_frame = found_frame;

    // 1. Clear BACK buffer (draw to back buffer for double buffering)
    memset(back_buffer, 0, 466 * 466 * sizeof(lv_color_t));

    // 2. Draw Shapes with interpolation to BACK buffer
    for (int i = 0; i < cur_frame->shape_count; i++) {
      const anim_shape_t *s1 = &cur_frame->shapes[i];

      // Find matching shape in next frame (same index for now)
      const anim_shape_t *s2 =
          (i < next_frame->shape_count) ? &next_frame->shapes[i] : s1;

      // Interpolate properties
      float x = s1->x + (s2->x - s1->x) * t;
      float y = s1->y + (s2->y - s1->y) * t;
      float w = s1->w + (s2->w - s1->w) * t;
      float h = s1->h + (s2->h - s1->h) * t;
      float opa_f = s1->opacity + (s2->opacity - s1->opacity) * t;

      lv_color_t color = lv_color_hex(s1->color);
      uint8_t opa = (uint8_t)(opa_f * 255);

      if (s1->type == SHAPE_ELLIPSE) {
        draw_ellipse_filled(back_buffer, 466, 466, x, y, w, h, color, opa);
      } else if (s1->type == SHAPE_RECT) {
        // Draw rect to back_buffer directly
        int rx1 = (int)x, ry1 = (int)y;
        int rx2 = (int)(x + w), ry2 = (int)(y + h);
        if (rx1 < 0)
          rx1 = 0;
        if (ry1 < 0)
          ry1 = 0;
        if (rx2 >= 466)
          rx2 = 465;
        if (ry2 >= 466)
          ry2 = 465;
        for (int py = ry1; py <= ry2; py++) {
          for (int px = rx1; px <= rx2; px++) {
            back_buffer[py * 466 + px] = color;
          }
        }
      } else if (s1->type == SHAPE_LINE) {
        float x2 = s1->x2 + (s2->x2 - s1->x2) * t;
        float y2 = s1->y2 + (s2->y2 - s1->y2) * t;

        lv_draw_line_dsc_t dsc;
        lv_draw_line_dsc_init(&dsc);
        dsc.color = color;
        dsc.opa = opa;
        dsc.width = 2;
        lv_point_t points[2] = {{(lv_coord_t)x, (lv_coord_t)y},
                                {(lv_coord_t)x2, (lv_coord_t)y2}};
        lv_canvas_draw_line(vector_canvas, points, 2, &dsc);
      } else if (s1->type == SHAPE_TEXT && s1->text != NULL) {
        // Text rendering via lv_canvas_draw_text
        lv_draw_label_dsc_t label_dsc;
        lv_draw_label_dsc_init(&label_dsc);
        label_dsc.color = color;
        label_dsc.opa = opa;

        // Use default font (enable more in lv_conf.h if needed)
        label_dsc.font = &lv_font_montserrat_14;

        lv_canvas_draw_text(vector_canvas, (lv_coord_t)x, (lv_coord_t)y,
                            (lv_coord_t)w, &label_dsc, s1->text);
      }
    }

    // 3. DOUBLE BUFFER SWAP: Copy back buffer to front buffer atomically
    memcpy(canvas_buffer, back_buffer, 466 * 466 * sizeof(lv_color_t));

    // Force canvas refresh for smooth display
    lv_obj_invalidate(vector_canvas);
    return;
  }

  // Check if bitmap animation finished
  if (current_animimg) {
    if (!lv_obj_is_valid(current_animimg)) {
      const char *finished_name = current_anim_name;
      anim_manager_stop();
      if (finish_callback)
        finish_callback(finished_name);
    }
  }
}

void anim_manager_set_finish_callback(void (*callback)(const char *anim_name)) {
  finish_callback = callback;
}

// ========================================
// Helper: List all registered animations
// ========================================

void anim_manager_list_all(void) {
  printf("[AnimMgr] Registered animations (%d):\n", anim_count);
  for (uint8_t i = 0; i < anim_count; i++) {
    printf("  %d. '%s' - %d frames, %dms\n", i + 1, anim_registry[i].name,
           anim_registry[i].frame_count, anim_registry[i].default_duration_ms);
  }
}
