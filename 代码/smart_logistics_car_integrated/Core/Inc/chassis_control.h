#ifndef CHASSIS_CONTROL_H
#define CHASSIS_CONTROL_H

#include "mecanum_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void ChassisControl_Init(void);
MecanumResult_t ChassisControl_SetVelocity(const MecanumVelocity_t *vel);
MecanumResult_t ChassisControl_Stop(void);
MecanumResult_t ChassisControl_EmergencyStop(void);
MecanumResult_t ChassisControl_ClearEmergencyStop(void);
MecanumResult_t ChassisControl_ClearFault(void);
void ChassisControl_PeriodicTask(void);
MecanumState_t ChassisControl_GetState(void);
MecanumVelocity_t ChassisControl_GetTargetVelocity(void);
void ChassisControl_SetExternalEmergency(uint8_t active);

#ifdef __cplusplus
}
#endif

#endif
