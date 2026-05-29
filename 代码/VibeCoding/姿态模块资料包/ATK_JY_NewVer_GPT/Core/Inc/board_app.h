#ifndef BOARD_APP_H
#define BOARD_APP_H

#include "main.h"
#include "imu_app.h"
#include "laser_app.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BOARD_SENSOR_FAULT_IMU_INVALID       (1UL << 0)
#define BOARD_SENSOR_FAULT_IMU_NOT_CALIB     (1UL << 1)
#define BOARD_SENSOR_FAULT_LASER_INVALID     (1UL << 2)
#define BOARD_SENSOR_FAULT_LASER_TOO_CLOSE   (1UL << 3)

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

    uint16_t laser_raw_distance_mm;
    uint16_t laser_filtered_distance_mm;
    uint8_t laser_ok;
    uint8_t laser_is_warning;
    uint8_t laser_is_slow;
    uint8_t laser_is_too_close;

    uint32_t sensor_fault_flags;
    uint32_t timestamp_ms;
} SENSOR_Snapshot_t;

extern volatile SENSOR_Snapshot_t g_sensor_snapshot;
extern volatile uint32_t g_sensor_fault_flags;

void Board_App_Init(void);
void Board_App_Loop(void);
void Board_App_GetSnapshot(SENSOR_Snapshot_t *out);
uint32_t Board_App_GetFaultFlags(void);
void Board_App_I2C1_BusRecover(void);
void Board_App_UartRxCpltCallback(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_APP_H */
