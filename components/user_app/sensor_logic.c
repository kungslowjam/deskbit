/**
 * @file sensor_logic.c
 * @brief Robot Sensor Logic - Maps IMU actions to Robot Expressions
 */

#include "sensor_logic.h"
#include "anim_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "qmi8658c.h"
#include "ui_robo_eyes.h"
#include <math.h>
#include <stdio.h>

#define SENSOR_TASK_STACK_SIZE 4096
#define SENSOR_TASK_PRIORITY 5
#define SENSOR_POLL_MS 20 // 50Hz update rate

// Interaction Thresholds (In Gs, 1.0 = Earth Gravity)
#define SHAKE_THRESHOLD_STRONG 1.2f  // Heavy shake
#define SHAKE_THRESHOLD_GENTLE 0.4f  // Light nudge
#define TILT_THRESHOLD_SURPRISE 0.7f // Upside down / Big tilt
#define TILT_DEADZONE 0.2f           // Ignore small tilts

// State
static float acc_raw[3]; // Raw m/s2 from driver
static float acc[3];     // Normalized to Gs
static float gyro[3];
static float lp_acc[3] = {0}; // Low-passed accel (Gs)
static float alpha = 0.15f;   // Smoothing factor (slightly faster)

static int shake_cooldown = 0;
static int shake_accum = 0; // Tracks persistent shaking
static bool was_upside_down = false;

static void sensor_logic_task(void *arg) {
  // 1. Initialize Sensor with Retry
  bool imu_ok = false;
  int retry_count = 0;

  printf("[SensorLogic] Starting QMI8658 Initialization...\n");

  while (!imu_ok && retry_count < 10) {
    if (qmi8658_init() == 1) {
      imu_ok = true;
      printf("[SensorLogic] QMI8658 Initialized Successfully!\n");
    } else {
      retry_count++;
      printf("[SensorLogic] QMI8658 Init Failed (Trial %d). Retrying...\n",
             retry_count);
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  // Display initial status
  if (example_lvgl_lock(200)) {
    ui_robo_eyes_set_debug_text(imu_ok ? "IMU: OK" : "IMU: FAIL");
    example_lvgl_unlock();
  }

  // Give it a moment to settle
  vTaskDelay(pdMS_TO_TICKS(500));

  // Base loop
  while (1) {
    if (imu_ok) {
      // Read IMU (Returns m/s2)
      qmi8658_read_xyz(acc_raw, gyro);

      // Normalize to Gs (9.8 m/s2 = 1.0G)
      for (int i = 0; i < 3; i++) {
        acc[i] = acc_raw[i] / 9.807f;
      }

      // Low Pass Filter for smooth Gaze (even if not used, keeps state clean)
      for (int i = 0; i < 3; i++) {
        lp_acc[i] = (alpha * acc[i]) + ((1.0f - alpha) * lp_acc[i]);
      }

      // Shake Detection Magnitude (High Pass Filtered)
      float diff_x = acc[0] - lp_acc[0];
      float diff_y = acc[1] - lp_acc[1];
      float diff_z = acc[2] - lp_acc[2];
      float dynamic_mag =
          sqrtf(diff_x * diff_x + diff_y * diff_y + diff_z * diff_z);

      // --- DEBUG SHAKE METER ---
      static int debug_tick = 0;
      if (++debug_tick >= 10) { // Every 200ms
        debug_tick = 0;
        printf("[Sensor] G-Mag: %0.3f | Thr: %0.2f, Y-Tilt: %0.2f\n",
               dynamic_mag, SHAKE_THRESHOLD_GENTLE, lp_acc[1]);

        if (example_lvgl_lock(50)) {
          char dbg_buf[32];
          snprintf(dbg_buf, sizeof(dbg_buf), "G-Mag: %0.3f", dynamic_mag);
          ui_robo_eyes_set_debug_text(dbg_buf);
          example_lvgl_unlock();
        }
      }

      // --- Interaction Logic ---
      if (shake_cooldown > 0) {
        shake_cooldown -= SENSOR_POLL_MS;
        shake_accum = 0;
      } else {
        // STRONG SHAKE (Triggers Angry -> Dizzy)
        if (dynamic_mag > SHAKE_THRESHOLD_STRONG) {
          shake_accum += SENSOR_POLL_MS;

          // DIZZY: Shaking for > 1000ms consistently
          if (shake_accum > 1000) {
            printf("[Sensor] STABLE SHAKE! Triggering DIZZY\n");
            // Vector animation handled via Manager
            anim_manager_play("dizzy_anim", 1);
            shake_cooldown = 4000; // CD after big anim
            shake_accum = 0;
          }
          // ANGRY: Initial reaction (immediately on high mag)
          else if (ui_robo_eyes_get_emotion() != EMOTION_ANGRY &&
                   !anim_manager_is_playing()) {
            if (example_lvgl_lock(100)) { // Longer lock wait
              printf("[Sensor] Strong Mag! Triggering ANGRY\n");
              ui_robo_eyes_set_emotion_type(EMOTION_ANGRY);
              example_lvgl_unlock();
            } else {
              printf("[Sensor] !! Lock Failed, skipping Angry trigger\n");
            }
          }
        }
        // GENTLE NUDGE (Blink or Wake Up)
        else if (dynamic_mag > SHAKE_THRESHOLD_GENTLE) {
          // If we are already mid-shake, don't reset accum immediately (allow
          // small dips) Only reset if it stays in GENTLE range for a bit
          static int gentle_ticks = 0;
          if (++gentle_ticks > 5) { // 100ms in gentle range resets dizzy accum
            shake_accum = 0;
            gentle_ticks = 0;
          }

          if (ui_robo_eyes_get_emotion() != EMOTION_ANGRY &&
              !anim_manager_is_playing()) {
            if (example_lvgl_lock(50)) {
              if (ui_robo_eyes_get_emotion() == EMOTION_SLEEP) {
                ui_robo_eyes_set_emotion_type(EMOTION_NORMAL);
                shake_cooldown = 1000;
              } else if (ui_robo_eyes_get_emotion() == EMOTION_NORMAL) {
                ui_robo_eyes_blink();
                shake_cooldown = 500;
              }
              example_lvgl_unlock();
            }
          }
        } else {
          // TOTAL QUIET (Reset accum)
          shake_accum = 0;
        }
      }

      // SUDDEN IMPACT (Z-Axis Jerk)
      static float prev_z = 0;
      float z_jerk = fabsf(acc[2] - prev_z);
      prev_z = acc[2];
      if (z_jerk > 1.8f && shake_cooldown <= 0 && !anim_manager_is_playing()) {
        if (example_lvgl_lock(100)) {
          printf("[Sensor] IMPACT! Triggering SURPRISED\n");
          ui_robo_eyes_set_emotion_type(EMOTION_SURPRISED);
          example_lvgl_unlock();
          shake_cooldown = 1500;
        }
      }

      /* --- 3. TILT DETECTION DISABLED ---
      bool is_upside_down = (lp_acc[1] > TILT_THRESHOLD_SURPRISE);
      if (is_upside_down && !was_upside_down && shake_cooldown <= 0) {
        if (example_lvgl_lock(50)) {
          ui_robo_eyes_set_emotion_type(EMOTION_SURPRISED);
          example_lvgl_unlock();
          shake_cooldown = 2000;
        }
      }
      was_upside_down = is_upside_down;

      // Update gaze (DISABLED)
      if (dynamic_mag < SHAKE_THRESHOLD_GENTLE && shake_cooldown <= 0 &&
          !anim_manager_is_playing()) {
          // Gaze follow tilt is now disabled to keep eyes centered
      }
      */
    }
    vTaskDelay(pdMS_TO_TICKS(SENSOR_POLL_MS));
  }
}

void sensor_logic_init(void) {
  xTaskCreate(sensor_logic_task, "SensorLogic", SENSOR_TASK_STACK_SIZE, NULL,
              SENSOR_TASK_PRIORITY, NULL);
}
