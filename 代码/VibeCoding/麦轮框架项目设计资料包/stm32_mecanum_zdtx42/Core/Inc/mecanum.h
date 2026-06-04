#ifndef MECANUM_H
#define MECANUM_H

#include "mecanum_types.h"
#include "stm32f1xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

void Mecanum_Init(UART_HandleTypeDef *motor_uart);
MecanumResult_t Mecanum_SetVelocity(float vx_mps, float vy_mps, float wz_radps);
MecanumResult_t Mecanum_MoveForward(float speed_mps);
MecanumResult_t Mecanum_MoveBackward(float speed_mps);
MecanumResult_t Mecanum_MoveLeft(float speed_mps);
MecanumResult_t Mecanum_MoveRight(float speed_mps);
MecanumResult_t Mecanum_MoveVector(float vx_mps, float vy_mps);
MecanumResult_t Mecanum_Rotate(float wz_radps);
MecanumResult_t Mecanum_Stop(void);
MecanumResult_t Mecanum_EmergencyStop(void);
MecanumResult_t Mecanum_ClearEmergencyStop(void);
MecanumResult_t Mecanum_ClearFault(void);
void Mecanum_PeriodicTask(void);
MecanumState_t Mecanum_GetState(void);
MecanumVelocity_t Mecanum_GetTargetVelocity(void);

MecanumResult_t Mecanum_TestMoveForward(float speed_mps);
MecanumResult_t Mecanum_TestMoveLeft(float speed_mps);
MecanumResult_t Mecanum_TestRotate(float wz_radps);
MecanumResult_t RS485_TestDirectionLevel(uint8_t motor_addr, float *rpm_out);

#ifdef __cplusplus
}
#endif

#endif
