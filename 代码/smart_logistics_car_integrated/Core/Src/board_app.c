#include "board_app.h"
#include "imu_app.h"
#include "laser_app.h"

volatile uint8_t g_board_app_flags = 0U;
volatile uint32_t g_board_app_loop_count = 0U;
volatile uint32_t g_board_app_last_tick_ms = 0U;

void Board_App_Init(void)
{
    IMU_App_Init();
    Laser_App_Init();
    g_board_app_flags |= BOARD_APP_FLAG_INIT_DONE;
}

void Board_App_Loop(void)
{
    sensor_update();
    g_board_app_loop_count++;
    g_board_app_last_tick_ms = HAL_GetTick();
}

void Board_App_UartRxCpltCallback(UART_HandleTypeDef *huart)
{
    (void)huart;
}

HAL_StatusTypeDef Board_App_ZeroYaw(void)
{
    return IMU_App_ZeroYaw();
}

HAL_StatusTypeDef Board_App_StartImuGyroCalibration(void)
{
    return IMU_App_StartGyroCalibration();
}

HAL_StatusTypeDef Board_App_CalibrateImuAccelerometer(void)
{
    return IMU_App_CalibrateAccelerometer();
}

void Board_App_GetSensorSnapshot(SENSOR_Snapshot_t *snapshot)
{
    sensor_get_snapshot(snapshot);
}
