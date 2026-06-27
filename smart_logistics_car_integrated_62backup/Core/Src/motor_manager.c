#include "motor_manager.h"
#include "zdt_x42_modbus.h"
#include "mecanum_config.h"

static const uint8_t g_motor_addr[MOTOR_COUNT] = {
    MOTOR_ADDR_FL,
    MOTOR_ADDR_FR,
    MOTOR_ADDR_RL,
    MOTOR_ADDR_RR
};

/* 顺序固定为 FL, FR, RL, RR。默认值仅用于编译和初测，实车必须逐轮标定。 */
static int8_t g_motor_dir_sign[MOTOR_COUNT] = { 1, 1, 1, 1 };
static ZDTX42_MotorState_t g_motor_states[MOTOR_COUNT];

void MotorManager_Init(void)
{
    uint8_t i;

    ZDTX42_Init();
    for (i = 0u; i < MOTOR_COUNT; i++) {
        g_motor_states[i].addr = g_motor_addr[i];
        g_motor_states[i].online = 0u;
        g_motor_states[i].fault = 0u;
        g_motor_states[i].rpm_fb = 0.0f;
        g_motor_states[i].timeout_cnt = 0u;
        g_motor_states[i].crc_error_cnt = 0u;
    }
}

MecanumResult_t MotorManager_EnableAll(uint8_t enable)
{
    uint8_t i;
    MecanumResult_t ret;

    for (i = 0u; i < MOTOR_COUNT; i++) {
        ret = ZDTX42_EnableMotor(g_motor_addr[i], enable);
        if (ret != MECANUM_OK) {
            g_motor_states[i].online = 0u;
            g_motor_states[i].timeout_cnt++;
            return ret;
        }
        g_motor_states[i].online = 1u;
    }

    return MECANUM_OK;
}

MecanumResult_t MotorManager_SetWheelRpm(const WheelRpm_t *rpm)
{
    uint8_t i;
    MecanumResult_t ret;
    float motor_rpm;

    if (rpm == 0) {
        return MECANUM_BAD_PARAM;
    }

    for (i = 0u; i < MOTOR_COUNT; i++) {
        motor_rpm = rpm->wheel_rpm[i] * (float)g_motor_dir_sign[i];
        ret = ZDTX42_SetMotorSpeedRpm(g_motor_addr[i], motor_rpm, 1u);
        if (ret != MECANUM_OK) {
            g_motor_states[i].online = 0u;
            g_motor_states[i].timeout_cnt++;
            return ret;
        }
        g_motor_states[i].online = 1u;
    }

    return ZDTX42_SyncTrigger();
}

MecanumResult_t MotorManager_StopAll(void)
{
    WheelRpm_t zero = { { 0.0f, 0.0f, 0.0f, 0.0f } };

    return MotorManager_SetWheelRpm(&zero);
}

MecanumResult_t MotorManager_EmergencyStop(void)
{
    return ZDTX42_EmergencyStopAll();
}

MecanumResult_t MotorManager_UpdateFeedback(void)
{
    static uint8_t s_next_index = 0u;
    float rpm;
    MecanumResult_t ret;
    uint8_t i = s_next_index;

    ret = ZDTX42_ReadMotorSpeed(g_motor_addr[i], &rpm);
    if (ret == MECANUM_OK) {
        g_motor_states[i].rpm_fb = rpm * (float)g_motor_dir_sign[i];
        g_motor_states[i].online = 1u;
        g_motor_states[i].timeout_cnt = 0u;
    } else {
        g_motor_states[i].online = 0u;
        if (ret == MECANUM_CRC_ERROR) {
            g_motor_states[i].crc_error_cnt++;
        } else {
            g_motor_states[i].timeout_cnt++;
        }
    }

    s_next_index++;
    if (s_next_index >= MOTOR_COUNT) {
        s_next_index = 0u;
    }

    return ret;
}

const ZDTX42_MotorState_t *MotorManager_GetStates(void)
{
    return g_motor_states;
}

void MotorManager_SetDirectionSigns(const int8_t dir_sign[4])
{
    uint8_t i;

    if (dir_sign == 0) {
        return;
    }

    for (i = 0u; i < MOTOR_COUNT; i++) {
        g_motor_dir_sign[i] = (dir_sign[i] < 0) ? -1 : 1;
    }
}

const int8_t *MotorManager_GetDirectionSigns(void)
{
    return g_motor_dir_sign;
}

uint8_t MotorManager_HasFault(void)
{
    uint8_t i;

    for (i = 0u; i < MOTOR_COUNT; i++) {
        if ((g_motor_states[i].fault != 0u) ||
            (g_motor_states[i].timeout_cnt >= MOTOR_TIMEOUT_FAULT_LIMIT)) {
            return 1u;
        }
    }

    return 0u;
}

MecanumResult_t Mecanum_TestMotorDirection(float test_rpm)
{
    uint8_t i;
    MecanumResult_t ret;

    for (i = 0u; i < MOTOR_COUNT; i++) {
        ret = ZDTX42_SetMotorSpeedRpm(g_motor_addr[i], test_rpm, 0u);
        if (ret != MECANUM_OK) {
            return ret;
        }
        HAL_Delay(800u);
        ret = ZDTX42_SetMotorSpeedRpm(g_motor_addr[i], 0.0f, 0u);
        if (ret != MECANUM_OK) {
            return ret;
        }
        HAL_Delay(400u);
    }

    return MECANUM_OK;
}
