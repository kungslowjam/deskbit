#include "ui_liquid.h"
#include "esp_heap_caps.h"
#include "qmi8658c.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * LIQUID SOUL ENGINE v2.3 (Stable Aggregator Edition)
 * - Replaced Polygon with Circle Aggregation (Fixes Crash)
 * - Restored Spiky "Ferrobeat" Physics
 * - High-Contrast Cyan Aura Visuals
 */

#define V_RES 200
#define REAL_RES 466
#define SPIKE_COUNT 24
#define CORE_RAD 35.0f
#define SPIKE_BASE_RAD 18.0f

typedef struct {
  float x, y;
  float vx, vy;
  float radii[SPIKE_COUNT];
  float v_radii[SPIKE_COUNT];
} ferro_blob_t;

static ferro_blob_t blob;
static lv_obj_t *canvas = NULL;
static lv_color_t *canvas_buf = NULL;
static lv_timer_t *update_timer = NULL;
static bool is_active = false;

static void liquid_update_cb(lv_timer_t *t) {
  if (!is_active || !canvas)
    return;

  // 1. Read IMU
  float acc[3], gyro[3];
  qmi8658_read_xyz(acc, gyro);

  // Gravity (Center of Mass)
  float gx = -acc[0] * 0.28f;
  float gy = acc[1] * 0.28f;

  // Shake agitation
  float shake = (fabsf(gyro[0]) + fabsf(gyro[1]) + fabsf(gyro[2])) / 90.0f;
  if (shake > 5.0f)
    shake = 5.0f;

  // 2. Physics: Center of Mass
  blob.vx += gx;
  blob.vy += gy;
  blob.vx *= 0.9f;
  blob.vy *= 0.9f;
  blob.x += blob.vx;
  blob.y += blob.vy;

  // Fluid Bounds
  float margin = 60.0f;
  if (blob.x < margin) {
    blob.x = margin;
    blob.vx *= -0.3f;
  }
  if (blob.x > V_RES - margin) {
    blob.x = V_RES - margin;
    blob.vx *= -0.3f;
  }
  if (blob.y < margin) {
    blob.y = margin;
    blob.vy *= -0.3f;
  }
  if (blob.y > V_RES - margin) {
    blob.y = V_RES - margin;
    blob.vy *= -0.3f;
  }

  // 3. Physics: Surface Tension (Spikes)
  float time = (float)lv_tick_get() * 0.002f;
  for (int i = 0; i < SPIKE_COUNT; i++) {
    float peak = 0;
    if (shake > 0.7f) {
      // Harmonic peaks
      peak = sinf(time * 15.0f + i * 1.5f) * (shake * 18.0f);
    }

    float target = CORE_RAD + peak + (sinf(time * 3.0f + i * 0.5f) * 3.0f);
    float force = (target - blob.radii[i]) * 0.15f;

    blob.v_radii[i] += force;
    blob.v_radii[i] *= 0.85f;
    blob.radii[i] += blob.v_radii[i];
  }

  // 4. Draw
  // Fill Black
  lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);

  // Cyan Aura Glow
  lv_draw_rect_dsc_t glow;
  lv_draw_rect_dsc_init(&glow);
  glow.bg_color = lv_color_hex(0x00D4FF);
  glow.bg_opa = LV_OPA_30;
  glow.radius = LV_RADIUS_CIRCLE;
  lv_canvas_draw_rect(canvas, (lv_coord_t)blob.x - 65, (lv_coord_t)blob.y - 65,
                      130, 130, &glow);

  // Draw Interior Glow
  glow.bg_opa = LV_OPA_50;
  lv_canvas_draw_rect(canvas, (lv_coord_t)blob.x - 40, (lv_coord_t)blob.y - 40,
                      80, 80, &glow);

  // Draw Blobs (Cohesive aggregation)
  lv_draw_rect_dsc_t dsc;
  lv_draw_rect_dsc_init(&dsc);
  dsc.bg_color = lv_color_black();
  dsc.radius = LV_RADIUS_CIRCLE;
  dsc.border_width = 1;
  dsc.border_color = lv_color_hex(0x00AADD);
  dsc.border_opa = LV_OPA_40;

  // Draw central core
  lv_canvas_draw_rect(canvas, (lv_coord_t)blob.x - CORE_RAD,
                      (lv_coord_t)blob.y - CORE_RAD, CORE_RAD * 2, CORE_RAD * 2,
                      &dsc);

  // Draw Aggregated Spike Circles (These merge visually)
  for (int i = 0; i < SPIKE_COUNT; i++) {
    float angle = (i * 2.0f * (float)M_PI) / SPIKE_COUNT;
    float sx = blob.x + cosf(angle) * blob.radii[i];
    float sy = blob.y + sinf(angle) * blob.radii[i];
    float r = SPIKE_BASE_RAD + (shake * 2.0f);

    lv_canvas_draw_rect(canvas, (lv_coord_t)sx - r, (lv_coord_t)sy - r, r * 2,
                        r * 2, &dsc);
  }

  // Top Shine (Wet Look)
  dsc.bg_color = lv_color_white();
  dsc.bg_opa = LV_OPA_20;
  dsc.border_width = 0;
  lv_canvas_draw_rect(canvas, (lv_coord_t)blob.x - 10, (lv_coord_t)blob.y - 25,
                      25, 12, &dsc);
}

void ui_liquid_init(void) {
  blob.x = (float)V_RES / 2.0f;
  blob.y = (float)V_RES / 2.0f;
  blob.vx = 0;
  blob.vy = 0;
  for (int i = 0; i < SPIKE_COUNT; i++) {
    blob.radii[i] = CORE_RAD;
    blob.v_radii[i] = 0;
  }
}

void ui_liquid_show(lv_obj_t *parent) {
  if (canvas)
    return;

  ui_liquid_init();
  is_active = true;

  size_t buf_size = LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA(V_RES, V_RES);
  canvas_buf = heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
  if (!canvas_buf)
    return;

  canvas = lv_canvas_create(parent);
  lv_canvas_set_buffer(canvas, canvas_buf, V_RES, V_RES,
                       LV_IMG_CF_TRUE_COLOR_ALPHA);

  float zoom = (float)REAL_RES / V_RES;
  lv_img_set_zoom(canvas, (uint16_t)(256 * zoom));
  lv_obj_center(canvas);

  update_timer = lv_timer_create(liquid_update_cb, 25, NULL);
  printf("[Liquid] Aggregator Engine Loaded\n");
}

void ui_liquid_hide(void) {
  is_active = false;
  if (update_timer) {
    lv_timer_del(update_timer);
    update_timer = NULL;
  }
  if (canvas) {
    lv_obj_del(canvas);
    canvas = NULL;
  }
  if (canvas_buf) {
    free(canvas_buf);
    canvas_buf = NULL;
  }
  printf("[Liquid] Hidden\n");
}
