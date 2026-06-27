#include "imu_app.h"

extern I2C_HandleTypeDef hi2c1;

static JY61P_Device_t g_jy61p;

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

volatile uint32_t g_imu_update_count = 0U;
volatile uint32_t g_imu_last_tick_ms = 0U;
volatile uint16_t g_imu_error_count = 0U;
volatile JY61P_State_t g_imu_state = JY61P_STATE_RESET;
volatile JY61P_Error_t g_imu_last_error = JY61P_ERROR_NONE;

volatile float g_imu_roll_deg = 0.0f;
volatile float g_imu_pitch_deg = 0.0f;
volatile float g_imu_yaw_deg = 0.0f;
volatile float g_imu_ax_g = 0.0f;
volatile float g_imu_ay_g = 0.0f;
volatile float g_imu_az_g = 0.0f;
volatile float g_imu_gx_dps = 0.0f;
volatile float g_imu_gy_dps = 0.0f;
volatile float g_imu_gz_dps = 0.0f;
volatile float g_imu_temp_c = 0.0f;

static void imu_app_publish(uint32_t now_ms)
{
    const JY61P_Attitude_t *att = &g_jy61p.attitude;
    const JY61P_Data_t *raw = &g_jy61p.data;

    imu_valid = (g_jy61p.valid != 0U) && (JY61P_IsTimedOut(&g_jy61p, now_ms) == 0U);
    imu_calibrated = g_jy61p.calib.calibrated;

    car_yaw_est_deg = att->car_yaw_est_deg;
    car_pitch_deg = att->car_pitch_deg;
    car_roll_deg = att->car_roll_deg;
    gyro_yaw_dps = att->gyro_yaw_dps;
    acc_forward_g = att->acc_forward_g;
    acc_right_g = att->acc_right_g;
    acc_down_g = att->acc_down_g;
    temperature_c = att->temperature_c;

    g_imu_roll_deg = raw->roll_deg;
    g_imu_pitch_deg = raw->pitch_deg;
    g_imu_yaw_deg = raw->yaw_deg;
    g_imu_ax_g = raw->ax_g;
    g_imu_ay_g = raw->ay_g;
    g_imu_az_g = raw->az_g;
    g_imu_gx_dps = raw->gx_dps;
    g_imu_gy_dps = raw->gy_dps;
    g_imu_gz_dps = raw->gz_dps;
    g_imu_temp_c = raw->temperature_c;

    g_imu_state = g_jy61p.state;
    g_imu_last_error = g_jy61p.last_error;
    g_imu_error_count = g_jy61p.error_count;
    g_imu_last_tick_ms = g_jy61p.last_valid_ms;
}

void IMU_App_Init(void)
{
    JY61P_Init(&g_jy61p, &hi2c1);
    (void)JY61P_StartGyroCalibration(&g_jy61p);
    imu_app_publish(HAL_GetTick());
}

void IMU_App_Loop(void)
{
    uint32_t now_ms = HAL_GetTick();

    if (JY61P_Poll(&g_jy61p, now_ms) == HAL_OK)
    {
        g_imu_update_count++;
    }

    imu_app_publish(now_ms);
}

HAL_StatusTypeDef IMU_App_ZeroYaw(void)
{
    HAL_StatusTypeDef status = JY61P_ZeroYawReference(&g_jy61p);
    imu_app_publish(HAL_GetTick());
    return status;
}

HAL_StatusTypeDef IMU_App_StartGyroCalibration(void)
{
    HAL_StatusTypeDef status = JY61P_StartGyroCalibration(&g_jy61p);
    imu_app_publish(HAL_GetTick());
    return status;
}

HAL_StatusTypeDef IMU_App_CalibrateAccelerometer(void)
{
    return JY61P_HardwareCalibrateAccelerometer(&g_jy61p);
}

const JY61P_Device_t *IMU_App_GetDevice(void)
{
    return &g_jy61p;
}
