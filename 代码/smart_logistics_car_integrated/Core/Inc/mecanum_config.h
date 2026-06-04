#ifndef MECANUM_CONFIG_H
#define MECANUM_CONFIG_H

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MOTOR_ADDR_BROAD                  0x00u
#define MOTOR_ADDR_FL                     0x01u
#define MOTOR_ADDR_FR                     0x02u
#define MOTOR_ADDR_RL                     0x03u
#define MOTOR_ADDR_RR                     0x04u
#define MOTOR_COUNT                       4u

#define MECANUM_WHEEL_DIAMETER_M          0.075f
#define MECANUM_TRACK_WIDTH_M             0.320f
#define MECANUM_WHEEL_BASE_M              0.220f
#define MECANUM_K_M                       ((MECANUM_TRACK_WIDTH_M + MECANUM_WHEEL_BASE_M) * 0.5f)

/* Change this only if Mecanum_Rotate(+wz) is opposite on the real chassis. */
#define MECANUM_WZ_SIGN                   (1.0f)

#define MECANUM_MAX_VX_MPS                0.60f
#define MECANUM_MAX_VY_MPS                0.60f
#define MECANUM_MAX_WZ_RADPS              1.50f
#define RPM_LIMIT                         500.0f
#define ACC_LIMIT_RPM_S                   2000.0f

/* ZDT speed scaling can vary by firmware version; verify with one motor first. */
#define ZDT_SPEED_MODE_SCALE              1.0f
#define ZDT_SPEED_FEEDBACK_SCALE          1.0f
#define ZDT_DEFAULT_ACCEL_RPM_S           1000u

#define MODBUS_BAUDRATE                   115200u
#define MODBUS_RESPONSE_TIMEOUT_MS        10u
#define MODBUS_INTER_FRAME_GAP_MS         2u
#define MODBUS_TX_SETTLE_US               20u

#define CHASSIS_CTRL_PERIOD_MS            20u
#define MOTOR_FEEDBACK_PERIOD_MS          50u
#define CHASSIS_COMMAND_TIMEOUT_MS        500u
#define MOTOR_TIMEOUT_FAULT_LIMIT         3u

/*
 * 0: no extra MCU GPIO for RS485 direction. Use this for the official wiring
 *    with an external/board RS485 module connected to A/B.
 * 1: drive DE/RE with a MCU GPIO named RS485_DIR in CubeMX.
 */
#define MODBUS_USE_RS485_DIR_GPIO         0u

#if (MODBUS_USE_RS485_DIR_GPIO != 0u)
#ifndef RS485_DIR_GPIO_Port
#define RS485_DIR_GPIO_Port               GPIOB
#endif

#ifndef RS485_DIR_Pin
#define RS485_DIR_Pin                     GPIO_PIN_1
#endif

#define RS485_DIR_TX_LEVEL                GPIO_PIN_SET
#define RS485_DIR_RX_LEVEL                GPIO_PIN_RESET
#endif

#ifdef __cplusplus
}
#endif

#endif
