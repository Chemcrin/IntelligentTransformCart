#ifndef BOARD_APP_H
#define BOARD_APP_H

#include "sensors.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BOARD_APP_FLAG_INIT_DONE 0x01U

extern volatile uint8_t g_board_app_flags;
extern volatile uint32_t g_board_app_loop_count;
extern volatile uint32_t g_board_app_last_tick_ms;

void Board_App_Init(void);
void Board_App_Loop(void);
void Board_App_UartRxCpltCallback(UART_HandleTypeDef *huart);
HAL_StatusTypeDef Board_App_ZeroYaw(void);
HAL_StatusTypeDef Board_App_StartImuGyroCalibration(void);
HAL_StatusTypeDef Board_App_CalibrateImuAccelerometer(void);
void Board_App_GetSensorSnapshot(SENSOR_Snapshot_t *snapshot);

#ifdef __cplusplus
}
#endif

#endif
