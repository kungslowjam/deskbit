/**
 * @file ui_robo_eyes.h
 * @brief Multi-Emotion Robot Eyes System
 *
 * Supports 10 unique emotion expressions with smooth transitions:
 * NORMAL, HAPPY, SAD, LAUGH, LOVE, SLEEP, TALK, ANGRY, SURPRISED, SKEPTIC
 */

#ifndef UI_ROBO_EYES_H
#define UI_ROBO_EYES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/**
 * @brief Robot emotion types
 */
typedef enum {
  EMOTION_NORMAL = 0, // Default friendly look
  EMOTION_HAPPY,      // Arch shape, cheerful
  EMOTION_SAD,        // Drooping, with optional tear
  EMOTION_LAUGH,      // Closed crescents, vibrating
  EMOTION_LOVE,       // Heart shape or heart pupils
  EMOTION_SLEEP,      // Thin closed lines
  EMOTION_TALK,       // Normal with speech-sync blinks
  EMOTION_ANGRY,      // Small intense with top mask
  EMOTION_SURPRISED,  // Large round circles
  EMOTION_SKEPTIC,    // Asymmetric, one squinted
  EMOTION_MAX
} robot_emotion_t;

/**
 * @brief Initialize the robot eyes UI
 */
void ui_robo_eyes_init(void);

/**
 * @brief Set emotion by enum type (recommended)
 * @param emotion The emotion to display
 */
void ui_robo_eyes_set_emotion_type(robot_emotion_t emotion);

/**
 * @brief Set emotion by string name (legacy support)
 * @param emotion String name: "normal", "happy", "sad", etc.
 */
void ui_robo_eyes_set_emotion(const char *emotion);

/**
 * @brief Make eyes look at a specific position
 * @param x Horizontal offset (-100 to 100)
 * @param y Vertical offset (-100 to 100)
 */
void ui_robo_eyes_look_at(int16_t x, int16_t y);

/**
 * @brief Trigger a blink animation
 */
void ui_robo_eyes_blink(void);

/**
 * @brief Get current emotion
 * @return Current emotion type
 */
robot_emotion_t ui_robo_eyes_get_emotion(void);

#ifdef __cplusplus
}
#endif

#endif // UI_ROBO_EYES_H
