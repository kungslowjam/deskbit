/**
 * @file sensor_logic.h
 * @brief Robot Sensor Logic (IMU -> Eyes)
 */

#ifndef SENSOR_LOGIC_H
#define SENSOR_LOGIC_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the sensor logic task (QMI8658)
 */
void sensor_logic_init(void);

#ifdef __cplusplus
}
#endif

#endif // SENSOR_LOGIC_H
