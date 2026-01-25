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
// --- Advanced SDF Rendering Engine (Sub-pixel AA) ---

// Blend two colors with alpha (0-255)
static inline lv_color_t blend_color(lv_color_t bg, lv_color_t fg,
                                     uint8_t alpha) {
  if (alpha >= 255)
    return fg;
  if (alpha == 0)
    return bg;

  uint32_t val_r = ((uint32_t)LV_COLOR_GET_R(fg) * alpha +
                    (uint32_t)LV_COLOR_GET_R(bg) * (255 - alpha)) >>
                   8;
  uint32_t val_g = ((uint32_t)LV_COLOR_GET_G(fg) * alpha +
                    (uint32_t)LV_COLOR_GET_G(bg) * (255 - alpha)) >>
                   8;
  uint32_t val_b = ((uint32_t)LV_COLOR_GET_B(fg) * alpha +
                    (uint32_t)LV_COLOR_GET_B(bg) * (255 - alpha)) >>
                   8;

  lv_color_t res = LV_COLOR_MAKE(val_r, val_g, val_b);
  return res;
}

// SDF for Rounded Rectangle
static float sd_rect(float px, float py, float cx, float cy, float hw,
                     float hh) {
  float dx = fabsf(px - cx) - hw;
  float dy = fabsf(py - cy) - hh;
  float max_d = fmaxf(dx, dy);
  return max_d;
}

// SDF for Ellipse (Approximate)
static float sd_ellipse(float px, float py, float cx, float cy, float rx,
                        float ry) {
  if (rx < 0.1f || ry < 0.1f)
    return 1e10;
  float k0 = sqrtf((px - cx) * (px - cx) / (rx * rx) +
                   (py - cy) * (py - cy) / (ry * ry));
  return (k0 - 1.0f) * fminf(rx, ry);
}

// SDF for Line Segment
static float sd_line(float px, float py, float x1, float y1, float x2,
                     float y2) {
  float dx = x2 - x1;
  float dy = y2 - y1;
  float l2 = dx * dx + dy * dy;
  if (l2 < 0.1f)
    return sqrtf((px - x1) * (px - x1) + (py - y1) * (py - y1));
  float t = fmaxf(0, fminf(1, ((px - x1) * dx + (py - y1) * dy) / l2));
  float proj_x = x1 + t * dx;
  float proj_y = y1 + t * dy;
  return sqrtf((px - proj_x) * (px - proj_x) + (py - proj_y) * (py - proj_y));
}

static void draw_shape_sdf(lv_color_t *buf, int canvas_w, int canvas_h,
                           uint8_t type, float x, float y, float w, float h,
                           float rot, float x2, float y2, lv_color_t color,
                           uint8_t opa) {
  // Bounding box for optimization
  float cx = x + w / 2.0f;
  float cy = y + h / 2.0f;
  float max_dim = sqrtf(w * w + h * h);

  // If line, bounding box is different
  int x1_bound, y1_bound, x2_bound, y2_bound;
  if (type == 2) { // SHAPE_LINE
    x1_bound = (int)fminf(x, x2) - 2;
    y1_bound = (int)fminf(y, y2) - 2;
    x2_bound = (int)fmaxf(x, x2) + 2;
    y2_bound = (int)fmaxf(y, y2) + 2;
  } else {
    x1_bound = (int)(cx - max_dim / 2.0f) - 2;
    y1_bound = (int)(cy - max_dim / 2.0f) - 2;
    x2_bound = (int)(cx + max_dim / 2.0f) + 2;
    y2_bound = (int)(cy + max_dim / 2.0f) + 2;
  }

  // Clip to canvas
  if (x1_bound < 0)
    x1_bound = 0;
  if (y1_bound < 0)
    y1_bound = 0;
  if (x2_bound >= canvas_w)
    x2_bound = canvas_w - 1;
  if (y2_bound >= canvas_h)
    y2_bound = canvas_h - 1;

  float hw = w / 2.0f;
  float hh = h / 2.0f;

  // Anti-aliasing smoothness
  const float smoothness = 0.8f;

  for (int py = y1_bound; py <= y2_bound; py++) {
    int row_start = py * canvas_w;
    float fy = (float)py + 0.5f;
    for (int px = x1_bound; px <= x2_bound; px++) {
      float fx = (float)px + 0.5f;
      float d = 1e10;

      if (type == 0) { // SHAPE_RECT
        // Handle rotation if not zero
        if (rot != 0) {
          float rad = rot * M_PI / 180.0f;
          float cos_a = cosf(rad);
          float sin_a = sinf(rad);
          float dx = fx - cx;
          float dy = fy - cy;
          float rx = dx * cos_a + dy * sin_a;
          float ry = -dx * sin_a + dy * cos_a;
          d = sd_rect(rx, ry, 0, 0, hw, hh);
        } else {
          d = sd_rect(fx, fy, cx, cy, hw, hh);
        }
      } else if (type == 1) { // SHAPE_ELLIPSE
        d = sd_ellipse(fx, fy, cx, cy, hw, hh);
      } else if (type == 2) {                     // SHAPE_LINE
        d = sd_line(fx, fy, x, y, x2, y2) - 1.0f; // 1px half-width
      }

      if (d < smoothness) {
        float alpha_f = 0.5f - d / (2.0f * smoothness);
        if (alpha_f > 1.0f)
          alpha_f = 1.0f;
        if (alpha_f < 0.0f)
          alpha_f = 0.0f;

        uint16_t total_alpha = (uint16_t)(alpha_f * opa);
        if (total_alpha > 0) {
          buf[row_start + px] =
              blend_color(buf[row_start + px], color, (uint8_t)total_alpha);
        }
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

// Binary Loader for .rbat files
bool anim_manager_load_rbat(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return false;

  char magic[4];
  fread(magic, 1, 4, f);
  if (memcmp(magic, "RBAT", 4) != 0) {
    fclose(f);
    return false;
  }

  uint16_t version, width, height, frame_count;
  uint32_t reserved;
  fread(&version, 2, 1, f);
  fread(&width, 2, 1, f);
  fread(&height, 2, 1, f);
  fread(&frame_count, 2, 1, f);
  fread(&reserved, 4, 1, f);

  // Allocate anim_vector_t and cast away const to fill data
  anim_vector_t *vector = (anim_vector_t *)malloc(sizeof(anim_vector_t));
  ((char **)&vector->name)[0] = strdup(path);
  vector->frame_count = frame_count;
  // Use a non-const pointer to fill the data
  anim_vector_frame_t *write_frames =
      (anim_vector_frame_t *)malloc(sizeof(anim_vector_frame_t) * frame_count);
  ((anim_vector_frame_t **)&vector->frames)[0] = write_frames;

  for (uint16_t i = 0; i < frame_count; i++) {
    uint16_t duration, shape_count;
    fread(&duration, 2, 1, f);
    fread(&shape_count, 2, 1, f);

    write_frames[i].duration_ms = duration;
    write_frames[i].shape_count = shape_count;

    // Use non-const pointer to fill the shapes data
    anim_shape_t *write_shapes =
        (anim_shape_t *)malloc(sizeof(anim_shape_t) * shape_count);
    ((anim_shape_t **)&write_frames[i].shapes)[0] = write_shapes;

    for (uint16_t j = 0; j < shape_count; j++) {
      anim_shape_t *s = &write_shapes[j];
      uint8_t type;
      fread(&type, 1, 1, f);
      s->type = type;
      fread(&s->x, 4, 1, f);
      fread(&s->y, 4, 1, f);
      fread(&s->w, 4, 1, f);
      fread(&s->h, 4, 1, f);
      fread(&s->rotation, 4, 1, f);
      fread(&s->opacity, 4, 1, f);

      uint32_t color_hex;
      fread(&color_hex, 4, 1, f); // 0xRRGGBB
      s->color = color_hex;

      fread(&s->x2, 4, 1, f);
      fread(&s->y2, 4, 1, f);

      uint8_t font_size;
      uint16_t text_len;
      fread(&font_size, 1, 1, f);
      fread(&text_len, 2, 1, f);
      s->font_size = font_size;

      if (text_len > 0) {
        char *text = (char *)malloc(text_len + 1);
        fread(text, 1, text_len, f);
        text[text_len] = '\0';
        s->text = text;
      } else {
        s->text = NULL;
      }
    }
  }

  fclose(f);
  return anim_manager_register_vector(vector);
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

      if (s1->type == SHAPE_RECT || s1->type == SHAPE_ELLIPSE ||
          s1->type == SHAPE_LINE) {
        draw_shape_sdf(back_buffer, 466, 466, s1->type, x, y, w, h,
                       s1->rotation, s1->x2 + (s2->x2 - s1->x2) * t,
                       s1->y2 + (s2->y2 - s1->y2) * t, color, opa);
      } else if (s1->type == SHAPE_TEXT && s1->text != NULL) {
        // Text rendering via lv_canvas_draw_text
        lv_draw_label_dsc_t label_dsc;
        lv_draw_label_dsc_init(&label_dsc);
        label_dsc.color = color;
        label_dsc.opa = opa;
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
