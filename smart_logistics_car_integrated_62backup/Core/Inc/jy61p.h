#ifndef JY61P_H
#define JY61P_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JY61P_I2C_ADDR_7BIT              0x50U
#define JY61P_I2C_ADDR_HAL               (JY61P_I2C_ADDR_7BIT << 1)
#define JY61P_I2C_TIMEOUT_MS             5U

#define JY61P_REG_AX                     0x34U
#define JY61P_REG_AY                     0x35U
#define JY61P_REG_AZ                     0x36U
#define JY61P_REG_GX                     0x37U
#define JY61P_REG_GY                     0x38U
#define JY61P_REG_GZ                     0x39U
#define JY61P_REG_ROLL                   0x3DU
#define JY61P_REG_PITCH                  0x3EU
#define JY61P_REG_YAW                    0x3FU
#define JY61P_REG_TEMP                   0x40U
#define JY61P_REG_START                  JY61P_REG_AX
#define JY61P_REG_COUNT                  13U
#define JY61P_READ_BYTES                 (JY61P_REG_COUNT * 2U)

#define JY61P_REG_SAVE                   0x00U
#define JY61P_REG_CALSW                  0x01U
#define JY61P_REG_KEY                    0x69U
#define JY61P_CMD_UNLOCK                 0xB588U
#define JY61P_CMD_SAVE                   0x0000U
#define JY61P_CMD_CAL_ACC                0x0001U
#define JY61P_CMD_ZERO_YAW               0x0004U

#define JY61P_ACC_SCALE_G                16.0f
#define JY61P_GYRO_SCALE_DPS             2000.0f
#define JY61P_ANGLE_SCALE_DEG            180.0f
#define JY61P_RAW_RESOLUTION             32768.0f

#define JY61P_UPDATE_PERIOD_MS           10U
#define JY61P_DATA_TIMEOUT_MS            100U
#define JY61P_BOOT_IGNORE_MS             1000U
#define JY61P_CALIB_SAMPLE_COUNT         200U
#define JY61P_CALIB_SAMPLE_PERIOD_MS     10U
#define JY61P_GYRO_STILL_THRESHOLD_DPS   1.5f
#define JY61P_ACC_NORM_MIN_G             0.85f
#define JY61P_ACC_NORM_MAX_G             1.15f
#define JY61P_FILTER_ALPHA_ATT           0.30f
#define JY61P_YAW_FILTER_GYRO_WEIGHT     0.98f
#define JY61P_YAW_FILTER_ANGLE_WEIGHT    0.02f

#define JY61P_SIGN_ACC_FORWARD           1.0f
#define JY61P_SIGN_ACC_RIGHT             1.0f
#define JY61P_SIGN_ACC_DOWN              -1.0f
#define JY61P_SIGN_GYRO_YAW              1.0f
#define JY61P_SIGN_CAR_PITCH             1.0f
#define JY61P_SIGN_CAR_ROLL              1.0f
#define JY61P_SIGN_CAR_YAW               1.0f

#define JY61P_FLAG_ACCEL                 0x01U
#define JY61P_FLAG_GYRO                  0x02U
#define JY61P_FLAG_ANGLE                 0x04U
#define JY61P_FLAG_TEMP                  0x08U

typedef enum
{
    JY61P_STATE_RESET = 0,
    JY61P_STATE_NOT_FOUND,
    JY61P_STATE_CALIBRATING,
    JY61P_STATE_RUNNING,
    JY61P_STATE_ERROR
} JY61P_State_t;

typedef enum
{
    JY61P_ERROR_NONE = 0,
    JY61P_ERROR_NULL,
    JY61P_ERROR_I2C,
    JY61P_ERROR_NOT_READY,
    JY61P_ERROR_BAD_FRAME,
    JY61P_ERROR_TIMEOUT
} JY61P_Error_t;

typedef struct
{
    int16_t ax;
    int16_t ay;
    int16_t az;
    int16_t gx;
    int16_t gy;
    int16_t gz;
    int16_t roll;
    int16_t pitch;
    int16_t yaw;
    int16_t temp;
} JY61P_RawData_t;

typedef struct
{
    float ax_g;
    float ay_g;
    float az_g;
    float gx_dps;
    float gy_dps;
    float gz_dps;
    float roll_deg;
    float pitch_deg;
    float yaw_deg;
    float temperature_c;
} JY61P_Data_t;

typedef struct
{
    float car_yaw_est_deg;
    float car_pitch_deg;
    float car_roll_deg;
    float gyro_yaw_dps;
    float acc_forward_g;
    float acc_right_g;
    float acc_down_g;
    float temperature_c;
} JY61P_Attitude_t;

typedef struct
{
    float gyro_bias_x_dps;
    float gyro_bias_y_dps;
    float gyro_bias_z_dps;
    float yaw_offset_deg;
    uint16_t sample_count;
    uint8_t calibrated;
} JY61P_CalibParam_t;

typedef struct
{
    I2C_HandleTypeDef *i2c;
    uint8_t addr_7bit;
    uint16_t addr_hal;
    JY61P_State_t state;
    JY61P_Error_t last_error;
    JY61P_RawData_t raw;
    JY61P_Data_t data;
    JY61P_Attitude_t attitude;
    JY61P_CalibParam_t calib;
    uint32_t init_ms;
    uint32_t last_poll_ms;
    uint32_t last_valid_ms;
    uint32_t valid_count;
    uint16_t error_count;
    uint16_t consecutive_error_count;
    uint8_t valid;
} JY61P_Device_t;

void JY61P_Init(JY61P_Device_t *dev, I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef JY61P_Poll(JY61P_Device_t *dev, uint32_t now_ms);
HAL_StatusTypeDef JY61P_ReadAll(JY61P_Device_t *dev, JY61P_Data_t *data);
HAL_StatusTypeDef JY61P_StartGyroCalibration(JY61P_Device_t *dev);
HAL_StatusTypeDef JY61P_ZeroYawReference(JY61P_Device_t *dev);
HAL_StatusTypeDef JY61P_WriteRegister(JY61P_Device_t *dev, uint8_t reg, uint16_t value);
HAL_StatusTypeDef JY61P_HardwareZeroYaw(JY61P_Device_t *dev);
HAL_StatusTypeDef JY61P_HardwareCalibrateAccelerometer(JY61P_Device_t *dev);
uint8_t JY61P_IsTimedOut(const JY61P_Device_t *dev, uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif
