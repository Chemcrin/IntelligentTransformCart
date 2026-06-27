#ifndef SENSORS_H
#define SENSORS_H

#include "atk_ms53l1m.h"
#include "jy61p.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SENSOR_I2C_TIMEOUT_MS              5U
#define SENSOR_DATA_TIMEOUT_MS             500U
#define SENSOR_I2C_RECOVER_PULSE_COUNT     9U
#define SENSOR_FAULT_RECOVER_PERIOD_MS     1000U

typedef struct
{
    float car_yaw_est_deg;
    float car_pitch_deg;
    float car_roll_deg;
    float gyro_yaw_dps;
    float acc_forward_g;
    float acc_right_g;
    float acc_down_g;
    float temperature_c;
    uint8_t imu_valid;
    uint8_t imu_calibrated;
    JY61P_State_t imu_state;
    JY61P_Error_t imu_error;

    uint16_t laser_raw_distance_mm;
    uint16_t laser_filtered_distance_mm;
    uint16_t laser_last_valid_distance_mm;
    uint8_t laser_ok;
    uint8_t laser_warning;
    uint8_t laser_slow;
    uint8_t laser_too_close;
    ATK_MS53L1M_Status_t laser_error;

    uint32_t imu_last_ms;
    uint32_t laser_last_ms;
    uint32_t snapshot_ms;
} SENSOR_Snapshot_t;

void sensor_update(void);
void sensor_get_snapshot(SENSOR_Snapshot_t *snapshot);

#ifdef __cplusplus
}
#endif

#endif
