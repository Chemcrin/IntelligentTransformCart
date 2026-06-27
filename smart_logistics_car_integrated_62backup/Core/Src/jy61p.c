#include "jy61p.h"

static int16_t jy61p_i16_from_le(const uint8_t *bytes)
{
    return (int16_t)(((uint16_t)bytes[1] << 8) | bytes[0]);
}

static float jy61p_scaled(int16_t raw, float range)
{
    return ((float)raw / JY61P_RAW_RESOLUTION) * range;
}

static float jy61p_absf(float value)
{
    return (value < 0.0f) ? -value : value;
}

static float jy61p_wrap_180(float angle_deg)
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

static float jy61p_lpf(float old_value, float new_value, float alpha)
{
    return (old_value * (1.0f - alpha)) + (new_value * alpha);
}

static uint8_t jy61p_is_still(const JY61P_Data_t *data)
{
    float acc_norm_sq = (data->ax_g * data->ax_g) +
                        (data->ay_g * data->ay_g) +
                        (data->az_g * data->az_g);
    float acc_min_sq = JY61P_ACC_NORM_MIN_G * JY61P_ACC_NORM_MIN_G;
    float acc_max_sq = JY61P_ACC_NORM_MAX_G * JY61P_ACC_NORM_MAX_G;

    if ((acc_norm_sq < acc_min_sq) || (acc_norm_sq > acc_max_sq))
    {
        return 0U;
    }

    if ((jy61p_absf(data->gx_dps) > JY61P_GYRO_STILL_THRESHOLD_DPS) ||
        (jy61p_absf(data->gy_dps) > JY61P_GYRO_STILL_THRESHOLD_DPS) ||
        (jy61p_absf(data->gz_dps) > JY61P_GYRO_STILL_THRESHOLD_DPS))
    {
        return 0U;
    }

    return 1U;
}

static void jy61p_decode_raw(const uint8_t *rx, JY61P_RawData_t *raw)
{
    raw->ax = jy61p_i16_from_le(&rx[0]);
    raw->ay = jy61p_i16_from_le(&rx[2]);
    raw->az = jy61p_i16_from_le(&rx[4]);
    raw->gx = jy61p_i16_from_le(&rx[6]);
    raw->gy = jy61p_i16_from_le(&rx[8]);
    raw->gz = jy61p_i16_from_le(&rx[10]);
    raw->roll = jy61p_i16_from_le(&rx[18]);
    raw->pitch = jy61p_i16_from_le(&rx[20]);
    raw->yaw = jy61p_i16_from_le(&rx[22]);
    raw->temp = jy61p_i16_from_le(&rx[24]);
}

static void jy61p_convert_data(const JY61P_RawData_t *raw, JY61P_Data_t *data)
{
    data->ax_g = jy61p_scaled(raw->ax, JY61P_ACC_SCALE_G);
    data->ay_g = jy61p_scaled(raw->ay, JY61P_ACC_SCALE_G);
    data->az_g = jy61p_scaled(raw->az, JY61P_ACC_SCALE_G);
    data->gx_dps = jy61p_scaled(raw->gx, JY61P_GYRO_SCALE_DPS);
    data->gy_dps = jy61p_scaled(raw->gy, JY61P_GYRO_SCALE_DPS);
    data->gz_dps = jy61p_scaled(raw->gz, JY61P_GYRO_SCALE_DPS);
    data->roll_deg = jy61p_scaled(raw->roll, JY61P_ANGLE_SCALE_DEG);
    data->pitch_deg = jy61p_scaled(raw->pitch, JY61P_ANGLE_SCALE_DEG);
    data->yaw_deg = jy61p_scaled(raw->yaw, JY61P_ANGLE_SCALE_DEG);
    data->temperature_c = (float)raw->temp / 100.0f;
}

static uint8_t jy61p_frame_plausible(const JY61P_Data_t *data)
{
    if ((jy61p_absf(data->ax_g) > 20.0f) ||
        (jy61p_absf(data->ay_g) > 20.0f) ||
        (jy61p_absf(data->az_g) > 20.0f))
    {
        return 0U;
    }

    if ((jy61p_absf(data->gx_dps) > 2200.0f) ||
        (jy61p_absf(data->gy_dps) > 2200.0f) ||
        (jy61p_absf(data->gz_dps) > 2200.0f))
    {
        return 0U;
    }

    if ((jy61p_absf(data->roll_deg) > 181.0f) ||
        (jy61p_absf(data->pitch_deg) > 181.0f) ||
        (jy61p_absf(data->yaw_deg) > 181.0f))
    {
        return 0U;
    }

    return 1U;
}

static void jy61p_update_calibration(JY61P_Device_t *dev, const JY61P_Data_t *data)
{
    JY61P_CalibParam_t *calib = &dev->calib;
    float next_n;

    if (dev->state != JY61P_STATE_CALIBRATING)
    {
        return;
    }

    if (jy61p_is_still(data) == 0U)
    {
        calib->gyro_bias_x_dps = 0.0f;
        calib->gyro_bias_y_dps = 0.0f;
        calib->gyro_bias_z_dps = 0.0f;
        calib->sample_count = 0U;
        return;
    }

    next_n = (float)calib->sample_count + 1.0f;
    calib->gyro_bias_x_dps += (data->gx_dps - calib->gyro_bias_x_dps) / next_n;
    calib->gyro_bias_y_dps += (data->gy_dps - calib->gyro_bias_y_dps) / next_n;
    calib->gyro_bias_z_dps += (data->gz_dps - calib->gyro_bias_z_dps) / next_n;
    calib->sample_count++;

    if (calib->sample_count >= JY61P_CALIB_SAMPLE_COUNT)
    {
        calib->calibrated = 1U;
        dev->state = JY61P_STATE_RUNNING;
    }
}

static void jy61p_update_attitude(JY61P_Device_t *dev, const JY61P_Data_t *data, uint32_t now_ms)
{
    JY61P_Attitude_t mapped;
    float gyro_z;
    float yaw_angle;
    float dt_s = 0.0f;

    if (dev->last_valid_ms != 0U)
    {
        dt_s = (float)(uint32_t)(now_ms - dev->last_valid_ms) / 1000.0f;
        if (dt_s > 0.2f)
        {
            dt_s = 0.0f;
        }
    }

    gyro_z = data->gz_dps - dev->calib.gyro_bias_z_dps;
    yaw_angle = jy61p_wrap_180((data->yaw_deg * JY61P_SIGN_CAR_YAW) - dev->calib.yaw_offset_deg);

    mapped.car_pitch_deg = data->roll_deg * JY61P_SIGN_CAR_PITCH;
    mapped.car_roll_deg = data->pitch_deg * JY61P_SIGN_CAR_ROLL;
    mapped.gyro_yaw_dps = gyro_z * JY61P_SIGN_GYRO_YAW;
    mapped.acc_forward_g = data->ay_g * JY61P_SIGN_ACC_FORWARD;
    mapped.acc_right_g = data->ax_g * JY61P_SIGN_ACC_RIGHT;
    mapped.acc_down_g = data->az_g * JY61P_SIGN_ACC_DOWN;
    mapped.temperature_c = data->temperature_c;

    if (dev->valid == 0U)
    {
        mapped.car_yaw_est_deg = yaw_angle;
        dev->attitude = mapped;
        return;
    }

    if (dt_s > 0.0f)
    {
        float gyro_yaw = jy61p_wrap_180(dev->attitude.car_yaw_est_deg + (mapped.gyro_yaw_dps * dt_s));
        float yaw_error = jy61p_wrap_180(yaw_angle - gyro_yaw);
        mapped.car_yaw_est_deg = jy61p_wrap_180(gyro_yaw + (yaw_error * JY61P_YAW_FILTER_ANGLE_WEIGHT));
    }
    else
    {
        mapped.car_yaw_est_deg = yaw_angle;
    }

    dev->attitude.car_pitch_deg = jy61p_lpf(dev->attitude.car_pitch_deg, mapped.car_pitch_deg, JY61P_FILTER_ALPHA_ATT);
    dev->attitude.car_roll_deg = jy61p_lpf(dev->attitude.car_roll_deg, mapped.car_roll_deg, JY61P_FILTER_ALPHA_ATT);
    dev->attitude.car_yaw_est_deg = mapped.car_yaw_est_deg;
    dev->attitude.gyro_yaw_dps = mapped.gyro_yaw_dps;
    dev->attitude.acc_forward_g = mapped.acc_forward_g;
    dev->attitude.acc_right_g = mapped.acc_right_g;
    dev->attitude.acc_down_g = mapped.acc_down_g;
    dev->attitude.temperature_c = mapped.temperature_c;
}

void JY61P_Init(JY61P_Device_t *dev, I2C_HandleTypeDef *hi2c)
{
    if (dev == NULL)
    {
        return;
    }

    dev->i2c = hi2c;
    dev->addr_7bit = JY61P_I2C_ADDR_7BIT;
    dev->addr_hal = JY61P_I2C_ADDR_HAL;
    dev->state = JY61P_STATE_RESET;
    dev->last_error = JY61P_ERROR_NONE;
    dev->raw = (JY61P_RawData_t){0};
    dev->data = (JY61P_Data_t){0};
    dev->attitude = (JY61P_Attitude_t){0};
    dev->calib = (JY61P_CalibParam_t){0};
    dev->init_ms = HAL_GetTick();
    dev->last_poll_ms = 0U;
    dev->last_valid_ms = 0U;
    dev->valid_count = 0U;
    dev->error_count = 0U;
    dev->consecutive_error_count = 0U;
    dev->valid = 0U;

    if (hi2c == NULL)
    {
        dev->state = JY61P_STATE_ERROR;
        dev->last_error = JY61P_ERROR_NULL;
    }
}

HAL_StatusTypeDef JY61P_ReadAll(JY61P_Device_t *dev, JY61P_Data_t *data)
{
    uint8_t rx[JY61P_READ_BYTES];
    HAL_StatusTypeDef status;
    JY61P_RawData_t raw;
    JY61P_Data_t converted;

    if ((dev == NULL) || (dev->i2c == NULL) || (data == NULL))
    {
        if (dev != NULL)
        {
            dev->last_error = JY61P_ERROR_NULL;
        }
        return HAL_ERROR;
    }

    status = HAL_I2C_Mem_Read(dev->i2c,
                              dev->addr_hal,
                              JY61P_REG_START,
                              I2C_MEMADD_SIZE_8BIT,
                              rx,
                              JY61P_READ_BYTES,
                              JY61P_I2C_TIMEOUT_MS);
    if (status != HAL_OK)
    {
        dev->last_error = JY61P_ERROR_I2C;
        return status;
    }

    jy61p_decode_raw(rx, &raw);
    jy61p_convert_data(&raw, &converted);

    if (jy61p_frame_plausible(&converted) == 0U)
    {
        dev->last_error = JY61P_ERROR_BAD_FRAME;
        return HAL_ERROR;
    }

    dev->raw = raw;
    dev->data = converted;
    *data = converted;
    dev->last_error = JY61P_ERROR_NONE;
    return HAL_OK;
}

HAL_StatusTypeDef JY61P_Poll(JY61P_Device_t *dev, uint32_t now_ms)
{
    JY61P_Data_t data;

    if ((dev == NULL) || (dev->i2c == NULL))
    {
        return HAL_ERROR;
    }

    if ((uint32_t)(now_ms - dev->last_poll_ms) < JY61P_UPDATE_PERIOD_MS)
    {
        return HAL_BUSY;
    }
    dev->last_poll_ms = now_ms;

    if ((uint32_t)(now_ms - dev->init_ms) < JY61P_BOOT_IGNORE_MS)
    {
        dev->last_error = JY61P_ERROR_NOT_READY;
        return HAL_BUSY;
    }

    if ((dev->state == JY61P_STATE_RESET) || (dev->state == JY61P_STATE_NOT_FOUND))
    {
        if (HAL_I2C_IsDeviceReady(dev->i2c, dev->addr_hal, 1U, JY61P_I2C_TIMEOUT_MS) != HAL_OK)
        {
            dev->state = JY61P_STATE_NOT_FOUND;
            dev->last_error = JY61P_ERROR_NOT_READY;
            dev->error_count++;
            dev->consecutive_error_count++;
            return HAL_ERROR;
        }
        dev->state = JY61P_STATE_CALIBRATING;
    }

    if (JY61P_ReadAll(dev, &data) != HAL_OK)
    {
        dev->error_count++;
        dev->consecutive_error_count++;
        if (JY61P_IsTimedOut(dev, now_ms) != 0U)
        {
            dev->valid = 0U;
            dev->last_error = JY61P_ERROR_TIMEOUT;
        }
        return HAL_ERROR;
    }

    jy61p_update_calibration(dev, &data);
    jy61p_update_attitude(dev, &data, now_ms);

    dev->valid = 1U;
    dev->valid_count++;
    dev->last_valid_ms = now_ms;
    dev->consecutive_error_count = 0U;
    return HAL_OK;
}

HAL_StatusTypeDef JY61P_StartGyroCalibration(JY61P_Device_t *dev)
{
    if (dev == NULL)
    {
        return HAL_ERROR;
    }

    dev->calib.gyro_bias_x_dps = 0.0f;
    dev->calib.gyro_bias_y_dps = 0.0f;
    dev->calib.gyro_bias_z_dps = 0.0f;
    dev->calib.sample_count = 0U;
    dev->calib.calibrated = 0U;
    dev->state = JY61P_STATE_CALIBRATING;
    return HAL_OK;
}

HAL_StatusTypeDef JY61P_ZeroYawReference(JY61P_Device_t *dev)
{
    if (dev == NULL)
    {
        return HAL_ERROR;
    }

    dev->calib.yaw_offset_deg = dev->data.yaw_deg * JY61P_SIGN_CAR_YAW;
    dev->attitude.car_yaw_est_deg = 0.0f;
    return HAL_OK;
}

HAL_StatusTypeDef JY61P_WriteRegister(JY61P_Device_t *dev, uint8_t reg, uint16_t value)
{
    uint8_t tx[2];

    if ((dev == NULL) || (dev->i2c == NULL))
    {
        return HAL_ERROR;
    }

    tx[0] = (uint8_t)(value & 0xFFU);
    tx[1] = (uint8_t)((value >> 8) & 0xFFU);

    return HAL_I2C_Mem_Write(dev->i2c,
                             dev->addr_hal,
                             reg,
                             I2C_MEMADD_SIZE_8BIT,
                             tx,
                             sizeof(tx),
                             JY61P_I2C_TIMEOUT_MS);
}

HAL_StatusTypeDef JY61P_HardwareZeroYaw(JY61P_Device_t *dev)
{
    if (JY61P_WriteRegister(dev, JY61P_REG_KEY, JY61P_CMD_UNLOCK) != HAL_OK)
    {
        return HAL_ERROR;
    }
    if (JY61P_WriteRegister(dev, JY61P_REG_CALSW, JY61P_CMD_ZERO_YAW) != HAL_OK)
    {
        return HAL_ERROR;
    }
    HAL_Delay(200U);
    return JY61P_WriteRegister(dev, JY61P_REG_SAVE, JY61P_CMD_SAVE);
}

HAL_StatusTypeDef JY61P_HardwareCalibrateAccelerometer(JY61P_Device_t *dev)
{
    if (JY61P_WriteRegister(dev, JY61P_REG_KEY, JY61P_CMD_UNLOCK) != HAL_OK)
    {
        return HAL_ERROR;
    }
    if (JY61P_WriteRegister(dev, JY61P_REG_CALSW, JY61P_CMD_CAL_ACC) != HAL_OK)
    {
        return HAL_ERROR;
    }
    HAL_Delay(3000U);
    return JY61P_WriteRegister(dev, JY61P_REG_SAVE, JY61P_CMD_SAVE);
}

uint8_t JY61P_IsTimedOut(const JY61P_Device_t *dev, uint32_t now_ms)
{
    if ((dev == NULL) || (dev->last_valid_ms == 0U))
    {
        return 1U;
    }

    return ((uint32_t)(now_ms - dev->last_valid_ms) > JY61P_DATA_TIMEOUT_MS) ? 1U : 0U;
}
