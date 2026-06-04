#include "mecanum_kinematics.h"
#include "mecanum_config.h"

#define MECANUM_PI 3.14159265358979323846f

static float absf_local(float v)
{
    return (v >= 0.0f) ? v : -v;
}

static float clampf_local(float v, float min_v, float max_v)
{
    if (v < min_v) return min_v;
    if (v > max_v) return max_v;
    return v;
}

float Mecanum_WheelMpsToRpm(float wheel_mps)
{
    return wheel_mps / (MECANUM_PI * MECANUM_WHEEL_DIAMETER_M) * 60.0f;
}

float Mecanum_RpmToWheelMps(float rpm)
{
    return rpm * (MECANUM_PI * MECANUM_WHEEL_DIAMETER_M) / 60.0f;
}

void Mecanum_InverseKinematics(const MecanumVelocity_t *vel, MecanumWheelSpeed_t *wheel_mps, WheelRpm_t *wheel_rpm)
{
    float wz_eff;

    if ((vel == 0) || (wheel_mps == 0) || (wheel_rpm == 0)) {
        return;
    }

    wz_eff = MECANUM_WZ_SIGN * vel->wz_radps;

    wheel_mps->wheel_fl_mps = vel->vy_mps - vel->vx_mps - MECANUM_K_M * wz_eff;
    wheel_mps->wheel_fr_mps = vel->vy_mps + vel->vx_mps + MECANUM_K_M * wz_eff;
    wheel_mps->wheel_rl_mps = vel->vy_mps + vel->vx_mps - MECANUM_K_M * wz_eff;
    wheel_mps->wheel_rr_mps = vel->vy_mps - vel->vx_mps + MECANUM_K_M * wz_eff;

    wheel_rpm->wheel_rpm[MOTOR_INDEX_FL] = Mecanum_WheelMpsToRpm(wheel_mps->wheel_fl_mps);
    wheel_rpm->wheel_rpm[MOTOR_INDEX_FR] = Mecanum_WheelMpsToRpm(wheel_mps->wheel_fr_mps);
    wheel_rpm->wheel_rpm[MOTOR_INDEX_RL] = Mecanum_WheelMpsToRpm(wheel_mps->wheel_rl_mps);
    wheel_rpm->wheel_rpm[MOTOR_INDEX_RR] = Mecanum_WheelMpsToRpm(wheel_mps->wheel_rr_mps);
}

void Mecanum_ForwardKinematics(const WheelRpm_t *wheel_rpm, MecanumVelocity_t *vel_est)
{
    float fl;
    float fr;
    float rl;
    float rr;

    if ((wheel_rpm == 0) || (vel_est == 0)) {
        return;
    }

    fl = Mecanum_RpmToWheelMps(wheel_rpm->wheel_rpm[MOTOR_INDEX_FL]);
    fr = Mecanum_RpmToWheelMps(wheel_rpm->wheel_rpm[MOTOR_INDEX_FR]);
    rl = Mecanum_RpmToWheelMps(wheel_rpm->wheel_rpm[MOTOR_INDEX_RL]);
    rr = Mecanum_RpmToWheelMps(wheel_rpm->wheel_rpm[MOTOR_INDEX_RR]);

    vel_est->vy_mps = (fl + fr + rl + rr) * 0.25f;
    vel_est->vx_mps = (-fl + fr + rl - rr) * 0.25f;
    vel_est->wz_radps = ((-fl + fr - rl + rr) / (4.0f * MECANUM_K_M)) * MECANUM_WZ_SIGN;
}

void Mecanum_LimitWheelSpeed(WheelRpm_t *wheel_rpm, const WheelRpm_t *last_rpm, float dt_s)
{
    float max_abs = 0.0f;
    float scale = 1.0f;
    float max_delta;
    uint8_t i;

    if (wheel_rpm == 0) {
        return;
    }

    for (i = 0u; i < MOTOR_COUNT; i++) {
        float a = absf_local(wheel_rpm->wheel_rpm[i]);
        if (a > max_abs) {
            max_abs = a;
        }
    }

    if (max_abs > RPM_LIMIT) {
        scale = RPM_LIMIT / max_abs;
        for (i = 0u; i < MOTOR_COUNT; i++) {
            wheel_rpm->wheel_rpm[i] *= scale;
        }
    }

    if ((last_rpm == 0) || (dt_s <= 0.0f)) {
        return;
    }

    max_delta = ACC_LIMIT_RPM_S * dt_s;
    for (i = 0u; i < MOTOR_COUNT; i++) {
        float min_rpm = last_rpm->wheel_rpm[i] - max_delta;
        float max_rpm = last_rpm->wheel_rpm[i] + max_delta;
        wheel_rpm->wheel_rpm[i] = clampf_local(wheel_rpm->wheel_rpm[i], min_rpm, max_rpm);
    }
}
