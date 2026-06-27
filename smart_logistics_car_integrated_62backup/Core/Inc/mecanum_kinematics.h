#ifndef MECANUM_KINEMATICS_H
#define MECANUM_KINEMATICS_H

#include "mecanum_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void Mecanum_InverseKinematics(const MecanumVelocity_t *vel, MecanumWheelSpeed_t *wheel_mps, WheelRpm_t *wheel_rpm);
void Mecanum_ForwardKinematics(const WheelRpm_t *wheel_rpm, MecanumVelocity_t *vel_est);
float Mecanum_WheelMpsToRpm(float wheel_mps);
float Mecanum_RpmToWheelMps(float rpm);
void Mecanum_LimitWheelSpeed(WheelRpm_t *wheel_rpm, const WheelRpm_t *last_rpm, float dt_s);

#ifdef __cplusplus
}
#endif

#endif
