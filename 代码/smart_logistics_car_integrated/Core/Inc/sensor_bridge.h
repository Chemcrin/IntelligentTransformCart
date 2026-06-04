#ifndef SENSOR_BRIDGE_H
#define SENSOR_BRIDGE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float yaw_rad;
    float gyro_z_rads;
    float pitch_deg;
    float roll_deg;
    float distance_m;
    uint16_t raw_distance_mm;
    uint16_t filtered_distance_mm;
    bool imu_valid;
    bool imu_calibrated;
    bool laser_valid;
    uint8_t laser_warning;
    uint32_t timestamp_ms;
    uint32_t fault_flags;
} SensorSnapshot_t;

float JY61P_GetYawRad(void);
float JY61P_GetGyroZRadS(void);
float JY61P_GetPitchDeg(void);
float JY61P_GetRollDeg(void);
bool JY61P_IsDataValid(void);
bool JY61P_IsCalibrated(void);

bool Laser_GetDistance(float *distance_m);
uint16_t Laser_GetRawDistanceMm(void);
uint8_t Laser_GetWarningLevel(void);
bool Laser_IsOnline(void);

void Sensor_GetSnapshot(SensorSnapshot_t *snap);

#ifdef __cplusplus
}
#endif

#endif
