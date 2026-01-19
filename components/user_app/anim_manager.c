/**
 * @file anim_manager.c
 * @brief Animation Manager Implementation
 */

#include "anim_manager.h"
#include <stdio.h>
#include <string.h>

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

  // Stop current animation if playing
  if (current_animimg) {
    lv_obj_del(current_animimg);
    current_animimg = NULL;
  }

  // Create animation image
  current_animimg = lv_animimg_create(parent_obj);
  if (!current_animimg) {
    printf("[AnimMgr] ERROR: Failed to create animimg\n");
    return false;
  }

  // Set frames
  lv_animimg_set_src(current_animimg, (const void **)anim->frames,
                     anim->frame_count);

  // Set duration
  lv_animimg_set_duration(current_animimg, anim->default_duration_ms);

  // Set loop count
  if (loop == 0) {
    lv_animimg_set_repeat_count(current_animimg, LV_ANIM_REPEAT_INFINITE);
  } else {
    lv_animimg_set_repeat_count(current_animimg, loop);
  }

  // Center on screen
  lv_obj_align(current_animimg, LV_ALIGN_CENTER, 0, 0);

  // Ensure visible and on top
  lv_obj_clear_flag(current_animimg, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(current_animimg);

  // Start animation
  lv_animimg_start(current_animimg);

  current_anim_name = name;
  is_playing = true;

  printf("[AnimMgr] Playing '%s' (loop: %d)\n", name, loop);

  return true;
}

void anim_manager_stop(void) {
  if (current_animimg) {
    lv_obj_del(current_animimg);
    current_animimg = NULL;
  }

  if (is_playing) {
    printf("[AnimMgr] Stopped '%s'\n", current_anim_name);
  }

  current_anim_name = NULL;
  is_playing = false;
}

bool anim_manager_is_playing(void) { return is_playing; }

const char *anim_manager_get_current(void) { return current_anim_name; }

void anim_manager_update(void) {
  // Check if animation finished
  if (is_playing && current_animimg) {
    // Check if animimg is still valid and playing
    // LVGL doesn't have a direct "is_playing" check, so we check if object
    // exists
    if (!lv_obj_is_valid(current_animimg)) {
      // Animation finished and object was deleted
      const char *finished_name = current_anim_name;
      is_playing = false;
      current_anim_name = NULL;
      current_animimg = NULL;

      // Call callback if set
      if (finish_callback) {
        finish_callback(finished_name);
      }
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
