#include "anim_presets.h"
#include "anim_manager.h"
#include <stddef.h>

// --- PRESET: WINK ---
static const anim_shape_t wink_f0[] = {
    {1, 90, 80, 60, 190, 0, 0x00FFFF, 1.0f, 0, 0, NULL, 0, 100, 0, 0, NULL,
     0}, // Left (Normal)
    {1, 376, 80, 60, 190, 0, 0x00FFFF, 1.0f, 0, 0, NULL, 0, 100, 0, 0, NULL,
     0} // Right (Normal)
};
static const anim_shape_t wink_f1[] = {
    {1, 90, 80, 60, 190, 0, 0x00FFFF, 1.0f, 0, 0, NULL, 0, 100, 0, 0, NULL,
     0}, // Left (Normal)
    {1, 376, 175, 75, 10, 0, 0x00FFFF, 1.0f, 0, 0, NULL, 0, 5, 0, 0, NULL,
     0} // Right (Wink/Closed)
};
static const anim_vector_frame_t wink_frames[] = {
    {wink_f0, 2, 300, 3}, // Start
    {wink_f1, 2, 100, 0}, // Close fast
    {wink_f0, 2, 200, 4}  // Open with overshoot
};
static const anim_vector_t wink_anim = {"wink", wink_frames, 3};

// --- PRESET: SURPRISE ---
static const anim_shape_t surp_f0[] = {
    {1, 90, 80, 60, 190, 0, 0x00FFFF, 1.0f, 0, 0, NULL, 0, 100, 0, 0, NULL, 0},
    {1, 376, 80, 60, 190, 0, 0x00FFFF, 1.0f, 0, 0, NULL, 0, 100, 0, 0, NULL,
     0}};
static const anim_shape_t surp_f1[] = {
    {1, 75, 60, 90, 230, 0, 0x00FFFF, 1.0f, 0, 0, NULL, 0, 115, 0, 0, NULL, 0},
    {1, 361, 60, 90, 230, 0, 0x00FFFF, 1.0f, 0, 0, NULL, 0, 115, 0, 0, NULL,
     0}};
static const anim_vector_frame_t surp_frames[] = {
    {surp_f0, 2, 100, 4}, {surp_f1, 2, 1000, 3}, {surp_f0, 2, 300, 3}};
static const anim_vector_t surp_anim = {"surprise", surp_frames, 3};

// --- PRESET: SKEPTIC ---
static const anim_shape_t skep_f1[] = {
    {1, 90, 80, 60, 190, 0, 0x00FFFF, 1.0f, 0, 0, NULL, 0, 100, 0, 0, NULL,
     0}, // Normal
    {1, 376, 130, 70, 80, 0, 0x00FFFF, 1.0f, 0, 0, NULL, 0, 40, 0, 0, NULL,
     0} // Squint
};
static const anim_vector_frame_t skep_frames[] = {
    {surp_f0, 2, 200, 3}, {skep_f1, 2, 1500, 3}, {surp_f0, 2, 400, 3}};
static const anim_vector_t skep_anim = {"skeptic", skep_frames, 3};

void anim_presets_init(void) {
  anim_manager_register_vector(&wink_anim);
  anim_manager_register_vector(&surp_anim);
  anim_manager_register_vector(&skep_anim);
}
