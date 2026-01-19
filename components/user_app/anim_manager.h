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
#include <stdbool.h>

// Animation info structure
typedef struct {
  const char *name;
  const lv_img_dsc_t **frames;
  uint8_t frame_count;
  uint16_t default_duration_ms;
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
 * @brief Set callback when animation finishes
 * @param callback Function to call when animation ends
 */
void anim_manager_set_finish_callback(void (*callback)(const char *anim_name));

#endif // ANIM_MANAGER_H
