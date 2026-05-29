#include "jy61p.h"
#include <string.h>

#define JY61P_REG_INDEX(reg)             ((uint8_t)((reg) - JY61P_REG_START))
#define JY61P_IDX_AX                     (0U)
#define JY61P_IDX_AY                     (1U)
#define JY61P_IDX_AZ                     (2U)
#define JY61P_IDX_GX                     (3U)
#define JY61P_IDX_GY                     (4U)
#define JY61P_IDX_GZ                     (5U)
#define JY61P_IDX_ROLL                   (9U)   /* 0x3D */
#define JY61P_IDX_PITCH                  (10U)  /* 0x3E */
#define JY61P_IDX_YAW                    (11U)  /* 0x3F */
#define JY61P_IDX_TEMP                   (12U)  /* 0x40 */

static float jy61p_absf(float x)
{
    return (x < 0.0f) ? -x : x;
}

float JY61P_Wrap180(float angle_deg)
{
    while (angle_deg > 180.0f)
    {
        angle_deg -= 360.0f;
    }
    while (angle_deg < -180.0f)
    {
        angle_deg += 360.0f;
    }
    return angle_deg;
}

static int16_t jy61p_le_i16(const uint8_t *p)
{
    return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8U));
}

static float jy61p_lowpass(float old_value, float new_value, float alpha)
{
    return old_value + alpha * (new_value - old_value);
}

static uint8_t jy61p_raw_is_all_pattern(const uint8_t *rx, uint8_t pattern)
{
    uint8_t i;
    for (i = 0U; i < JY61P_RX_LEN; i++)
    {
        if (rx[i] != pattern)
        {
            return 0U;
        }
    }
    return 1U;
}

static void jy61p_mark_error(JY61P_Device_t *dev, JY61P_Error_t err)
{
    if (dev == 0)
    {
        return;
    }
    dev->last_error = err;
    dev->error_count++;
    dev->consecutive_error_count++;
    if ((err == JY61P_ERROR_TIMEOUT) || (dev->consecutive_error_count >= 3U))
    {
        dev->state = JY61P_STATE_ERROR;
        dev->attitude.valid = 0U;
    }
}

static uint8_t jy61p_raw_physical_valid(const JY61P_RawData_t *raw)
{
    float acc_norm_sq;

    if (raw == 0)
    {
        return 0U;
    }

    if ((jy61p_absf(raw->acc_x_g) > 16.1f) ||
        (jy61p_absf(raw->acc_y_g) > 16.1f) ||
        (jy61p_absf(raw->acc_z_g) > 16.1f))
    {
        return 0U;
    }

    if ((jy61p_absf(raw->gyro_x_dps) > 2000.5f) ||
        (jy61p_absf(raw->gyro_y_dps) > 2000.5f) ||
        (jy61p_absf(raw->gyro_z_dps) > 2000.5f))
    {
        return 0U;
    }

    if ((jy61p_absf(raw->jy61p_roll_deg) > 180.5f) ||
        (jy61p_absf(raw->jy61p_pitch_deg) > 180.5f) ||
        (jy61p_absf(raw->jy61p_yaw_deg) > 180.5f))
    {
        return 0U;
    }

    acc_norm_sq = raw->acc_x_g * raw->acc_x_g + raw->acc_y_g * raw->acc_y_g + raw->acc_z_g * raw->acc_z_g;
    if ((acc_norm_sq < 0.20f) || (acc_norm_sq > 400.0f))
    {
        return 0U;
    }

    return 1U;
}

static uint8_t jy61p_is_still_sample(const JY61P_RawData_t *raw)
{
    float acc_norm_sq;

    if (raw == 0)
    {
        return 0U;
    }

    acc_norm_sq = raw->acc_x_g * raw->acc_x_g + raw->acc_y_g * raw->acc_y_g + raw->acc_z_g * raw->acc_z_g;

    if ((jy61p_absf(raw->gyro_x_dps) < JY61P_GYRO_STILL_THRESHOLD_DPS) &&
        (jy61p_absf(raw->gyro_y_dps) < JY61P_GYRO_STILL_THRESHOLD_DPS) &&
        (jy61p_absf(raw->gyro_z_dps) < JY61P_GYRO_STILL_THRESHOLD_DPS) &&
        (acc_norm_sq >= (JY61P_ACC_NORM_MIN_G * JY61P_ACC_NORM_MIN_G)) &&
        (acc_norm_sq <= (JY61P_ACC_NORM_MAX_G * JY61P_ACC_NORM_MAX_G)))
    {
        return 1U;
    }

    return 0U;
}

static void jy61p_parse_raw_words(JY61P_Device_t *dev, const uint8_t *rx, uint32_t now_ms)
{
    JY61P_RawData_t *raw = &dev->raw;

    raw->acc_raw_x = jy61p_le_i16(&rx[JY61P_IDX_AX * 2U]);
    raw->acc_raw_y = jy61p_le_i16(&rx[JY61P_IDX_AY * 2U]);
    raw->acc_raw_z = jy61p_le_i16(&rx[JY61P_IDX_AZ * 2U]);
    raw->gyro_raw_x = jy61p_le_i16(&rx[JY61P_IDX_GX * 2U]);
    raw->gyro_raw_y = jy61p_le_i16(&rx[JY61P_IDX_GY * 2U]);
    raw->gyro_raw_z = jy61p_le_i16(&rx[JY61P_IDX_GZ * 2U]);
    raw->roll_raw = jy61p_le_i16(&rx[JY61P_IDX_ROLL * 2U]);
    raw->pitch_raw = jy61p_le_i16(&rx[JY61P_IDX_PITCH * 2U]);
    raw->yaw_raw = jy61p_le_i16(&rx[JY61P_IDX_YAW * 2U]);
    raw->temperature_raw = jy61p_le_i16(&rx[JY61P_IDX_TEMP * 2U]);

    raw->acc_x_g = ((float)raw->acc_raw_x) * (16.0f / 32768.0f);
    raw->acc_y_g = ((float)raw->acc_raw_y) * (16.0f / 32768.0f);
    raw->acc_z_g = ((float)raw->acc_raw_z) * (16.0f / 32768.0f);
    raw->gyro_x_dps = ((float)raw->gyro_raw_x) * (2000.0f / 32768.0f);
    raw->gyro_y_dps = ((float)raw->gyro_raw_y) * (2000.0f / 32768.0f);
    raw->gyro_z_dps = ((float)raw->gyro_raw_z) * (2000.0f / 32768.0f);
    raw->jy61p_roll_deg = ((float)raw->roll_raw) * (180.0f / 32768.0f);
    raw->jy61p_pitch_deg = ((float)raw->pitch_raw) * (180.0f / 32768.0f);
    raw->jy61p_yaw_deg = ((float)raw->yaw_raw) * (180.0f / 32768.0f);
    raw->temperature_c = ((float)raw->temperature_raw) / 100.0f;
    raw->sample_time_ms = now_ms;
}

static void jy61p_update_attitude(JY61P_Device_t *dev, uint32_t now_ms)
{
    JY61P_RawData_t *raw = &dev->raw;
    JY61P_Attitude_t *att = &dev->attitude;
    float car_pitch_raw_deg;
    float car_roll_raw_deg;
    float car_yaw_raw_deg;
    float gyro_pitch_dps;
    float gyro_roll_dps;
    float gyro_yaw_dps;
    float dt_s;
    float yaw_pred;
    float yaw_err;

    car_pitch_raw_deg = JY61P_SIGN_CAR_PITCH * raw->jy61p_roll_deg;
    car_roll_raw_deg = JY61P_SIGN_CAR_ROLL * raw->jy61p_pitch_deg;
    car_yaw_raw_deg = JY61P_Wrap180((JY61P_SIGN_CAR_YAW * raw->jy61p_yaw_deg) - dev->calib.yaw_zero_deg);

    gyro_pitch_dps = JY61P_SIGN_CAR_PITCH * (raw->gyro_x_dps - dev->calib.gyro_bias_x_dps);
    gyro_roll_dps = JY61P_SIGN_CAR_ROLL * (raw->gyro_y_dps - dev->calib.gyro_bias_y_dps);
    gyro_yaw_dps = JY61P_SIGN_CAR_YAW * (raw->gyro_z_dps - dev->calib.gyro_bias_z_dps);

    if (dev->filter_initialized == 0U)
    {
        att->car_pitch_deg = car_pitch_raw_deg;
        att->car_roll_deg = car_roll_raw_deg;
        att->car_yaw_deg = car_yaw_raw_deg;
        att->car_yaw_est_deg = car_yaw_raw_deg;
        att->gyro_pitch_dps = gyro_pitch_dps;
        att->gyro_roll_dps = gyro_roll_dps;
        att->gyro_yaw_dps = gyro_yaw_dps;
        dev->filter_initialized = 1U;
    }
    else
    {
        dt_s = ((float)(now_ms - dev->last_estimate_ms)) * 0.001f;
        if ((dt_s <= 0.0f) || (dt_s > 0.20f))
        {
            dt_s = ((float)JY61P_UPDATE_PERIOD_MS) * 0.001f;
        }

        att->car_pitch_deg = jy61p_lowpass(att->car_pitch_deg, car_pitch_raw_deg, JY61P_FILTER_ALPHA_ATT);
        att->car_roll_deg = jy61p_lowpass(att->car_roll_deg, car_roll_raw_deg, JY61P_FILTER_ALPHA_ATT);
        att->gyro_pitch_dps = jy61p_lowpass(att->gyro_pitch_dps, gyro_pitch_dps, JY61P_FILTER_ALPHA_GYRO);
        att->gyro_roll_dps = jy61p_lowpass(att->gyro_roll_dps, gyro_roll_dps, JY61P_FILTER_ALPHA_GYRO);
        att->gyro_yaw_dps = jy61p_lowpass(att->gyro_yaw_dps, gyro_yaw_dps, JY61P_FILTER_ALPHA_GYRO);

        yaw_pred = JY61P_Wrap180(att->car_yaw_est_deg + gyro_yaw_dps * dt_s);
        yaw_err = JY61P_Wrap180(car_yaw_raw_deg - yaw_pred);
        att->car_yaw_est_deg = JY61P_Wrap180(yaw_pred + JY61P_YAW_FILTER_ANGLE_WEIGHT * yaw_err);
        att->car_yaw_deg = car_yaw_raw_deg;
    }

    att->jy61p_roll_deg = raw->jy61p_roll_deg;
    att->jy61p_pitch_deg = raw->jy61p_pitch_deg;
    att->jy61p_yaw_deg = raw->jy61p_yaw_deg;
    att->acc_forward_g = raw->acc_y_g;
    att->acc_right_g = raw->acc_x_g;
    att->acc_down_g = JY61P_SIGN_ACC_DOWN * raw->acc_z_g;
    att->temperature_c = raw->temperature_c;
    att->valid = 1U;
    att->calibrated = dev->calib.calibrated;
    att->last_valid_ms = now_ms;

    dev->last_estimate_ms = now_ms;
    dev->state = JY61P_STATE_RUNNING;
    dev->last_error = JY61P_ERROR_NONE;
    dev->consecutive_error_count = 0U;
}

JY61P_Result_t JY61P_Init(JY61P_Device_t *dev, I2C_HandleTypeDef *i2c, uint32_t now_ms)
{
    HAL_StatusTypeDef hal_ret;

    if ((dev == 0) || (i2c == 0))
    {
        return JY61P_RESULT_PARAM_ERROR;
    }

    memset(dev, 0, sizeof(*dev));
    dev->i2c = i2c;
    dev->addr_7bit = JY61P_I2C_ADDR_7BIT;
    dev->addr_hal = JY61P_I2C_ADDR_HAL;
    dev->state = JY61P_STATE_UNINIT;
    dev->last_poll_ms = now_ms;
    dev->last_estimate_ms = now_ms;

    hal_ret = HAL_I2C_IsDeviceReady(dev->i2c, dev->addr_hal, 2U, JY61P_I2C_TIMEOUT_MS);
    if (hal_ret != HAL_OK)
    {
        dev->state = JY61P_STATE_ERROR;
        dev->last_error = JY61P_ERROR_I2C;
        dev->error_count = 1U;
        return JY61P_RESULT_I2C_ERROR;
    }

    JY61P_StartCalibration(dev, now_ms);
    return JY61P_RESULT_OK;
}

JY61P_Result_t JY61P_ReadRaw(JY61P_Device_t *dev, uint32_t now_ms)
{
    uint8_t rx[JY61P_RX_LEN];
    HAL_StatusTypeDef hal_ret;

    if ((dev == 0) || (dev->i2c == 0))
    {
        return JY61P_RESULT_PARAM_ERROR;
    }

    hal_ret = HAL_I2C_Mem_Read(dev->i2c,
                               dev->addr_hal,
                               JY61P_REG_START,
                               I2C_MEMADD_SIZE_8BIT,
                               rx,
                               JY61P_RX_LEN,
                               JY61P_I2C_TIMEOUT_MS);
    if (hal_ret != HAL_OK)
    {
        jy61p_mark_error(dev, JY61P_ERROR_I2C);
        return JY61P_RESULT_I2C_ERROR;
    }

    if ((jy61p_raw_is_all_pattern(rx, 0x00U) != 0U) || (jy61p_raw_is_all_pattern(rx, 0xFFU) != 0U))
    {
        jy61p_mark_error(dev, JY61P_ERROR_DATA_INVALID);
        return JY61P_RESULT_INVALID_DATA;
    }

    jy61p_parse_raw_words(dev, rx, now_ms);
    if (jy61p_raw_physical_valid(&dev->raw) == 0U)
    {
        jy61p_mark_error(dev, JY61P_ERROR_DATA_INVALID);
        return JY61P_RESULT_INVALID_DATA;
    }

    return JY61P_RESULT_OK;
}

void JY61P_StartCalibration(JY61P_Device_t *dev, uint32_t now_ms)
{
    if (dev == 0)
    {
        return;
    }

    dev->state = JY61P_STATE_CALIBRATING;
    dev->calib.calibrated = 0U;
    dev->calib.sample_count = 0U;
    dev->calib_sum_gx = 0.0f;
    dev->calib_sum_gy = 0.0f;
    dev->calib_sum_gz = 0.0f;
    dev->calib_sum_yaw = 0.0f;
    dev->last_calib_step_ms = now_ms;
    dev->filter_initialized = 0U;
    dev->attitude.valid = 0U;
    dev->attitude.calibrated = 0U;
}

JY61P_Result_t JY61P_CalibrationStep(JY61P_Device_t *dev, uint32_t now_ms)
{
    JY61P_Result_t ret;

    if ((dev == 0) || (dev->i2c == 0))
    {
        return JY61P_RESULT_PARAM_ERROR;
    }

    if ((uint32_t)(now_ms - dev->last_calib_step_ms) < JY61P_CALIB_SAMPLE_PERIOD_MS)
    {
        return JY61P_RESULT_BUSY;
    }
    dev->last_calib_step_ms = now_ms;

    ret = JY61P_ReadRaw(dev, now_ms);
    if (ret != JY61P_RESULT_OK)
    {
        dev->state = JY61P_STATE_CALIBRATING;
        return ret;
    }

    if (jy61p_is_still_sample(&dev->raw) == 0U)
    {
        dev->calib.sample_count = 0U;
        dev->calib_sum_gx = 0.0f;
        dev->calib_sum_gy = 0.0f;
        dev->calib_sum_gz = 0.0f;
        dev->calib_sum_yaw = 0.0f;
        dev->last_error = JY61P_ERROR_NOT_STILL;
        return JY61P_RESULT_BUSY;
    }

    dev->calib_sum_gx += dev->raw.gyro_x_dps;
    dev->calib_sum_gy += dev->raw.gyro_y_dps;
    dev->calib_sum_gz += dev->raw.gyro_z_dps;
    dev->calib_sum_yaw += (JY61P_SIGN_CAR_YAW * dev->raw.jy61p_yaw_deg);
    dev->calib.sample_count++;

    if (dev->calib.sample_count >= JY61P_CALIB_SAMPLE_COUNT)
    {
        float inv_count = 1.0f / (float)dev->calib.sample_count;
        dev->calib.gyro_bias_x_dps = dev->calib_sum_gx * inv_count;
        dev->calib.gyro_bias_y_dps = dev->calib_sum_gy * inv_count;
        dev->calib.gyro_bias_z_dps = dev->calib_sum_gz * inv_count;
        dev->calib.yaw_zero_deg = JY61P_Wrap180(dev->calib_sum_yaw * inv_count);
        dev->calib.calibrated = 1U;
        dev->calib.last_calib_ms = now_ms;
        dev->attitude.calibrated = 1U;
        dev->filter_initialized = 0U;
        dev->consecutive_error_count = 0U;
        dev->last_error = JY61P_ERROR_NONE;
        jy61p_update_attitude(dev, now_ms);
        return JY61P_RESULT_OK;
    }

    return JY61P_RESULT_BUSY;
}

JY61P_Result_t JY61P_Update(JY61P_Device_t *dev, uint32_t now_ms)
{
    JY61P_Result_t ret;

    if ((dev == 0) || (dev->i2c == 0))
    {
        return JY61P_RESULT_PARAM_ERROR;
    }

    if ((dev->attitude.last_valid_ms != 0U) &&
        ((uint32_t)(now_ms - dev->attitude.last_valid_ms) > JY61P_DATA_TIMEOUT_MS))
    {
        dev->attitude.valid = 0U;
        dev->last_error = JY61P_ERROR_TIMEOUT;
        dev->state = JY61P_STATE_ERROR;
    }

    if ((uint32_t)(now_ms - dev->last_poll_ms) < JY61P_UPDATE_PERIOD_MS)
    {
        return JY61P_RESULT_NO_UPDATE;
    }
    dev->last_poll_ms = now_ms;

    if (dev->state == JY61P_STATE_CALIBRATING)
    {
        return JY61P_CalibrationStep(dev, now_ms);
    }

    ret = JY61P_ReadRaw(dev, now_ms);
    if (ret != JY61P_RESULT_OK)
    {
        return ret;
    }

    jy61p_update_attitude(dev, now_ms);
    return JY61P_RESULT_OK;
}

JY61P_Result_t JY61P_SetYawZero(JY61P_Device_t *dev)
{
    if (dev == 0)
    {
        return JY61P_RESULT_PARAM_ERROR;
    }

    if (dev->attitude.valid == 0U)
    {
        return JY61P_RESULT_INVALID_DATA;
    }

    dev->calib.yaw_zero_deg = JY61P_Wrap180(JY61P_SIGN_CAR_YAW * dev->raw.jy61p_yaw_deg);
    dev->attitude.car_yaw_deg = 0.0f;
    dev->attitude.car_yaw_est_deg = 0.0f;
    dev->last_estimate_ms = dev->attitude.last_valid_ms;
    return JY61P_RESULT_OK;
}

JY61P_Result_t JY61P_Recover(JY61P_Device_t *dev, uint32_t now_ms)
{
    HAL_StatusTypeDef hal_ret;

    if ((dev == 0) || (dev->i2c == 0))
    {
        return JY61P_RESULT_PARAM_ERROR;
    }

    hal_ret = HAL_I2C_IsDeviceReady(dev->i2c, dev->addr_hal, 2U, JY61P_I2C_TIMEOUT_MS);
    if (hal_ret != HAL_OK)
    {
        jy61p_mark_error(dev, JY61P_ERROR_I2C);
        return JY61P_RESULT_I2C_ERROR;
    }

    dev->state = (dev->calib.calibrated != 0U) ? JY61P_STATE_RUNNING : JY61P_STATE_CALIBRATING;
    dev->consecutive_error_count = 0U;
    dev->last_poll_ms = now_ms;
    return JY61P_RESULT_OK;
}

JY61P_Result_t JY61P_GetAttitude(const JY61P_Device_t *dev, JY61P_Attitude_t *out)
{
    if ((dev == 0) || (out == 0))
    {
        return JY61P_RESULT_PARAM_ERROR;
    }

    *out = dev->attitude;
    return (dev->attitude.valid != 0U) ? JY61P_RESULT_OK : JY61P_RESULT_TIMEOUT;
}

JY61P_Result_t JY61P_GetRaw(const JY61P_Device_t *dev, JY61P_RawData_t *out)
{
    if ((dev == 0) || (out == 0))
    {
        return JY61P_RESULT_PARAM_ERROR;
    }

    *out = dev->raw;
    return JY61P_RESULT_OK;
}

JY61P_State_t JY61P_GetState(const JY61P_Device_t *dev)
{
    if (dev == 0)
    {
        return JY61P_STATE_UNINIT;
    }
    return dev->state;
}

uint8_t JY61P_IsFresh(const JY61P_Device_t *dev, uint32_t now_ms)
{
    if ((dev == 0) || (dev->attitude.valid == 0U))
    {
        return 0U;
    }

    return ((uint32_t)(now_ms - dev->attitude.last_valid_ms) <= JY61P_DATA_TIMEOUT_MS) ? 1U : 0U;
}
