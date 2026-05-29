#include "imu_app.h"
#include <string.h>

static JY61P_Device_t s_jy61p;

volatile float g_imu_car_yaw_est_deg = 0.0f;
volatile float g_imu_car_pitch_deg = 0.0f;
volatile float g_imu_car_roll_deg = 0.0f;
volatile float g_imu_gyro_yaw_dps = 0.0f;
volatile float g_imu_acc_forward_g = 0.0f;
volatile float g_imu_acc_right_g = 0.0f;
volatile float g_imu_acc_down_g = 0.0f;
volatile float g_imu_temperature_c = 0.0f;
volatile uint8_t g_imu_valid = 0U;
volatile uint8_t g_imu_calibrated = 0U;
volatile uint16_t g_imu_error_count = 0U;
volatile uint8_t g_imu_state = JY61P_STATE_UNINIT;

volatile float car_yaw_est_deg = 0.0f;
volatile float car_pitch_deg = 0.0f;
volatile float car_roll_deg = 0.0f;
volatile float gyro_yaw_dps = 0.0f;
volatile float acc_forward_g = 0.0f;
volatile float acc_right_g = 0.0f;
volatile float acc_down_g = 0.0f;
volatile float temperature_c = 0.0f;
volatile uint8_t imu_valid = 0U;
volatile uint8_t imu_calibrated = 0U;

static void imu_app_sync_public_vars(void)
{
    JY61P_Attitude_t att;
    (void)JY61P_GetAttitude(&s_jy61p, &att);

    g_imu_car_yaw_est_deg = att.car_yaw_est_deg;
    g_imu_car_pitch_deg = att.car_pitch_deg;
    g_imu_car_roll_deg = att.car_roll_deg;
    g_imu_gyro_yaw_dps = att.gyro_yaw_dps;
    g_imu_acc_forward_g = att.acc_forward_g;
    g_imu_acc_right_g = att.acc_right_g;
    g_imu_acc_down_g = att.acc_down_g;
    g_imu_temperature_c = att.temperature_c;
    g_imu_valid = att.valid;
    g_imu_calibrated = att.calibrated;
    g_imu_error_count = s_jy61p.error_count;
    g_imu_state = (uint8_t)s_jy61p.state;

    car_yaw_est_deg = g_imu_car_yaw_est_deg;
    car_pitch_deg = g_imu_car_pitch_deg;
    car_roll_deg = g_imu_car_roll_deg;
    gyro_yaw_dps = g_imu_gyro_yaw_dps;
    acc_forward_g = g_imu_acc_forward_g;
    acc_right_g = g_imu_acc_right_g;
    acc_down_g = g_imu_acc_down_g;
    temperature_c = g_imu_temperature_c;
    imu_valid = g_imu_valid;
    imu_calibrated = g_imu_calibrated;
}

void IMU_App_Init(I2C_HandleTypeDef *i2c)
{
    uint32_t now_ms = HAL_GetTick();
    (void)JY61P_Init(&s_jy61p, i2c, now_ms);
    imu_app_sync_public_vars();
}

void IMU_App_Loop(void)
{
    uint32_t now_ms = HAL_GetTick();
    (void)JY61P_Update(&s_jy61p, now_ms);
    imu_app_sync_public_vars();
}

void IMU_App_StartCalibration(void)
{
    JY61P_StartCalibration(&s_jy61p, HAL_GetTick());
    imu_app_sync_public_vars();
}

void IMU_App_SetYawZero(void)
{
    (void)JY61P_SetYawZero(&s_jy61p);
    imu_app_sync_public_vars();
}

const JY61P_Device_t *IMU_App_GetDevice(void)
{
    return &s_jy61p;
}
