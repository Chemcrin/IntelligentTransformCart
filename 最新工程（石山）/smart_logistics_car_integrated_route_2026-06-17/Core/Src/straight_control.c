#include "straight_control.h"

#include "mecanum_config.h"
#include "motor_manager.h"
#include "sensor_bridge.h"

#define STRAIGHT_PI                    3.14159265358979323846f
#define STRAIGHT_TWO_PI                6.28318530717958647692f

static uint8_t g_straight_enabled = STRAIGHT_CONTROL_ENABLE_DEFAULT;
static uint8_t g_straight_active = 0U;
static StraightPid_t g_yaw_pid;
static StraightPid_t g_lr_pid;
static StraightControlStatus_t g_straight_status;

static float straight_absf(float value)
{
    return (value >= 0.0f) ? value : -value;
}

static float straight_clampf(float value, float min_v, float max_v)
{
    if (value < min_v) {
        return min_v;
    }
    if (value > max_v) {
        return max_v;
    }
    return value;
}

static float straight_wrap_pi(float angle_rad)
{
    while (angle_rad > STRAIGHT_PI) {
        angle_rad -= STRAIGHT_TWO_PI;
    }
    while (angle_rad < -STRAIGHT_PI) {
        angle_rad += STRAIGHT_TWO_PI;
    }
    return angle_rad;
}

static void straight_pid_init(StraightPid_t *pid,
                              float kp,
                              float ki,
                              float kd,
                              float output_min,
                              float output_max,
                              float integral_min,
                              float integral_max,
                              float deadband)
{
    if (pid == 0) {
        return;
    }

    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->integral = 0.0f;
    pid->previous_error = 0.0f;
    pid->output_min = output_min;
    pid->output_max = output_max;
    pid->integral_min = integral_min;
    pid->integral_max = integral_max;
    pid->deadband = deadband;
    pid->first_update = 1U;
}

static void straight_pid_reset(StraightPid_t *pid)
{
    if (pid == 0) {
        return;
    }

    pid->integral = 0.0f;
    pid->previous_error = 0.0f;
    pid->first_update = 1U;
}

static uint8_t straight_should_engage(const MecanumVelocity_t *target)
{
    if (target == 0) {
        return 0U;
    }

    if (g_straight_enabled == 0U) {
        return 0U;
    }

    if (straight_absf(target->vy_mps) < STRAIGHT_CONTROL_MIN_SPEED_MPS) {
        return 0U;
    }

    if (straight_absf(target->vx_mps) > STRAIGHT_CONTROL_VX_EPS_MPS) {
        return 0U;
    }

    if (straight_absf(target->wz_radps) > STRAIGHT_CONTROL_WZ_EPS_RADPS) {
        return 0U;
    }

    return 1U;
}

static uint8_t straight_read_left_right_rpm(float *left_avg, float *right_avg)
{
    const ZDTX42_MotorState_t *states = MotorManager_GetStates();
    float left_sum = 0.0f;
    float right_sum = 0.0f;
    uint8_t left_count = 0U;
    uint8_t right_count = 0U;

    if ((states == 0) || (left_avg == 0) || (right_avg == 0)) {
        return 0U;
    }

    if (states[MOTOR_INDEX_FL].online != 0U) {
        left_sum += states[MOTOR_INDEX_FL].rpm_fb;
        left_count++;
    }
    if (states[MOTOR_INDEX_RL].online != 0U) {
        left_sum += states[MOTOR_INDEX_RL].rpm_fb;
        left_count++;
    }
    if (states[MOTOR_INDEX_FR].online != 0U) {
        right_sum += states[MOTOR_INDEX_FR].rpm_fb;
        right_count++;
    }
    if (states[MOTOR_INDEX_RR].online != 0U) {
        right_sum += states[MOTOR_INDEX_RR].rpm_fb;
        right_count++;
    }

    if ((left_count == 0U) || (right_count == 0U)) {
        return 0U;
    }

    *left_avg = left_sum / (float)left_count;
    *right_avg = right_sum / (float)right_count;
    return 1U;
}

void StraightControl_Init(void)
{
    straight_pid_init(&g_yaw_pid,
                      STRAIGHT_YAW_KP,
                      STRAIGHT_YAW_KI,
                      STRAIGHT_YAW_KD,
                      -STRAIGHT_CONTROL_MAX_WZ_RADPS,
                      STRAIGHT_CONTROL_MAX_WZ_RADPS,
                      -STRAIGHT_CONTROL_MAX_WZ_RADPS,
                      STRAIGHT_CONTROL_MAX_WZ_RADPS,
                      STRAIGHT_CONTROL_YAW_DEADBAND_RAD);

    straight_pid_init(&g_lr_pid,
                      STRAIGHT_LR_KP,
                      STRAIGHT_LR_KI,
                      STRAIGHT_LR_KD,
                      -STRAIGHT_LR_MAX_WZ_RADPS,
                      STRAIGHT_LR_MAX_WZ_RADPS,
                      -STRAIGHT_LR_MAX_WZ_RADPS,
                      STRAIGHT_LR_MAX_WZ_RADPS,
                      STRAIGHT_CONTROL_LR_DEADBAND_RPM);

    g_straight_enabled = STRAIGHT_CONTROL_ENABLE_DEFAULT;
    StraightControl_Reset();
}

void StraightControl_Enable(uint8_t enable)
{
    g_straight_enabled = (enable != 0U) ? 1U : 0U;
    if (g_straight_enabled == 0U) {
        StraightControl_Reset();
    }
    g_straight_status.enabled = g_straight_enabled;
}

uint8_t StraightControl_IsEnabled(void)
{
    return g_straight_enabled;
}

void StraightControl_Reset(void)
{
    straight_pid_reset(&g_yaw_pid);
    straight_pid_reset(&g_lr_pid);
    g_straight_active = 0U;
    g_straight_status = (StraightControlStatus_t){0};
    g_straight_status.enabled = g_straight_enabled;
}

float StraightControl_PIDCalculate(StraightPid_t *pid, float error, float dt_s)
{
    float derivative = 0.0f;
    float output;

    if (pid == 0) {
        return 0.0f;
    }

    if ((dt_s <= 0.0f) || (dt_s > 0.2f)) {
        dt_s = 0.02f;
    }

    if (straight_absf(error) < pid->deadband) {
        error = 0.0f;
    }

    pid->integral += error * dt_s;
    pid->integral = straight_clampf(pid->integral, pid->integral_min, pid->integral_max);

    if (pid->first_update != 0U) {
        derivative = 0.0f;
        pid->first_update = 0U;
    } else {
        derivative = (error - pid->previous_error) / dt_s;
    }
    pid->previous_error = error;

    output = (pid->kp * error) + (pid->ki * pid->integral) + (pid->kd * derivative);
    output = straight_clampf(output, pid->output_min, pid->output_max);

    return output;
}

MecanumVelocity_t StraightControl_Apply(const MecanumVelocity_t *target, float dt_s)
{
    MecanumVelocity_t out = {0.0f, 0.0f, 0.0f};
    SensorSnapshot_t snap;
    float left_rpm = 0.0f;
    float right_rpm = 0.0f;
    float yaw_error;
    float yaw_correction;
    float lr_error;
    float lr_correction = 0.0f;
    float correction;

    if (target == 0) {
        StraightControl_Reset();
        return out;
    }
    out = *target;

    if (straight_should_engage(target) == 0U) {
        StraightControl_Reset();
        return out;
    }

    Sensor_GetSnapshot(&snap);
    if ((snap.imu_valid == 0) || (snap.imu_calibrated == 0)) {
        StraightControl_Reset();
        g_straight_status.enabled = g_straight_enabled;
        g_straight_status.using_imu = 0U;
        return out;
    }

    if (g_straight_active == 0U) {
        g_straight_active = 1U;
        g_straight_status.target_yaw_rad = snap.yaw_rad;
        straight_pid_reset(&g_yaw_pid);
        straight_pid_reset(&g_lr_pid);
    }

    yaw_error = straight_wrap_pi(g_straight_status.target_yaw_rad - snap.yaw_rad);
    yaw_correction = StraightControl_PIDCalculate(&g_yaw_pid, yaw_error, dt_s);

    g_straight_status.using_motor_feedback = straight_read_left_right_rpm(&left_rpm, &right_rpm);
    if (g_straight_status.using_motor_feedback != 0U) {
        lr_error = (left_rpm - right_rpm) * STRAIGHT_LR_CORRECTION_SIGN;
        lr_correction = StraightControl_PIDCalculate(&g_lr_pid, lr_error, dt_s);
    } else {
        lr_error = 0.0f;
        straight_pid_reset(&g_lr_pid);
    }

    correction = straight_clampf(yaw_correction + lr_correction,
                                -STRAIGHT_CONTROL_MAX_WZ_RADPS,
                                STRAIGHT_CONTROL_MAX_WZ_RADPS);

    out.wz_radps = straight_clampf(target->wz_radps + correction,
                                   -MECANUM_MAX_WZ_RADPS,
                                   MECANUM_MAX_WZ_RADPS);

    g_straight_status.enabled = g_straight_enabled;
    g_straight_status.active = g_straight_active;
    g_straight_status.using_imu = 1U;
    g_straight_status.current_yaw_rad = snap.yaw_rad;
    g_straight_status.yaw_error_rad = yaw_error;
    g_straight_status.yaw_correction_radps = yaw_correction;
    g_straight_status.left_rpm_avg = left_rpm;
    g_straight_status.right_rpm_avg = right_rpm;
    g_straight_status.lr_error_rpm = lr_error;
    g_straight_status.lr_correction_radps = lr_correction;
    g_straight_status.output_wz_radps = correction;

    return out;
}

StraightControlStatus_t StraightControl_GetStatus(void)
{
    return g_straight_status;
}
