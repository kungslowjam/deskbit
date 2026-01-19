/**
 * @file anim_manager_example.c
 * @brief Example: How to integrate Animation Manager with ui_robo_eyes.c
 *
 * This file shows you how to use Animation Manager WITHOUT modifying
 * your existing ui_robo_eyes.c file.
 *
 * You can either:
 * 1. Copy these functions into ui_robo_eyes.c
 * 2. Or create a new file and call from ui_robo_eyes.c
 */

#include "anim_manager.h"
#include "lvgl.h"

// Declare this in anim_registry.c
extern void register_all_animations(void);

// ========================================
// Example 1: Initialize in your init function
// ========================================

void example_init_with_anim_manager(lv_obj_t *scr_eyes) {
  // Your existing init code...
  // ...

  // Add these 2 lines:
  anim_manager_init(scr_eyes);
  register_all_animations();

  printf("Animation Manager ready!\n");
}

// ========================================
// Example 2: Auto-blink every 3 seconds
// ========================================

void example_auto_blink_update(void) {
  static uint32_t last_blink = 0;
  uint32_t now = lv_tick_get();

  // Blink every 3 seconds
  if (now - last_blink > 3000) {
    anim_manager_play("blink", 1); // Play once
    last_blink = now;
  }

  // IMPORTANT: Call update every frame
  anim_manager_update();
}

// ========================================
// Example 3: Emotion system with animations
// ========================================

typedef enum { EMO_IDLE, EMO_HAPPY, EMO_SAD, EMO_BLINK, EMO_LOVE } emotion_t;

void example_set_emotion(emotion_t emotion) {
  switch (emotion) {
  case EMO_IDLE:
    anim_manager_play("idle", 0); // Loop forever
    break;

  case EMO_HAPPY:
    anim_manager_play("happy", 0); // Loop forever
    break;

  case EMO_SAD:
    anim_manager_play("sad", 0); // Loop forever
    break;

  case EMO_BLINK:
    anim_manager_play("blink", 1); // Play once
    break;

  case EMO_LOVE:
    anim_manager_play("love", 0); // Loop forever
    break;
  }
}

// ========================================
// Example 4: Animation callback
// ========================================

void example_on_animation_finished(const char *anim_name) {
  printf("Animation '%s' finished!\n", anim_name);

  // Do something after animation ends
  if (strcmp(anim_name, "blink") == 0) {
    // Return to idle animation
    anim_manager_play("idle", 0);
  }
}

void example_setup_callback(void) {
  anim_manager_set_finish_callback(example_on_animation_finished);
}

// ========================================
// Example 5: Check animation state
// ========================================

void example_check_state(void) {
  if (anim_manager_is_playing()) {
    const char *current = anim_manager_get_current();
    printf("Currently playing: %s\n", current);
  } else {
    printf("No animation playing\n");
  }
}

// ========================================
// Example 6: Complete integration example
// ========================================

/*
 * Add this to your ui_robo_eyes.c:
 */

/*
#include "anim_manager.h"
extern void register_all_animations(void);

// In your init function:
void ui_robo_eyes_init(void) {
    // ... existing code ...

    // Initialize Animation Manager
    anim_manager_init(scr_eyes);
    register_all_animations();
}

// In your update function:
static void update_positions(void) {
    // ... existing code ...

    // Auto-blink
    static uint32_t last_blink = 0;
    uint32_t now = lv_tick_get();
    if (now - last_blink > 3000) {
        anim_manager_play("blink", 1);
        last_blink = now;
    }

    // Update Animation Manager
    anim_manager_update();
}
*/
