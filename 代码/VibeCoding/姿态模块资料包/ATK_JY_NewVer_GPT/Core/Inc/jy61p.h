#ifndef JY61P_H
#define JY61P_H

#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* JY61P: 7-bit I2C address 0x50, HAL uses left-shifted address 0xA0. */
#define JY61P_I2C_ADDR_7BIT              (0x50U)
#define JY61P_I2C_ADDR_HAL               ((uint16_t)(JY61P_I2C_ADDR_7BIT << 1U))

#define JY61P_REG_START                  (0x34U)
#define JY61P_REG_WORD_COUNT             (13U)
#define JY61P_RX_LEN                     (JY61P_REG_WORD_COUNT * 2U)

#define JY61P_UPDATE_PERIOD_MS           (10U)
#define JY61P_DATA_TIMEOUT_MS            (100U)
#define JY61P_I2C_TIMEOUT_MS             (5U)

#define JY61P_CALIB_SAMPLE_COUNT         (200U)
#define JY61P_CALIB_SAMPLE_PERIOD_MS     (10U)
#define JY61P_GYRO_STILL_THRESHOLD_DPS   (1.5f)
#define JY61P_ACC_NORM_MIN_G             (0.95f)
#define JY61P_ACC_NORM_MAX_G             (1.05f)

#define JY61P_FILTER_ALPHA_ATT           (0.30f)
#define JY61P_FILTER_ALPHA_GYRO          (0.30f)
#define JY61P_YAW_FILTER_ANGLE_WEIGHT    (0.02f)

/* Installation direction: +Y forward, +X right. Adjust these after real car test. */
#ifndef JY61P_SIGN_CAR_PITCH
#define JY61P_SIGN_CAR_PITCH             (1.0f)
#endif
#ifndef JY61P_SIGN_CAR_ROLL
#define JY61P_SIGN_CAR_ROLL              (1.0f)
#endif
#ifndef JY61P_SIGN_CAR_YAW
#define JY61P_SIGN_CAR_YAW               (1.0f)
#endif
#ifndef JY61P_SIGN_ACC_DOWN
#define JY61P_SIGN_ACC_DOWN              (-1.0f)
#endif

typedef enum
{
    JY61P_STATE_UNINIT = 0,
    JY61P_STATE_RUNNING,
    JY61P_STATE_CALIBRATING,
    JY61P_STATE_ERROR
} JY61P_State_t;

typedef enum
{
    JY61P_ERROR_NONE = 0,
    JY61P_ERROR_PARAM,
    JY61P_ERROR_I2C,
    JY61P_ERROR_DATA_INVALID,
    JY61P_ERROR_TIMEOUT,
    JY61P_ERROR_NOT_STILL
} JY61P_Error_t;

typedef enum
{
    JY61P_RESULT_OK = 0,
    JY61P_RESULT_BUSY,
    JY61P_RESULT_NO_UPDATE,
    JY61P_RESULT_I2C_ERROR,
    JY61P_RESULT_INVALID_DATA,
    JY61P_RESULT_TIMEOUT,
    JY61P_RESULT_PARAM_ERROR
} JY61P_Result_t;

typedef struct
{
    int16_t acc_raw_x;
    int16_t acc_raw_y;
    int16_t acc_raw_z;
    int16_t gyro_raw_x;
    int16_t gyro_raw_y;
    int16_t gyro_raw_z;
    int16_t roll_raw;
    int16_t pitch_raw;
    int16_t yaw_raw;
    int16_t temperature_raw;

    float acc_x_g;
    float acc_y_g;
    float acc_z_g;
    float gyro_x_dps;
    float gyro_y_dps;
    float gyro_z_dps;
    float jy61p_roll_deg;
    float jy61p_pitch_deg;
    float jy61p_yaw_deg;
    float temperature_c;

    uint32_t sample_time_ms;
} JY61P_RawData_t;

typedef struct
{
    float jy61p_roll_deg;
    float jy61p_pitch_deg;
    float jy61p_yaw_deg;

    float car_roll_deg;
    float car_pitch_deg;
    float car_yaw_deg;
    float car_yaw_est_deg;

    float gyro_pitch_dps;
    float gyro_roll_dps;
    float gyro_yaw_dps;

    float acc_forward_g;
    float acc_right_g;
    float acc_down_g;
    float temperature_c;

    uint8_t valid;
    uint8_t calibrated;
    uint32_t last_valid_ms;
} JY61P_Attitude_t;

typedef struct
{
    float gyro_bias_x_dps;
    float gyro_bias_y_dps;
    float gyro_bias_z_dps;
    float yaw_zero_deg;
    uint8_t calibrated;
    uint16_t sample_count;
    uint32_t last_calib_ms;
} JY61P_CalibParam_t;

typedef struct
{
    I2C_HandleTypeDef *i2c;
    uint8_t addr_7bit;
    uint16_t addr_hal;

    JY61P_State_t state;
    JY61P_Error_t last_error;
    uint16_t error_count;
    uint16_t consecutive_error_count;

    JY61P_RawData_t raw;
    JY61P_Attitude_t attitude;
    JY61P_CalibParam_t calib;

    uint32_t last_poll_ms;
    uint32_t last_estimate_ms;
    uint32_t last_calib_step_ms;

    float calib_sum_gx;
    float calib_sum_gy;
    float calib_sum_gz;
    float calib_sum_yaw;

    uint8_t filter_initialized;
} JY61P_Device_t;

JY61P_Result_t JY61P_Init(JY61P_Device_t *dev, I2C_HandleTypeDef *i2c, uint32_t now_ms);
JY61P_Result_t JY61P_Update(JY61P_Device_t *dev, uint32_t now_ms);
JY61P_Result_t JY61P_ReadRaw(JY61P_Device_t *dev, uint32_t now_ms);
JY61P_Result_t JY61P_GetAttitude(const JY61P_Device_t *dev, JY61P_Attitude_t *out);
JY61P_Result_t JY61P_GetRaw(const JY61P_Device_t *dev, JY61P_RawData_t *out);
void JY61P_StartCalibration(JY61P_Device_t *dev, uint32_t now_ms);
JY61P_Result_t JY61P_CalibrationStep(JY61P_Device_t *dev, uint32_t now_ms);
JY61P_Result_t JY61P_SetYawZero(JY61P_Device_t *dev);
JY61P_Result_t JY61P_Recover(JY61P_Device_t *dev, uint32_t now_ms);
JY61P_State_t JY61P_GetState(const JY61P_Device_t *dev);
uint8_t JY61P_IsFresh(const JY61P_Device_t *dev, uint32_t now_ms);
float JY61P_Wrap180(float angle_deg);

#ifdef __cplusplus
}
#endif

#endif /* JY61P_H */
