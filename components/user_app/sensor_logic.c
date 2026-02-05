/**
 * @file sensor_logic.c
 * @brief Robot Sensor Logic - Maps IMU actions to Robot Expressions
 */

#include "sensor_logic.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "qmi8658c.h"
#include "ui_robo_eyes.h"
#include <math.h>
#include <stdio.h>

// Config
#define SENSOR_TASK_STACK_SIZE 4096
#define SENSOR_TASK_PRIORITY 5
#define SENSOR_POLL_MS 20 // 50Hz update rate

// Interaction Thresholds
#define SHAKE_THRESHOLD_STRONG 0.8f // Heavy shake
#define SHAKE_THRESHOLD_GENTLE 0.3f // Light nudge
#define TILT_DEADZONE 0.1f          // Ignore small tilts

// State
static float acc[3];
static float gyro[3];
static float lp_acc[3] = {0}; // Low-passed accel
static float alpha = 0.1f;    // Smoothing factor

static int shake_cooldown = 0;

static void sensor_logic_task(void *arg) {
  // 1. Initialize Sensor
  qmi8658_init();
  printf("[SensorLogic] QMI8658 Initialized\n");

  // Give it a moment to settle
  vTaskDelay(pdMS_TO_TICKS(100));

  // Base loop
  while (1) {
    // Read IMU
    qmi8658_read_xyz(acc, gyro);

    // Low Pass Filter for smooth Gaze
    for (int i = 0; i < 3; i++) {
      lp_acc[i] = (alpha * acc[i]) + ((1.0f - alpha) * lp_acc[i]);
    }

    // --- 1. SHAKE DETECTION (Magnitude Variation) ---
    // Calculate magnitude of AC (Dynamic) acceleration (High Pass)
    // Simple approximation: |Current - LowPass|
    float diff_x = acc[0] - lp_acc[0];
    float diff_y = acc[1] - lp_acc[1];
    float diff_z = acc[2] - lp_acc[2];
    float dynamic_mag =
        sqrtf(diff_x * diff_x + diff_y * diff_y + diff_z * diff_z);

    if (shake_cooldown > 0) {
      shake_cooldown -= SENSOR_POLL_MS;
    } else {
      if (dynamic_mag > SHAKE_THRESHOLD_STRONG) {
        // STRONG SHAKE -> ANGRY (User Request)
        // Check current emotion to avoid resetting if already there
        if (ui_robo_eyes_get_emotion() != EMOTION_ANGRY) {
          ui_robo_eyes_set_emotion_type(EMOTION_ANGRY);
          printf("[Sensor] Shake Detected! ANGRY\n");
          shake_cooldown = 2000; // 2 seconds lock
        }
      } else if (dynamic_mag > SHAKE_THRESHOLD_GENTLE) {
        // GENTLE NUDGE -> WAKE UP / HAPPY
        if (ui_robo_eyes_get_emotion() == EMOTION_SLEEP) {
          ui_robo_eyes_set_emotion_type(EMOTION_NORMAL); // Wake up
          shake_cooldown = 1000;
        }
      }
    }

    // --- 2. TILT DETECTION (Look At) ---
    // Only update gaze if NOT shaking (to avoid erratic eyes)
    if (dynamic_mag < SHAKE_THRESHOLD_GENTLE && shake_cooldown <= 0) {

      // Orientation Mapping (Based on Board Mounting)
      // Usually:
      // Acc X -> Left/Right Tilt
      // Acc Y -> Up/Down Tilt

      // Map -0.5g..0.5g to -100..100 gaze range
      float gaze_x_raw = lp_acc[0]; // Assuming X is horizontal
      float gaze_y_raw = lp_acc[1]; // Assuming Y is vertical

      // Clamp and Scale
      float scale = 150.0f; // Multiplier for sensitivity

      int16_t look_x = (int16_t)(gaze_x_raw * scale);  // Invert if needed: -
      int16_t look_y = (int16_t)(gaze_y_raw * -scale); // Usually Y is inverted

      // Deadzone (Center snap)
      if (fabsf(gaze_x_raw) < TILT_DEADZONE)
        look_x = 0;
      if (fabsf(gaze_y_raw) < TILT_DEADZONE)
        look_y = 0;

      // Limit bounds
      if (look_x > 100)
        look_x = 100;
      if (look_x < -100)
        look_x = -100;
      if (look_y > 100)
        look_y = 100;
      if (look_y < -100)
        look_y = -100;

      // Only update if we are in an emotion that supports looking (Safe check)
      robot_emotion_t curr_emo = ui_robo_eyes_get_emotion();
      if (curr_emo == EMOTION_NORMAL || curr_emo == EMOTION_HAPPY ||
          curr_emo == EMOTION_ANGRY || curr_emo == EMOTION_SKEPTIC ||
          curr_emo == EMOTION_TALK) {

        ui_robo_eyes_look_at(look_x, look_y);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(SENSOR_POLL_MS));
  }
}

void sensor_logic_init(void) {
  xTaskCreate(sensor_logic_task, "SensorLogic", SENSOR_TASK_STACK_SIZE, NULL,
              SENSOR_TASK_PRIORITY, NULL);
}
