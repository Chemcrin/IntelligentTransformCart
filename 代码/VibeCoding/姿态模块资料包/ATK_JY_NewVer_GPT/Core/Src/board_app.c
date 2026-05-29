#include "board_app.h"
#include "i2c.h"
#include "gpio.h"

volatile SENSOR_Snapshot_t g_sensor_snapshot;
volatile uint32_t g_sensor_fault_flags = 0U;

static uint32_t s_last_i2c_recover_ms = 0U;

static void board_app_update_snapshot(void)
{
    uint32_t flags = 0U;
    uint32_t now_ms = HAL_GetTick();

    if (g_imu_valid == 0U)
    {
        flags |= BOARD_SENSOR_FAULT_IMU_INVALID;
    }
    if (g_imu_calibrated == 0U)
    {
        flags |= BOARD_SENSOR_FAULT_IMU_NOT_CALIB;
    }
    if (g_laser_ok == 0U)
    {
        flags |= BOARD_SENSOR_FAULT_LASER_INVALID;
    }
    if (g_laser_is_too_close != 0U)
    {
        flags |= BOARD_SENSOR_FAULT_LASER_TOO_CLOSE;
    }

    g_sensor_snapshot.car_yaw_est_deg = g_imu_car_yaw_est_deg;
    g_sensor_snapshot.car_pitch_deg = g_imu_car_pitch_deg;
    g_sensor_snapshot.car_roll_deg = g_imu_car_roll_deg;
    g_sensor_snapshot.gyro_yaw_dps = g_imu_gyro_yaw_dps;
    g_sensor_snapshot.acc_forward_g = g_imu_acc_forward_g;
    g_sensor_snapshot.acc_right_g = g_imu_acc_right_g;
    g_sensor_snapshot.acc_down_g = g_imu_acc_down_g;
    g_sensor_snapshot.temperature_c = g_imu_temperature_c;
    g_sensor_snapshot.imu_valid = g_imu_valid;
    g_sensor_snapshot.imu_calibrated = g_imu_calibrated;

    g_sensor_snapshot.laser_raw_distance_mm = g_laser_raw_distance_mm;
    g_sensor_snapshot.laser_filtered_distance_mm = g_laser_filtered_distance_mm;
    g_sensor_snapshot.laser_ok = g_laser_ok;
    g_sensor_snapshot.laser_is_warning = g_laser_is_warning;
    g_sensor_snapshot.laser_is_slow = g_laser_is_slow;
    g_sensor_snapshot.laser_is_too_close = g_laser_is_too_close;

    g_sensor_snapshot.sensor_fault_flags = flags;
    g_sensor_snapshot.timestamp_ms = now_ms;
    g_sensor_fault_flags = flags;
}

void Board_App_Init(void)
{
    IMU_App_Init(&hi2c1);
    Laser_App_Init(&hi2c1);
    board_app_update_snapshot();
}

void Board_App_Loop(void)
{
    uint32_t now_ms;
    uint32_t flags;

    IMU_App_Loop();
    Laser_App_Loop();
    board_app_update_snapshot();

    now_ms = HAL_GetTick();
    flags = g_sensor_fault_flags;

    /* Conservative bus recovery: only when both I2C sensors are invalid after startup.
     * The recovery uses open-drain GPIO release/high and never drives PB6/PB7 push-pull high.
     */
    if ((now_ms > 3000U) &&
        ((flags & BOARD_SENSOR_FAULT_IMU_INVALID) != 0U) &&
        ((flags & BOARD_SENSOR_FAULT_LASER_INVALID) != 0U) &&
        ((uint32_t)(now_ms - s_last_i2c_recover_ms) > 1000U))
    {
        Board_App_I2C1_BusRecover();
        s_last_i2c_recover_ms = now_ms;
        IMU_App_Init(&hi2c1);
        Laser_App_Init(&hi2c1);
    }
}

void Board_App_GetSnapshot(SENSOR_Snapshot_t *out)
{
    if (out == 0)
    {
        return;
    }

    out->car_yaw_est_deg = g_sensor_snapshot.car_yaw_est_deg;
    out->car_pitch_deg = g_sensor_snapshot.car_pitch_deg;
    out->car_roll_deg = g_sensor_snapshot.car_roll_deg;
    out->gyro_yaw_dps = g_sensor_snapshot.gyro_yaw_dps;
    out->acc_forward_g = g_sensor_snapshot.acc_forward_g;
    out->acc_right_g = g_sensor_snapshot.acc_right_g;
    out->acc_down_g = g_sensor_snapshot.acc_down_g;
    out->temperature_c = g_sensor_snapshot.temperature_c;
    out->imu_valid = g_sensor_snapshot.imu_valid;
    out->imu_calibrated = g_sensor_snapshot.imu_calibrated;
    out->laser_raw_distance_mm = g_sensor_snapshot.laser_raw_distance_mm;
    out->laser_filtered_distance_mm = g_sensor_snapshot.laser_filtered_distance_mm;
    out->laser_ok = g_sensor_snapshot.laser_ok;
    out->laser_is_warning = g_sensor_snapshot.laser_is_warning;
    out->laser_is_slow = g_sensor_snapshot.laser_is_slow;
    out->laser_is_too_close = g_sensor_snapshot.laser_is_too_close;
    out->sensor_fault_flags = g_sensor_snapshot.sensor_fault_flags;
    out->timestamp_ms = g_sensor_snapshot.timestamp_ms;
}

uint32_t Board_App_GetFaultFlags(void)
{
    return g_sensor_fault_flags;
}

void Board_App_I2C1_BusRecover(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    uint8_t i;

    (void)HAL_I2C_DeInit(&hi2c1);

    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Release SCL/SDA through open-drain high. */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_SET);
    HAL_Delay(1U);

    for (i = 0U; i < 9U; i++)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
        HAL_Delay(1U);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
        HAL_Delay(1U);
    }

    /* Generate a STOP condition: SDA low while SCL released, then release SDA. */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
    HAL_Delay(1U);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_Delay(1U);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
    HAL_Delay(1U);

    MX_I2C1_Init();
}

void Board_App_UartRxCpltCallback(UART_HandleTypeDef *huart)
{
    (void)huart;
    /* Reserved for future motor/diagnostic UART callbacks. Sensor I2C polling is not run here. */
}
