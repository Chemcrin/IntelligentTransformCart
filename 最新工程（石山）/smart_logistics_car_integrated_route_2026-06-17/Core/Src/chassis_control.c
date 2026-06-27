#include "chassis_control.h"
#include "mecanum_kinematics.h"
#include "motor_manager.h"
#include "mecanum_config.h"
#include "straight_control.h"

static MecanumState_t g_chassis_state = MECANUM_STATE_IDLE;
static MecanumVelocity_t g_target_vel = { 0.0f, 0.0f, 0.0f };
static WheelRpm_t g_last_rpm = { { 0.0f, 0.0f, 0.0f, 0.0f } };
static uint32_t g_last_cmd_tick = 0u;
static uint32_t g_last_ctrl_tick = 0u;
static uint32_t g_last_feedback_tick = 0u;
static uint8_t g_external_emergency = 0u;

static float clampf_chassis(float v, float min_v, float max_v)
{
    if (v < min_v) return min_v;
    if (v > max_v) return max_v;
    return v;
}

static uint8_t velocity_is_zero(const MecanumVelocity_t *vel)
{
    if (vel == 0) {
        return 1u;
    }

    return ((vel->vx_mps < 0.001f) && (vel->vx_mps > -0.001f) &&
            (vel->vy_mps < 0.001f) && (vel->vy_mps > -0.001f) &&
            (vel->wz_radps < 0.001f) && (vel->wz_radps > -0.001f));
}

void ChassisControl_Init(void)
{
    MotorManager_Init();
    StraightControl_Init();
    g_target_vel.vx_mps = 0.0f;
    g_target_vel.vy_mps = 0.0f;
    g_target_vel.wz_radps = 0.0f;
    g_last_cmd_tick = HAL_GetTick();
    g_last_ctrl_tick = HAL_GetTick();
    g_last_feedback_tick = HAL_GetTick();
    g_external_emergency = 0u;

    (void)MotorManager_EnableAll(1u);
    g_chassis_state = MECANUM_STATE_READY;
}

MecanumResult_t ChassisControl_SetVelocity(const MecanumVelocity_t *vel)
{
    if (vel == 0) {
        return MECANUM_BAD_PARAM;
    }

    g_external_emergency = 0u;
    g_target_vel.vx_mps = clampf_chassis(vel->vx_mps, -MECANUM_MAX_VX_MPS, MECANUM_MAX_VX_MPS);
    g_target_vel.vy_mps = clampf_chassis(vel->vy_mps, -MECANUM_MAX_VY_MPS, MECANUM_MAX_VY_MPS);
    g_target_vel.wz_radps = clampf_chassis(vel->wz_radps, -MECANUM_MAX_WZ_RADPS, MECANUM_MAX_WZ_RADPS);
    g_last_cmd_tick = HAL_GetTick();

    g_chassis_state = velocity_is_zero(&g_target_vel) ? MECANUM_STATE_READY : MECANUM_STATE_RUNNING;

    return MECANUM_OK;
}

MecanumResult_t ChassisControl_Stop(void)
{
    g_target_vel.vx_mps = 0.0f;
    g_target_vel.vy_mps = 0.0f;
    g_target_vel.wz_radps = 0.0f;
    StraightControl_Reset();
    g_last_cmd_tick = HAL_GetTick();
    (void)MotorManager_StopAll();
    g_chassis_state = MECANUM_STATE_STOP;

    return MECANUM_OK;
}

MecanumResult_t ChassisControl_EmergencyStop(void)
{
    g_target_vel.vx_mps = 0.0f;
    g_target_vel.vy_mps = 0.0f;
    g_target_vel.wz_radps = 0.0f;
    StraightControl_Reset();
    g_external_emergency = 0u;
    (void)MotorManager_StopAll();
    g_chassis_state = MECANUM_STATE_STOP;
    return MECANUM_OK;
}

MecanumResult_t ChassisControl_ClearEmergencyStop(void)
{
    g_external_emergency = 0u;
    StraightControl_Reset();
    (void)MotorManager_EnableAll(1u);
    g_chassis_state = MECANUM_STATE_READY;
    return MECANUM_OK;
}

MecanumResult_t ChassisControl_ClearFault(void)
{
    StraightControl_Reset();
    (void)MotorManager_EnableAll(1u);
    g_chassis_state = MECANUM_STATE_READY;
    return MECANUM_OK;
}

void ChassisControl_PeriodicTask(void)
{
    uint32_t now = HAL_GetTick();
    MecanumWheelSpeed_t wheel_mps;
    WheelRpm_t wheel_rpm;
    MecanumVelocity_t ctrl_vel;
    float dt_s;

    if (g_external_emergency != 0u) {
        g_external_emergency = 0u;
    }

    if ((g_chassis_state == MECANUM_STATE_EMERGENCY_STOP) || (g_chassis_state == MECANUM_STATE_FAULT)) {
        g_chassis_state = MECANUM_STATE_READY;
    }

    if ((now - g_last_feedback_tick) >= MOTOR_FEEDBACK_PERIOD_MS) {
        g_last_feedback_tick = now;
        (void)MotorManager_UpdateFeedback();
    }

#if (CHASSIS_COMMAND_TIMEOUT_MS > 0u)
    if ((now - g_last_cmd_tick) > CHASSIS_COMMAND_TIMEOUT_MS) {
        (void)ChassisControl_Stop();
        return;
    }
#endif

    if ((now - g_last_ctrl_tick) < CHASSIS_CTRL_PERIOD_MS) {
        return;
    }

    dt_s = (float)(now - g_last_ctrl_tick) * 0.001f;
    g_last_ctrl_tick = now;

    ctrl_vel = StraightControl_Apply(&g_target_vel, dt_s);
    Mecanum_InverseKinematics(&ctrl_vel, &wheel_mps, &wheel_rpm);
    Mecanum_LimitWheelSpeed(&wheel_rpm, &g_last_rpm, dt_s);

    (void)MotorManager_SetWheelRpm(&wheel_rpm);
    g_last_rpm = wheel_rpm;
    if (velocity_is_zero(&g_target_vel) != 0u) {
        g_chassis_state = MECANUM_STATE_READY;
    } else {
        g_chassis_state = MECANUM_STATE_RUNNING;
    }
}

MecanumState_t ChassisControl_GetState(void)
{
    return g_chassis_state;
}

MecanumVelocity_t ChassisControl_GetTargetVelocity(void)
{
    return g_target_vel;
}

void ChassisControl_SetExternalEmergency(uint8_t active)
{
    g_external_emergency = active;
}


void ChassisControl_EnableStraightControl(uint8_t enable)
{
    StraightControl_Enable(enable);
}

uint8_t ChassisControl_IsStraightControlEnabled(void)
{
    return StraightControl_IsEnabled();
}


