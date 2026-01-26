/**
 * @file anim_manager.c
 * @brief Pro Animation Manager using LVGL Object Pool (Zero Canvas/PSRAM)
 */

#include "anim_manager.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// Object Pool Selection (Increased for complexity)
#define POOL_SIZE_OBJ 32
#define POOL_SIZE_LINE 16
#define POOL_SIZE_TEXT 12

typedef struct {
  lv_obj_t *obj;
  bool in_use;
  uint32_t color;
  uint8_t opa;
  uint8_t type; // 0: RECT, 1: ELLIPSE
} pool_item_t;

// Custom Ellipse Drawing Event (Optimized with Horizontal Strips)
static void ellipse_draw_event_cb(lv_event_t *e) {
  lv_obj_t *obj = lv_event_get_target(e);
  lv_draw_ctx_t *draw_ctx = lv_event_get_draw_ctx(e);
  pool_item_t *item = (pool_item_t *)lv_event_get_user_data(e);

  if (!item || item->type != 1)
    return;

  lv_area_t area;
  lv_obj_get_coords(obj, &area);

  lv_coord_t w = lv_area_get_width(&area);
  lv_coord_t h = lv_area_get_height(&area);
  if (w <= 0 || h <= 0)
    return;

  float rx = w / 2.0f;
  float ry = h / 2.0f;
  float cx = area.x1 + rx;
  float cy = area.y1 + ry;

  int16_t angle_raw = lv_obj_get_style_transform_angle(obj, 0);

  lv_draw_rect_dsc_t draw_dsc;
  lv_draw_rect_dsc_init(&draw_dsc);
  draw_dsc.bg_color = lv_color_hex(item->color);

  // OPTIMIZATION: If NOT rotated, draw horizontal lines (much faster)
  if (angle_raw == 0) {
    draw_dsc.bg_opa = item->opa;
    for (lv_coord_t y = area.y1; y <= area.y2; y++) {
      float dy = (y + 0.5f) - cy;
      float dy_ry = dy / ry;
      float s = 1.0f - (dy_ry * dy_ry);
      if (s < 0)
        continue;
      float dx = rx * sqrtf(s);

      lv_area_t line_area;
      line_area.y1 = y;
      line_area.y2 = y;
      line_area.x1 = (lv_coord_t)(cx - dx);
      line_area.x2 = (lv_coord_t)(cx + dx);
      lv_draw_rect(draw_ctx, &draw_dsc, &line_area);
    }
    return;
  }

  // Rotated case: Use strips of pixels if possible, or per-pixel for accuracy
  float angle = angle_raw / 10.0f;
  float rad = -angle * M_PI / 180.0f;
  float cos_a = cosf(rad);
  float sin_a = sinf(rad);

  for (lv_coord_t y = area.y1; y <= area.y2; y++) {
    lv_coord_t start_x = -1;
    for (lv_coord_t x = area.x1; x <= area.x2 + 1; x++) {
      bool inside = false;
      if (x <= area.x2) {
        float dx = (x + 0.5f) - cx;
        float dy = (y + 0.5f) - cy;
        float nx = dx * cos_a - dy * sin_a;
        float ny = dx * sin_a + dy * cos_a;
        float d = (nx * nx) / (rx * rx) + (ny * ny) / (ry * ry);
        if (d <= 1.0f)
          inside = true;
      }

      if (inside && start_x == -1) {
        start_x = x;
      } else if (!inside && start_x != -1) {
        // Draw the strip we found
        draw_dsc.bg_opa = item->opa;
        lv_area_t strip;
        strip.y1 = y;
        strip.y2 = y;
        strip.x1 = start_x;
        strip.x2 = x - 1;
        lv_draw_rect(draw_ctx, &draw_dsc, &strip);
        start_x = -1;
      }
    }
  }
}

// Maximum animations in registry
#define MAX_ANIMATIONS 20

// Internal State
static anim_info_t anim_registry[MAX_ANIMATIONS];
static uint8_t anim_count = 0;
static lv_obj_t *parent_obj = NULL;
static lv_obj_t *current_animimg = NULL;
static const char *current_anim_name = NULL;
static bool is_playing = false;
static void (*finish_callback)(const char *) = NULL;

static pool_item_t obj_pool[POOL_SIZE_OBJ];
static pool_item_t line_pool[POOL_SIZE_LINE];
static pool_item_t text_pool[POOL_SIZE_TEXT];

static uint32_t vector_start_time = 0;
static uint16_t vector_loop_count = 0;
static anim_vector_t *current_vector_data = NULL;

// Pool Management
static void init_pools(lv_obj_t *parent) {
  for (int i = 0; i < POOL_SIZE_OBJ; i++) {
    obj_pool[i].obj = lv_obj_create(parent);
    lv_obj_add_flag(obj_pool[i].obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_scrollbar_mode(obj_pool[i].obj, LV_SCROLLBAR_MODE_OFF);
    // Standardize styles to match pixel-perfect drawing
    lv_obj_set_style_border_width(obj_pool[i].obj, 0, 0);
    lv_obj_set_style_outline_width(obj_pool[i].obj, 0, 0);
    lv_obj_set_style_pad_all(obj_pool[i].obj, 0, 0);
    lv_obj_set_style_shadow_width(obj_pool[i].obj, 0, 0);
    // Bind event for custom ellipse drawing
    lv_obj_add_event_cb(obj_pool[i].obj, ellipse_draw_event_cb,
                        LV_EVENT_DRAW_MAIN, &obj_pool[i]);
    obj_pool[i].in_use = false;
  }
  for (int i = 0; i < POOL_SIZE_LINE; i++) {
    line_pool[i].obj = lv_line_create(parent);
    lv_obj_add_flag(line_pool[i].obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_line_rounded(line_pool[i].obj, true, 0);
    line_pool[i].in_use = false;
  }
  for (int i = 0; i < POOL_SIZE_TEXT; i++) {
    text_pool[i].obj = lv_label_create(parent);
    lv_obj_add_flag(text_pool[i].obj, LV_OBJ_FLAG_HIDDEN);
    text_pool[i].in_use = false;
  }
}

// Easing Helper
static float apply_easing(float t, uint8_t type) {
  if (t <= 0.0f)
    return 0.0f;
  if (t >= 1.0f)
    return 1.0f;

  switch (type) {
  case 1:
    return t * t; // EaseIn
  case 2:
    return t * (2.0f - t); // EaseOut
  case 3:
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t; // EaseInOut
  case 4: { // Overshoot (Back)
    const float s = 1.70158f;
    return (t - 1.0f) * (t - 1.0f) * ((s + 1.0f) * (t - 1.0f) + s) + 1.0f;
  }
  case 5: { // Bounce Out
    if (t < (1 / 2.75f))
      return 7.5625f * t * t;
    else if (t < (2 / 2.75f)) {
      t -= (1.5f / 2.75f);
      return 7.5625f * t * t + 0.75f;
    } else if (t < (2.5f / 2.75f)) {
      t -= (2.25f / 2.75f);
      return 7.5625f * t * t + 0.9375f;
    } else {
      t -= (2.625f / 2.75f);
      return 7.5625f * t * t + 0.984375f;
    }
  }
  default:
    return t; // Linear
  }
}

static void reset_pools(void) {
  for (int i = 0; i < POOL_SIZE_OBJ; i++) {
    lv_obj_add_flag(obj_pool[i].obj, LV_OBJ_FLAG_HIDDEN);
    obj_pool[i].in_use = false;
  }
  for (int i = 0; i < POOL_SIZE_LINE; i++) {
    lv_obj_add_flag(line_pool[i].obj, LV_OBJ_FLAG_HIDDEN);
    line_pool[i].in_use = false;
  }
  for (int i = 0; i < POOL_SIZE_TEXT; i++) {
    lv_obj_add_flag(text_pool[i].obj, LV_OBJ_FLAG_HIDDEN);
    text_pool[i].in_use = false;
  }
}

// Public API
void anim_manager_init(lv_obj_t *parent) {
  parent_obj = parent;
  anim_count = 0;
  current_animimg = NULL;
  current_anim_name = NULL;
  is_playing = false;
  finish_callback = NULL;
  init_pools(parent);
  printf("[AnimMgr] Pro System Initialized (Object Pool Mode)\n");
}

bool anim_manager_register(const char *name, const lv_img_dsc_t **frames,
                           uint8_t frame_count, uint16_t duration_ms) {
  if (anim_count >= MAX_ANIMATIONS)
    return false;
  for (uint8_t i = 0; i < anim_count; i++) {
    if (strcmp(anim_registry[i].name, name) == 0) {
      anim_registry[i].frames = frames;
      anim_registry[i].frame_count = frame_count;
      anim_registry[i].default_duration_ms = duration_ms;
      anim_registry[i].is_vector = false;
      return true;
    }
  }
  anim_registry[anim_count].name = name;
  anim_registry[anim_count].frames = frames;
  anim_registry[anim_count].frame_count = frame_count;
  anim_registry[anim_count].default_duration_ms = duration_ms;
  anim_registry[anim_count].is_vector = false;
  anim_count++;
  return true;
}

bool anim_manager_register_vector(const anim_vector_t *vector) {
  if (anim_count >= MAX_ANIMATIONS || !vector)
    return false;
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
  return true;
}

bool anim_manager_play(const char *name, uint16_t loop) {
  if (!parent_obj)
    return false;
  anim_info_t *anim = NULL;
  for (uint8_t i = 0; i < anim_count; i++) {
    if (strcmp(anim_registry[i].name, name) == 0) {
      anim = &anim_registry[i];
      break;
    }
  }
  if (!anim)
    return false;

  anim_manager_stop();

  if (anim->is_vector) {
    current_vector_data = (anim_vector_t *)anim->vector;
    vector_loop_count = loop;
    vector_start_time = lv_tick_get();
    is_playing = true;
    current_anim_name = name;
    reset_pools();
    // Maintain Initial Z-order (only once)
    for (int i = 0; i < POOL_SIZE_OBJ; i++)
      lv_obj_move_foreground(obj_pool[i].obj);
    for (int i = 0; i < POOL_SIZE_LINE; i++)
      lv_obj_move_foreground(line_pool[i].obj);
    for (int i = 0; i < POOL_SIZE_TEXT; i++)
      lv_obj_move_foreground(text_pool[i].obj);

    printf("[AnimMgr] Playing Vector '%s' (Super Optimized)\n", name);
  } else {
    current_animimg = lv_animimg_create(parent_obj);
    lv_animimg_set_src(current_animimg, (const void **)anim->frames,
                       anim->frame_count);
    lv_animimg_set_duration(current_animimg, anim->default_duration_ms);
    lv_animimg_set_repeat_count(current_animimg,
                                loop == 0 ? LV_ANIM_REPEAT_INFINITE : loop);
    lv_obj_align(current_animimg, LV_ALIGN_CENTER, 0, 0);
    lv_animimg_start(current_animimg);
    is_playing = true;
    current_anim_name = name;
  }
  return true;
}

void anim_manager_stop(void) {
  if (current_animimg) {
    lv_obj_del(current_animimg);
    current_animimg = NULL;
  }
  reset_pools();
  is_playing = false;
  current_anim_name = NULL;
  current_vector_data = NULL;
}

void anim_manager_update(void) {
  if (!is_playing || !current_vector_data)
    return;

  uint32_t elapsed = lv_tick_get() - vector_start_time;
  uint32_t total_duration = 0;
  for (int i = 0; i < current_vector_data->frame_count; i++)
    total_duration += current_vector_data->frames[i].duration_ms;

  if (elapsed >= total_duration) {
    if (vector_loop_count == 1 ||
        (vector_loop_count > 1 && --vector_loop_count == 0)) {
      const char *name = current_anim_name;
      anim_manager_stop();
      if (finish_callback)
        finish_callback(name);
      return;
    }
    vector_start_time = lv_tick_get();
    elapsed = 0;
  }

  uint32_t time_acc = 0, local_time = 0;
  uint16_t found_frame = 0;
  for (int i = 0; i < current_vector_data->frame_count; i++) {
    if (elapsed >= time_acc &&
        elapsed < time_acc + current_vector_data->frames[i].duration_ms) {
      found_frame = i;
      local_time = elapsed - time_acc;
      break;
    }
    time_acc += current_vector_data->frames[i].duration_ms;
  }

  uint32_t dur = current_vector_data->frames[found_frame].duration_ms;
  float t = (dur > 0) ? (float)local_time / dur : 0.0f;

  // Apply Easing
  t = apply_easing(t, current_vector_data->frames[found_frame].easing);

  const anim_vector_frame_t *f1 = &current_vector_data->frames[found_frame];
  const anim_vector_frame_t *f2 =
      &current_vector_data
           ->frames[(found_frame + 1) % current_vector_data->frame_count];

  int obj_idx = 0, line_idx = 0, text_idx = 0;
  for (int i = 0; i < f1->shape_count; i++) {
    const anim_shape_t *s1 = &f1->shapes[i];
    const anim_shape_t *s2 = (i < f2->shape_count) ? &f2->shapes[i] : s1;

    float x = s1->x + (s2->x - s1->x) * t;
    float y = s1->y + (s2->y - s1->y) * t;
    float w = s1->w + (s2->w - s1->w) * t;
    float h = s1->h + (s2->h - s1->h) * t;
    float opa = (s1->opacity + (s2->opacity - s1->opacity) * t) * 255.0f;
    float rot = s1->rotation + (s2->rotation - s1->rotation) * t;

    if (s1->type == 0 || s1->type == 1) { // RECT or ELLIPSE
      if (obj_idx < POOL_SIZE_OBJ) {
        pool_item_t *item = &obj_pool[obj_idx++];
        lv_obj_t *o = item->obj;
        item->type = s1->type;
        item->color = s1->color;
        item->opa = (uint8_t)opa;

        lv_obj_set_size(o, (lv_coord_t)w, (lv_coord_t)h);
        lv_obj_set_pos(o, (lv_coord_t)x, (lv_coord_t)y);

        if (s1->type == 0) { // RECT
          lv_obj_set_style_bg_color(o, lv_color_hex(s1->color), 0);
          lv_obj_set_style_bg_opa(o, (uint8_t)opa, 0);
          lv_obj_set_style_radius(o, 0, 0);
        } else { // ELLIPSE (Handled by event callback)
          lv_obj_set_style_bg_opa(o, 0, 0);
        }

        lv_obj_set_style_transform_pivot_x(o, (lv_coord_t)(w / 2), 0);
        lv_obj_set_style_transform_pivot_y(o, (lv_coord_t)(h / 2), 0);
        lv_obj_set_style_transform_angle(o, (int16_t)(rot * 10.0f), 0);

        lv_obj_clear_flag(o, LV_OBJ_FLAG_HIDDEN);
      }
    } else if (s1->type == 2 && line_idx < POOL_SIZE_LINE) {
      lv_obj_t *l = line_pool[line_idx++].obj;
      static lv_point_t pts[2];
      pts[0].x = (lv_coord_t)x;
      pts[0].y = (lv_coord_t)y;
      pts[1].x = (lv_coord_t)(s1->x2 + (s2->x2 - s1->x2) * t);
      pts[1].y = (lv_coord_t)(s1->y2 + (s2->y2 - s1->y2) * t);
      lv_line_set_points(l, pts, 2);
      lv_obj_set_style_line_color(l, lv_color_hex(s1->color), 0);
      lv_obj_set_style_line_opa(l, (uint8_t)opa, 0);
      lv_obj_clear_flag(l, LV_OBJ_FLAG_HIDDEN);
    } else if (s1->type == 3 && s1->text && text_idx < POOL_SIZE_TEXT) {
      lv_obj_t *lbl = text_pool[text_idx++].obj;
      lv_label_set_text(lbl, s1->text);
      lv_obj_set_pos(lbl, (lv_coord_t)x, (lv_coord_t)y);
      lv_obj_set_style_text_color(lbl, lv_color_hex(s1->color), 0);
      lv_obj_set_style_text_opa(lbl, (uint8_t)opa, 0);
      lv_obj_clear_flag(lbl, LV_OBJ_FLAG_HIDDEN);
    }
  }
  // Hide rest
  for (int i = obj_idx; i < POOL_SIZE_OBJ; i++)
    lv_obj_add_flag(obj_pool[i].obj, LV_OBJ_FLAG_HIDDEN);
  for (int i = line_idx; i < POOL_SIZE_LINE; i++)
    lv_obj_add_flag(line_pool[i].obj, LV_OBJ_FLAG_HIDDEN);
  for (int i = text_idx; i < POOL_SIZE_TEXT; i++)
    lv_obj_add_flag(text_pool[i].obj, LV_OBJ_FLAG_HIDDEN);
}

bool anim_manager_is_playing(void) { return is_playing; }
const char *anim_manager_get_current(void) { return current_anim_name; }
void anim_manager_set_finish_callback(void (*callback)(const char *)) {
  finish_callback = callback;
}

void anim_manager_list_all(void) {
  for (uint8_t i = 0; i < anim_count; i++)
    printf("  %d. '%s' [%s]\n", i + 1, anim_registry[i].name,
           anim_registry[i].is_vector ? "VECTOR" : "BITMAP");
}

// Binary Loader for .rbat files (Updated for Easing)
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

  anim_vector_t *vector = (anim_vector_t *)malloc(sizeof(anim_vector_t));
  ((char **)&vector->name)[0] = strdup(path);
  vector->frame_count = frame_count;

  anim_vector_frame_t *write_frames =
      (anim_vector_frame_t *)malloc(sizeof(anim_vector_frame_t) * frame_count);
  ((anim_vector_frame_t **)&vector->frames)[0] = write_frames;

  for (uint16_t i = 0; i < frame_count; i++) {
    uint16_t duration, shape_count;
    uint8_t easing;
    fread(&duration, 2, 1, f);
    fread(&shape_count, 2, 1, f);
    fread(&easing, 1, 1, f);

    write_frames[i].duration_ms = duration;
    write_frames[i].shape_count = shape_count;
    write_frames[i].easing = easing;

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
