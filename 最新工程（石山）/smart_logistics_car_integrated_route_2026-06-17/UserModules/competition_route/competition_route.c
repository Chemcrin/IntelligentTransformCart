#include "competition_route.h"

#include "chassis_control.h"
#include "config.h"
#include "mecanum.h"
#include "remote_cmd.h"
#include "stm32f1xx_hal.h"
#include <stddef.h>

#define COMP_ROUTE_DEG_TO_RAD 0.01745329251994329577f

static const CompetitionRouteStep_t g_default_route[] = {
    { COMP_ROUTE_ACTION_FORWARD_MM,  3300, COMPETITION_ROUTE_FORWARD_SPEED_MPS, 0U },
    { COMP_ROUTE_ACTION_ROTATE_DEG,     90, COMPETITION_ROUTE_ROTATE_MAX_WZ_RADPS, 0U },
    { COMP_ROUTE_ACTION_FORWARD_MM,  1000, COMPETITION_ROUTE_FORWARD_SPEED_MPS, 0U },
    { COMP_ROUTE_ACTION_ROTATE_DEG,     90, COMPETITION_ROUTE_ROTATE_MAX_WZ_RADPS, 0U },
    { COMP_ROUTE_ACTION_FORWARD_MM,  1000, COMPETITION_ROUTE_FORWARD_SPEED_MPS, 0U },
    { COMP_ROUTE_ACTION_FORWARD_MM, -1000, COMPETITION_ROUTE_FORWARD_SPEED_MPS, 0U },
    { COMP_ROUTE_ACTION_FORWARD_MM,   500, COMPETITION_ROUTE_FORWARD_SPEED_MPS, 0U },
    { COMP_ROUTE_ACTION_STRAFE_MM,   -350, COMPETITION_ROUTE_STRAFE_SPEED_MPS, 0U },
    { COMP_ROUTE_ACTION_FORWARD_MM,  1500, COMPETITION_ROUTE_FORWARD_SPEED_MPS, 0U },
    { COMP_ROUTE_ACTION_STRAFE_MM,    400, COMPETITION_ROUTE_STRAFE_SPEED_MPS, 0U },
    { COMP_ROUTE_ACTION_FORWARD_MM,   700, COMPETITION_ROUTE_FORWARD_SPEED_MPS, 0U },
    { COMP_ROUTE_ACTION_STOP,           0, 0.0f, 0U }
};

static const CompetitionRouteStep_t *g_route_steps = g_default_route;
static uint16_t g_route_step_count = (uint16_t)(sizeof(g_default_route) / sizeof(g_default_route[0]));
static CompetitionRouteStatus_t g_route_status;
static uint8_t g_step_started = 0U;
static uint8_t g_rotate_stable_count = 0U;
static uint32_t g_boot_ms = 0U;

static int32_t route_abs_i32(int32_t value)
{
    return (value >= 0) ? value : -value;
}

static float route_signf(float value)
{
    return (value >= 0.0f) ? 1.0f : -1.0f;
}

static float route_default_speed(const CompetitionRouteStep_t *step)
{
    if (step == NULL) {
        return 0.0f;
    }

    if (step->speed > 0.001f) {
        return step->speed;
    }

    switch (step->action) {
    case COMP_ROUTE_ACTION_FORWARD_MM:
        return COMPETITION_ROUTE_FORWARD_SPEED_MPS;
    case COMP_ROUTE_ACTION_STRAFE_MM:
        return COMPETITION_ROUTE_STRAFE_SPEED_MPS;
    case COMP_ROUTE_ACTION_ROTATE_DEG:
        return COMPETITION_ROUTE_ROTATE_MAX_WZ_RADPS;
    default:
        return 0.0f;
    }
}

static uint32_t route_calc_duration_ms(const CompetitionRouteStep_t *step)
{
    float speed;
    int32_t abs_value;
    uint32_t duration_ms;

    if (step == NULL) {
        return 0U;
    }

    if (step->timeout_ms != 0U) {
        return step->timeout_ms;
    }

    switch (step->action) {
    case COMP_ROUTE_ACTION_FORWARD_MM:
    case COMP_ROUTE_ACTION_STRAFE_MM:
        speed = route_default_speed(step);
        if (speed < 0.01f) {
            speed = 0.01f;
        }
        abs_value = route_abs_i32(step->value);
        duration_ms = (uint32_t)(((float)abs_value / speed) + 0.5f);
        if (duration_ms < 20U) {
            duration_ms = 20U;
        }
        return duration_ms;

    case COMP_ROUTE_ACTION_ROTATE_DEG:
        speed = route_default_speed(step);
        if (speed < 0.05f) {
            speed = 0.05f;
        }
        abs_value = route_abs_i32(step->value);
        duration_ms = (uint32_t)((((float)abs_value * COMP_ROUTE_DEG_TO_RAD) / speed) * 1000.0f + 0.5f);
        if (duration_ms < 20U) {
            duration_ms = 20U;
        }
        return duration_ms;

    case COMP_ROUTE_ACTION_WAIT_MS:
        return (step->value > 0) ? (uint32_t)step->value : 0U;

    default:
        return 0U;
    }
}

static void route_stop_motion(void)
{
    (void)Mecanum_SetVelocity(0.0f, 0.0f, 0.0f);
}

static void route_abort(CompetitionRouteAbortReason_t reason)
{
    if ((g_route_status.state == COMP_ROUTE_STATE_ABORTED) ||
        (g_route_status.state == COMP_ROUTE_STATE_FINISHED)) {
        return;
    }

    g_route_status.abort_reason = reason;
    g_route_status.state = COMP_ROUTE_STATE_ABORTED;
    g_step_started = 0U;
    g_rotate_stable_count = 0U;

    route_stop_motion();
}

static uint8_t route_update_safety(uint8_t running)
{
#if (FIXED_ROUTE_STANDALONE_ENABLE == 0U)
    RemoteCmdStatus_t remote = RemoteCmd_GetStatus();
#endif

    (void)running;
#if (FIXED_ROUTE_STANDALONE_ENABLE != 0U)
    g_route_status.rpi_link_seen = 1U;
#else
    if (remote.link_state == REMOTE_CMD_LINK_ALIVE) {
        g_route_status.rpi_link_seen = 1U;
    }
#endif

    return 1U;
}

static uint8_t route_ready_to_start(void)
{
#if (FIXED_ROUTE_STANDALONE_ENABLE == 0U)
    RemoteCmdStatus_t remote;
#endif
    uint32_t now_ms = HAL_GetTick();

    if ((now_ms - g_boot_ms) < COMPETITION_ROUTE_BOOT_WAIT_MS) {
        return 0U;
    }

#if (FIXED_ROUTE_STANDALONE_ENABLE != 0U)
    g_route_status.rpi_link_seen = 1U;
#else
    remote = RemoteCmd_GetStatus();
    if (remote.link_state == REMOTE_CMD_LINK_ALIVE) {
        g_route_status.rpi_link_seen = 1U;
    }
#endif

    ChassisControl_SetExternalEmergency(0U);
    (void)Mecanum_ClearEmergencyStop();
    (void)Mecanum_ClearFault();
    return 1U;
}

static void route_advance_step(void)
{
    (void)Mecanum_SetVelocity(0.0f, 0.0f, 0.0f);
    g_step_started = 0U;
    g_rotate_stable_count = 0U;

    if (g_route_status.step_index < g_route_step_count) {
        g_route_status.step_index++;
    }

    if (g_route_status.step_index >= g_route_step_count) {
        g_route_status.state = COMP_ROUTE_STATE_FINISHED;
        g_route_status.current_action = COMP_ROUTE_ACTION_STOP;
    }
}

static void route_enter_step(const CompetitionRouteStep_t *step)
{
    uint32_t now_ms = HAL_GetTick();
    float target_delta_rad;

    if (step == NULL) {
        route_abort(COMP_ROUTE_ABORT_CMD_ERROR);
        return;
    }

    g_step_started = 1U;
    g_rotate_stable_count = 0U;
    g_route_status.current_action = step->action;
    g_route_status.step_start_ms = now_ms;
    g_route_status.step_elapsed_ms = 0U;
    g_route_status.step_duration_ms = route_calc_duration_ms(step);
    g_route_status.rotate_error_rad = 0.0f;

    switch (step->action) {
    case COMP_ROUTE_ACTION_FORWARD_MM:
        (void)Mecanum_SetVelocity(0.0f,
                                  route_signf((float)step->value) * route_default_speed(step),
                                  0.0f);
        break;

    case COMP_ROUTE_ACTION_STRAFE_MM:
        (void)Mecanum_SetVelocity(route_signf((float)step->value) * route_default_speed(step),
                                  0.0f,
                                  0.0f);
        break;

    case COMP_ROUTE_ACTION_ROTATE_DEG:
        target_delta_rad = (float)step->value * COMP_ROUTE_DEG_TO_RAD;
        g_route_status.rotate_target_yaw_rad = 0.0f;
        (void)Mecanum_SetVelocity(0.0f,
                                  0.0f,
                                  route_signf(target_delta_rad) * route_default_speed(step));
        break;

    case COMP_ROUTE_ACTION_WAIT_MS:
    case COMP_ROUTE_ACTION_STOP:
    default:
        route_stop_motion();
        break;
    }
}

static void route_run_translation_step(const CompetitionRouteStep_t *step, uint32_t elapsed_ms)
{
    float speed;

    if (step == NULL) {
        route_abort(COMP_ROUTE_ABORT_CMD_ERROR);
        return;
    }

    if (elapsed_ms >= g_route_status.step_duration_ms) {
        route_advance_step();
        return;
    }

    speed = route_default_speed(step);
    if (step->action == COMP_ROUTE_ACTION_FORWARD_MM) {
        (void)Mecanum_SetVelocity(0.0f,
                                  route_signf((float)step->value) * speed,
                                  0.0f);
    } else if (step->action == COMP_ROUTE_ACTION_STRAFE_MM) {
        (void)Mecanum_SetVelocity(route_signf((float)step->value) * speed,
                                  0.0f,
                                  0.0f);
    } else {
        route_abort(COMP_ROUTE_ABORT_CMD_ERROR);
    }
}

static void route_run_rotation_step(const CompetitionRouteStep_t *step, uint32_t elapsed_ms)
{
    float speed;
    float target_delta_rad;

    if (step == NULL) {
        route_abort(COMP_ROUTE_ABORT_CMD_ERROR);
        return;
    }

    if (elapsed_ms >= g_route_status.step_duration_ms) {
        route_advance_step();
        return;
    }

    speed = route_default_speed(step);
    target_delta_rad = (float)step->value * COMP_ROUTE_DEG_TO_RAD;
    g_route_status.rotate_error_rad = 0.0f;
    (void)Mecanum_SetVelocity(0.0f, 0.0f, route_signf(target_delta_rad) * speed);
}

static void route_run_current_step(void)
{
    const CompetitionRouteStep_t *step;
    uint32_t now_ms;
    uint32_t elapsed_ms;

    if (g_route_status.step_index >= g_route_step_count) {
        route_stop_motion();
        g_route_status.state = COMP_ROUTE_STATE_FINISHED;
        return;
    }

    step = &g_route_steps[g_route_status.step_index];
    if (g_step_started == 0U) {
        route_enter_step(step);
        if (g_route_status.state == COMP_ROUTE_STATE_ABORTED) {
            return;
        }
    }

    now_ms = HAL_GetTick();
    elapsed_ms = now_ms - g_route_status.step_start_ms;
    g_route_status.step_elapsed_ms = elapsed_ms;
    g_route_status.current_action = step->action;

    switch (step->action) {
    case COMP_ROUTE_ACTION_FORWARD_MM:
    case COMP_ROUTE_ACTION_STRAFE_MM:
        route_run_translation_step(step, elapsed_ms);
        break;

    case COMP_ROUTE_ACTION_ROTATE_DEG:
        route_run_rotation_step(step, elapsed_ms);
        break;

    case COMP_ROUTE_ACTION_WAIT_MS:
        route_stop_motion();
        if (elapsed_ms >= g_route_status.step_duration_ms) {
            route_advance_step();
        }
        break;

    case COMP_ROUTE_ACTION_STOP:
        route_stop_motion();
        g_route_status.state = COMP_ROUTE_STATE_FINISHED;
        g_step_started = 0U;
        break;

    default:
        route_abort(COMP_ROUTE_ABORT_CMD_ERROR);
        break;
    }
}

void CompetitionRoute_Init(void)
{
    g_boot_ms = HAL_GetTick();
    g_route_steps = g_default_route;
    g_route_step_count = (uint16_t)(sizeof(g_default_route) / sizeof(g_default_route[0]));
    g_step_started = 0U;
    g_rotate_stable_count = 0U;

    g_route_status.state = COMP_ROUTE_STATE_IDLE;
    g_route_status.abort_reason = COMP_ROUTE_ABORT_NONE;
    g_route_status.current_action = COMP_ROUTE_ACTION_STOP;
    g_route_status.step_index = 0U;
    g_route_status.step_count = g_route_step_count;
    g_route_status.step_start_ms = 0U;
    g_route_status.step_elapsed_ms = 0U;
    g_route_status.step_duration_ms = 0U;
    g_route_status.rotate_target_yaw_rad = 0.0f;
    g_route_status.rotate_error_rad = 0.0f;
    g_route_status.rpi_link_seen = 0U;

#if (COMPETITION_ROUTE_AUTO_START != 0U)
    CompetitionRoute_StartDefault();
#endif
}

void CompetitionRoute_StartDefault(void)
{
    g_route_steps = g_default_route;
    g_route_step_count = (uint16_t)(sizeof(g_default_route) / sizeof(g_default_route[0]));
    g_step_started = 0U;
    g_rotate_stable_count = 0U;

    g_route_status.state = COMP_ROUTE_STATE_WAIT_READY;
    g_route_status.abort_reason = COMP_ROUTE_ABORT_NONE;
    g_route_status.current_action = COMP_ROUTE_ACTION_STOP;
    g_route_status.step_index = 0U;
    g_route_status.step_count = g_route_step_count;
    g_route_status.step_start_ms = 0U;
    g_route_status.step_elapsed_ms = 0U;
    g_route_status.step_duration_ms = 0U;
    g_route_status.rotate_target_yaw_rad = 0.0f;
    g_route_status.rotate_error_rad = 0.0f;

    Mecanum_EnableStraightControl(0U);
    (void)Mecanum_SetVelocity(0.0f, 0.0f, 0.0f);
}

void CompetitionRoute_Stop(void)
{
    g_route_status.state = COMP_ROUTE_STATE_IDLE;
    g_route_status.abort_reason = COMP_ROUTE_ABORT_NONE;
    g_route_status.current_action = COMP_ROUTE_ACTION_STOP;
    g_step_started = 0U;
    g_rotate_stable_count = 0U;
    route_stop_motion();
}

void CompetitionRoute_Loop(void)
{
    if (g_route_status.state == COMP_ROUTE_STATE_IDLE) {
        return;
    }

    if ((g_route_status.state == COMP_ROUTE_STATE_FINISHED) ||
        (g_route_status.state == COMP_ROUTE_STATE_ABORTED)) {
        route_stop_motion();
        return;
    }

    if (g_route_status.state == COMP_ROUTE_STATE_WAIT_READY) {
        if (route_ready_to_start() == 0U) {
            return;
        }
        g_route_status.state = COMP_ROUTE_STATE_RUNNING;
        g_step_started = 0U;
    }

    if (g_route_status.state == COMP_ROUTE_STATE_RUNNING) {
        if (route_update_safety(1U) == 0U) {
            return;
        }
        route_run_current_step();
    }
}

CompetitionRouteStatus_t CompetitionRoute_GetStatus(void)
{
    return g_route_status;
}

const char *CompetitionRoute_GetStateText(CompetitionRouteState_t state)
{
    switch (state) {
    case COMP_ROUTE_STATE_IDLE:
        return "IDLE";
    case COMP_ROUTE_STATE_WAIT_READY:
        return "WAIT_READY";
    case COMP_ROUTE_STATE_RUNNING:
        return "RUNNING";
    case COMP_ROUTE_STATE_FINISHED:
        return "FINISHED";
    case COMP_ROUTE_STATE_ABORTED:
        return "ABORTED";
    default:
        return "UNKNOWN";
    }
}

const char *CompetitionRoute_GetAbortText(CompetitionRouteAbortReason_t reason)
{
    switch (reason) {
    case COMP_ROUTE_ABORT_NONE:
        return "NONE";
    case COMP_ROUTE_ABORT_LASER_TOO_CLOSE:
        return "LASER_TOO_CLOSE";
    case COMP_ROUTE_ABORT_LASER_INVALID:
        return "LASER_INVALID";
    case COMP_ROUTE_ABORT_LINK_TIMEOUT:
        return "LINK_TIMEOUT";
    case COMP_ROUTE_ABORT_CHASSIS_FAULT:
        return "CHASSIS_FAULT";
    case COMP_ROUTE_ABORT_IMU_INVALID:
        return "IMU_INVALID";
    case COMP_ROUTE_ABORT_CMD_ERROR:
        return "CMD_ERROR";
    case COMP_ROUTE_ABORT_ROTATE_TIMEOUT:
        return "ROTATE_TIMEOUT";
    default:
        return "UNKNOWN";
    }
}
