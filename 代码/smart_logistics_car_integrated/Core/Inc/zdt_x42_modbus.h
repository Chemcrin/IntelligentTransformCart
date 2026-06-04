#ifndef ZDT_X42_MODBUS_H
#define ZDT_X42_MODBUS_H

#include "mecanum_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ZDT_FUNC_READ_INPUT_REGS          0x04u
#define ZDT_FUNC_WRITE_MULTI_REGS         0x10u

#define ZDT_REG_SPEED_FEEDBACK            0x0044u
#define ZDT_REG_ENABLE                    0x00E0u
#define ZDT_REG_SPEED_MODE                0x00E6u
#define ZDT_REG_STOP_IMMEDIATE            0x00FEu
#define ZDT_REG_SYNC_TRIGGER              0x00FFu

#define ZDT_ENABLE_CODE                   0xAB01u
#define ZDT_DISABLE_CODE                  0xAB00u
#define ZDT_STOP_CODE                     0x9800u
#define ZDT_SYNC_TRIGGER_CODE             0x6600u
#define ZDT_SYNC_WAIT_FLAG                0x0100u
#define ZDT_SYNC_NOW_FLAG                 0x0000u

typedef enum
{
    ZDT_DIR_CW = 0,
    ZDT_DIR_CCW = 1
} ZDTX42_Direction_t;

void ZDTX42_Init(void);
MecanumResult_t ZDTX42_EnableMotor(uint8_t motor_id, uint8_t enable);
MecanumResult_t ZDTX42_SetMotorSpeedRpm(uint8_t motor_id, float rpm_signed, uint8_t sync_wait);
MecanumResult_t ZDTX42_SetMotorSpeed(uint8_t motor_id, float speed_mps);
MecanumResult_t ZDTX42_StopMotor(uint8_t motor_id);
MecanumResult_t ZDTX42_StopAllMotors(void);
MecanumResult_t ZDTX42_EmergencyStopAll(void);
MecanumResult_t ZDTX42_SyncTrigger(void);
MecanumResult_t ZDTX42_ReadMotorSpeed(uint8_t motor_id, float *rpm_out);
MecanumResult_t ZDTX42_TestSingleMotor(uint8_t addr, float rpm);
MecanumResult_t ZDTX42_TestReadSpeed(uint8_t addr, float *rpm_out);

#ifdef __cplusplus
}
#endif

#endif
