/**
 * @file anim_registry.c
 * @brief Central registry for all Robot Face Studio animations
 *
 * HOW TO ADD NEW ANIMATIONS:
 * 1. Export from Web Studio (e.g., blink_anim.c, blink_anim.h)
 * 2. Copy to components/user_app/animations/
 * 3. Add #include below
 * 4. Add REGISTER_ANIM() in register_all_animations()
 * 5. Done! No need to modify other files.
 */

#include "anim_manager.h"

// ========================================
// Include your animation headers here
#include "animations/my_anim.h"
// ========================================


// Example: Uncomment when you have animations
// #include "animations/happy_anim.h"
// #include "animations/sad_anim.h"

// ========================================
// Register all animations
// ========================================

void register_all_animations(void) {
    anim_manager_register_vector(&my_anim_data);
  // Register animations here
  // Format: anim_manager_register(name, frames, frame_count, duration_ms)

  // Example: Uncomment when you have animations
  // anim_manager_register("happy", happy_anim_frames,
  // happy_anim_frame_count, 500); anim_manager_register("sad", sad_anim_frames,
  // sad_anim_frame_count, 800);

  // TODO: Add your animations here!
}
