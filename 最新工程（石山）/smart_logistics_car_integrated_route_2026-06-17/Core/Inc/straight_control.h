#ifndef STRAIGHT_CONTROL_H
#define STRAIGHT_CONTROL_H

#include "mecanum_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    float kp;
    float ki;
    float kd;
    float integral;
    float previous_error;
    float output_min;
    float output_max;
    float integral_min;
    float integral_max;
    float deadband;
    uint8_t first_update;
} StraightPid_t;

typedef struct
{
    uint8_t enabled;
    uint8_t active;
    uint8_t using_imu;
    uint8_t using_motor_feedback;
    float target_yaw_rad;
    float current_yaw_rad;
    float yaw_error_rad;
    float yaw_correction_radps;
    float left_rpm_avg;
    float right_rpm_avg;
    float lr_error_rpm;
    float lr_correction_radps;
    float output_wz_radps;
} StraightControlStatus_t;

void StraightControl_Init(void);
void StraightControl_Enable(uint8_t enable);
uint8_t StraightControl_IsEnabled(void);
void StraightControl_Reset(void);
float StraightControl_PIDCalculate(StraightPid_t *pid, float error, float dt_s);
MecanumVelocity_t StraightControl_Apply(const MecanumVelocity_t *target, float dt_s);
StraightControlStatus_t StraightControl_GetStatus(void);

#ifdef __cplusplus
}
#endif

#endif
