#ifndef MECANUM_TYPES_H
#define MECANUM_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    MECANUM_OK = 0,
    MECANUM_ERROR = -1,
    MECANUM_BUSY = -2,
    MECANUM_TIMEOUT = -3,
    MECANUM_BAD_PARAM = -4,
    MECANUM_CRC_ERROR = -5,
    MECANUM_FAULT_LOCKED = -6
} MecanumResult_t;

typedef enum
{
    MECANUM_STATE_IDLE = 0,
    MECANUM_STATE_READY,
    MECANUM_STATE_RUNNING,
    MECANUM_STATE_STOP,
    MECANUM_STATE_FAULT,
    MECANUM_STATE_EMERGENCY_STOP
} MecanumState_t;

typedef enum
{
    MOTOR_INDEX_FL = 0,
    MOTOR_INDEX_FR = 1,
    MOTOR_INDEX_RL = 2,
    MOTOR_INDEX_RR = 3
} MotorIndex_t;

typedef struct
{
    float vx_mps;      /* X 轴速度，向左为正，单位 m/s */
    float vy_mps;      /* Y 轴速度，向前为正，单位 m/s */
    float wz_radps;    /* Z 轴角速度，符号由 MECANUM_WZ_SIGN 标定，单位 rad/s */
} MecanumVelocity_t;

typedef struct
{
    float wheel_fl_mps;
    float wheel_fr_mps;
    float wheel_rl_mps;
    float wheel_rr_mps;
} MecanumWheelSpeed_t;

typedef struct
{
    float wheel_rpm[4]; /* 顺序固定：FL, FR, RL, RR */
} WheelRpm_t;

typedef struct
{
    uint8_t addr;
    uint8_t online;
    uint8_t fault;
    float rpm_fb;
    uint16_t timeout_cnt;
    uint16_t crc_error_cnt;
} ZDTX42_MotorState_t;

#ifdef __cplusplus
}
#endif

#endif
