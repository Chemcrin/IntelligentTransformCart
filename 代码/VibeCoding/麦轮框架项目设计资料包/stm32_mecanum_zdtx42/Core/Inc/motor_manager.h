#ifndef MOTOR_MANAGER_H
#define MOTOR_MANAGER_H

#include "mecanum_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void MotorManager_Init(void);
MecanumResult_t MotorManager_EnableAll(uint8_t enable);
MecanumResult_t MotorManager_SetWheelRpm(const WheelRpm_t *rpm);
MecanumResult_t MotorManager_StopAll(void);
MecanumResult_t MotorManager_EmergencyStop(void);
MecanumResult_t MotorManager_UpdateFeedback(void);
const ZDTX42_MotorState_t *MotorManager_GetStates(void);
void MotorManager_SetDirectionSigns(const int8_t dir_sign[4]);
const int8_t *MotorManager_GetDirectionSigns(void);
uint8_t MotorManager_HasFault(void);
MecanumResult_t Mecanum_TestMotorDirection(float test_rpm);

#ifdef __cplusplus
}
#endif

#endif
