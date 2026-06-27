#ifndef IMU_APP_H
#define IMU_APP_H

#include "jy61p.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern volatile float car_yaw_est_deg;
extern volatile float car_pitch_deg;
extern volatile float car_roll_deg;
extern volatile float gyro_yaw_dps;
extern volatile float acc_forward_g;
extern volatile float acc_right_g;
extern volatile float acc_down_g;
extern volatile float temperature_c;
extern volatile uint8_t imu_valid;
extern volatile uint8_t imu_calibrated;

extern volatile uint32_t g_imu_update_count;
extern volatile uint32_t g_imu_last_tick_ms;
extern volatile uint16_t g_imu_error_count;
extern volatile JY61P_State_t g_imu_state;
extern volatile JY61P_Error_t g_imu_last_error;

extern volatile float g_imu_roll_deg;
extern volatile float g_imu_pitch_deg;
extern volatile float g_imu_yaw_deg;
extern volatile float g_imu_ax_g;
extern volatile float g_imu_ay_g;
extern volatile float g_imu_az_g;
extern volatile float g_imu_gx_dps;
extern volatile float g_imu_gy_dps;
extern volatile float g_imu_gz_dps;
extern volatile float g_imu_temp_c;

void IMU_App_Init(void);
void IMU_App_Loop(void);
HAL_StatusTypeDef IMU_App_ZeroYaw(void);
HAL_StatusTypeDef IMU_App_StartGyroCalibration(void);
HAL_StatusTypeDef IMU_App_CalibrateAccelerometer(void);
const JY61P_Device_t *IMU_App_GetDevice(void);

#ifdef __cplusplus
}
#endif

#endif
