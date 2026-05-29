#ifndef IMU_APP_H
#define IMU_APP_H

#include "jy61p.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Recommended public variables for other modules. */
extern volatile float g_imu_car_yaw_est_deg;
extern volatile float g_imu_car_pitch_deg;
extern volatile float g_imu_car_roll_deg;
extern volatile float g_imu_gyro_yaw_dps;
extern volatile float g_imu_acc_forward_g;
extern volatile float g_imu_acc_right_g;
extern volatile float g_imu_acc_down_g;
extern volatile float g_imu_temperature_c;
extern volatile uint8_t g_imu_valid;
extern volatile uint8_t g_imu_calibrated;
extern volatile uint16_t g_imu_error_count;
extern volatile uint8_t g_imu_state;

/* Compatibility aliases using the variable names in the architecture document. */
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

void IMU_App_Init(I2C_HandleTypeDef *i2c);
void IMU_App_Loop(void);
void IMU_App_StartCalibration(void);
void IMU_App_SetYawZero(void);
const JY61P_Device_t *IMU_App_GetDevice(void);

#ifdef __cplusplus
}
#endif

#endif /* IMU_APP_H */
