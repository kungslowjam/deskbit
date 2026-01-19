/**
 * @file ui_custom_anim.h
 * @brief Custom Shape Animation Player with Interpolation & Easing
 */

#ifndef UI_CUSTOM_ANIM_H
#define UI_CUSTOM_ANIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdbool.h>

// Shape types (must match export)
typedef enum {
  SHAPE_RECT = 0,
  SHAPE_ELLIPSE = 1,
  SHAPE_LINE = 2,
  SHAPE_TEXT = 3 // Text label (rendered via lv_label)
} shape_type_t;

// Shape keyframe structure (must match export)
typedef struct {
  shape_type_t type;
  int16_t x, y;
  int16_t width, height;
  uint32_t color;
  int16_t rotation;
} shape_keyframe_t;

// Animation player handle
typedef struct {
  lv_obj_t *canvas; // Canvas for drawing
  lv_obj_t *parent; // Parent container
  void *canvas_buf; // Canvas buffer

  // Shape animation data
  const shape_keyframe_t **keyframes; // Array of keyframe arrays
  const uint8_t *shape_counts;        // Shapes per frame
  uint16_t frame_count;               // Total frames

  // Animation state
  lv_anim_t anim;           // LVGL animation object
  int32_t current_progress; // 0-1000 (0-100%)
  uint16_t current_frame;   // Current keyframe index
  uint16_t next_frame;      // Next keyframe index

  // Settings
  uint32_t duration_ms;     // Total animation duration
  lv_anim_path_cb_t easing; // Easing function
  bool is_playing;
  bool loop;
} custom_anim_t;

/**
 * @brief Create animation player
 */
custom_anim_t *ui_custom_anim_create(lv_obj_t *parent);

/**
 * @brief Load shape keyframes (NEW API)
 * @param anim Animation handle
 * @param keyframes Array of shape keyframe arrays (one per frame)
 * @param shape_counts Number of shapes in each frame
 * @param frame_count Total number of frames
 * @param duration_ms Total animation duration
 * @param easing Easing function (e.g., lv_anim_path_overshoot)
 */
void ui_custom_anim_set_shape_src(custom_anim_t *anim,
                                  const shape_keyframe_t **keyframes,
                                  const uint8_t *shape_counts,
                                  uint16_t frame_count, uint32_t duration_ms,
                                  lv_anim_path_cb_t easing);

/**
 * @brief Start animation
 */
void ui_custom_anim_start(custom_anim_t *anim, uint16_t repeat_count);

/**
 * @brief Stop animation
 */
void ui_custom_anim_stop(custom_anim_t *anim);

/**
 * @brief Align canvas
 */
void ui_custom_anim_align(custom_anim_t *anim, lv_align_t align,
                          lv_coord_t x_ofs, lv_coord_t y_ofs);

/**
 * @brief Delete animation
 */
void ui_custom_anim_delete(custom_anim_t *anim);

/**
 * @brief Check if playing
 */
bool ui_custom_anim_is_playing(custom_anim_t *anim);

// Legacy API (for backward compatibility - will use first/last frame only)
void ui_custom_anim_set_src(custom_anim_t *anim, const lv_img_dsc_t **imgs,
                            uint16_t count, uint32_t frame_duration_ms);

#ifdef __cplusplus
}
#endif

#endif // UI_CUSTOM_ANIM_H
