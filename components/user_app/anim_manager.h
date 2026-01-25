/**
 * @file anim_manager.h
 * @brief Animation Manager for Robot Face Studio animations
 *
 * This manager allows you to easily add and play animations
 * exported from Robot Face Studio without modifying existing code.
 *
 * Usage:
 *   1. Export animation from Web Studio (e.g., blink_anim.c, blink_anim.h)
 *   2. Copy files to animations/ folder
 *   3. Register in anim_manager.c
 *   4. Play with anim_manager_play("blink")
 */

#ifndef ANIM_MANAGER_H
#define ANIM_MANAGER_H

#include "lvgl.h"
#include "ui_custom_anim.h" // Use shape_type_t from here
#include <stdbool.h>

// Shape definition for vector animations
typedef struct {
  uint8_t type; // shape_type_t from ui_custom_anim.h
  float x, y, w, h;
  float rotation;
  uint32_t color; // HEX Color (e.g. 0xFFFFFF)
  float opacity;
  float x2, y2;      // Extra coords for lines
  const char *text;  // Text content for SHAPE_TEXT
  uint8_t font_size; // Font size for text (8, 12, 16, 20, etc.)
} anim_shape_t;

// Vector Frame
typedef struct {
  const anim_shape_t *shapes;
  uint16_t shape_count;
  uint16_t duration_ms;
} anim_vector_frame_t;

// Vector Animation
typedef struct {
  const char *name;
  const anim_vector_frame_t *frames;
  uint16_t frame_count;
} anim_vector_t;

// Animation info structure
typedef struct {
  const char *name;
  const lv_img_dsc_t **frames; // Existing bitmap support
  const anim_vector_t *vector; // New vector support
  uint8_t frame_count;
  uint16_t default_duration_ms;
  bool is_vector;
} anim_info_t;

/**
 * @brief Initialize the animation manager
 * @param parent Parent object where animations will be created
 */
void anim_manager_init(lv_obj_t *parent);

/**
 * @brief Register a new animation
 * @param name Animation name (e.g., "blink")
 * @param frames Array of frame descriptors
 * @param frame_count Number of frames
 * @param duration_ms Default duration in milliseconds
 * @return true if registered successfully
 */
bool anim_manager_register(const char *name, const lv_img_dsc_t **frames,
                           uint8_t frame_count, uint16_t duration_ms);

/**
 * @brief Register a new vector animation
 * @param vector Vector animation data
 * @return true if registered successfully
 */
bool anim_manager_register_vector(const anim_vector_t *vector);

/**
 * @brief Play an animation by name
 * @param name Animation name
 * @param loop Number of times to loop (0 = infinite, 1 = once)
 * @return true if animation started successfully
 */
bool anim_manager_play(const char *name, uint16_t loop);

/**
 * @brief Stop current animation
 */
void anim_manager_stop(void);

/**
 * @brief Check if an animation is currently playing
 * @return true if playing
 */
bool anim_manager_is_playing(void);

/**
 * @brief Get current animation name
 * @return Animation name or NULL if not playing
 */
const char *anim_manager_get_current(void);

/**
 * @brief Update function - call this in your main loop
 * Handles animation cleanup and callbacks
 */
void anim_manager_update(void);

/**
 * @brief Load a binary animation (.rbat) from file
 * @param path File path
 * @return true if loaded and registered
 */
bool anim_manager_load_rbat(const char *path);

/**
 * @brief Set callback when animation finishes
 * @param callback Function to call when animation ends
 */
void anim_manager_set_finish_callback(void (*callback)(const char *anim_name));

#endif // ANIM_MANAGER_H
