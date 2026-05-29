#include "atk_ms53l1m.h"
#include <string.h>

#ifndef __weak
#define __weak __attribute__((weak))
#endif

/* VL53L1X register addresses used by the lightweight fallback platform layer. */
#define VL53L1X_REG_GPIO_TIO_HV_STATUS       (0x0031U)
#define VL53L1X_REG_SYSTEM_INTERRUPT_CLEAR   (0x0086U)
#define VL53L1X_REG_SYSTEM_MODE_START        (0x0087U)
#define VL53L1X_REG_RESULT_RANGE_STATUS      (0x0089U)
#define VL53L1X_REG_RESULT_DISTANCE_MM       (0x0096U)
#define VL53L1X_REG_FIRMWARE_SYSTEM_STATUS   (0x00E5U)
#define VL53L1X_REG_IDENTIFICATION_MODEL_ID  (0x010FU)

static void atk_mark_error(ATK_MS53L1M_Device_t *dev, ATK_MS53L1M_Error_t err)
{
    if (dev == 0)
    {
        return;
    }
    dev->last_error = err;
    dev->error_count++;
    dev->consecutive_error_count++;
    if ((err == ATK_MS53L1M_ERROR_TIMEOUT) || (dev->consecutive_error_count >= 3U))
    {
        dev->state = ATK_MS53L1M_STATE_ERROR;
        dev->distance.valid = 0U;
    }
}

static uint16_t atk_abs_diff_u16(uint16_t a, uint16_t b)
{
    return (a >= b) ? (uint16_t)(a - b) : (uint16_t)(b - a);
}

static uint16_t atk_median_u16(const uint16_t *data, uint8_t count)
{
    uint16_t tmp[ATK_MS53L1M_MEDIAN_WINDOW];
    uint8_t i;
    uint8_t j;

    for (i = 0U; i < count; i++)
    {
        tmp[i] = data[i];
    }

    for (i = 0U; i < count; i++)
    {
        for (j = (uint8_t)(i + 1U); j < count; j++)
        {
            if (tmp[j] < tmp[i])
            {
                uint16_t swap = tmp[i];
                tmp[i] = tmp[j];
                tmp[j] = swap;
            }
        }
    }

    return tmp[count / 2U];
}

static uint16_t atk_avg_u16(const uint16_t *data, uint8_t count)
{
    uint8_t i;
    uint32_t sum = 0U;

    for (i = 0U; i < count; i++)
    {
        sum += data[i];
    }

    if (count == 0U)
    {
        return 0U;
    }
    return (uint16_t)(sum / count);
}

static uint8_t atk_range_is_valid(uint16_t mm)
{
    return ((mm >= ATK_MS53L1M_MIN_VALID_MM) && (mm <= ATK_MS53L1M_MAX_VALID_MM)) ? 1U : 0U;
}

static void atk_accept_distance(ATK_MS53L1M_Device_t *dev, uint16_t accepted_mm, uint32_t now_ms)
{
    dev->filter.avg_window[dev->filter.avg_index] = accepted_mm;
    dev->filter.avg_index = (uint8_t)((dev->filter.avg_index + 1U) % ATK_MS53L1M_AVG_WINDOW);
    if (dev->filter.avg_count < ATK_MS53L1M_AVG_WINDOW)
    {
        dev->filter.avg_count++;
    }

    dev->filter.last_accepted_mm = accepted_mm;
    dev->distance.filtered_distance_mm = atk_avg_u16(dev->filter.avg_window, dev->filter.avg_count);
    dev->distance.last_valid_distance_mm = dev->distance.filtered_distance_mm;
    dev->distance.valid = 1U;
    dev->distance.is_warning = (dev->distance.filtered_distance_mm <= ATK_MS53L1M_WARN_DISTANCE_MM) ? 1U : 0U;
    dev->distance.is_slow = (dev->distance.filtered_distance_mm <= ATK_MS53L1M_SLOW_DISTANCE_MM) ? 1U : 0U;
    dev->distance.is_too_close = (dev->filter.close_confirm_count >= ATK_MS53L1M_STOP_CONFIRM_COUNT) ? 1U : 0U;
    dev->distance.last_valid_ms = now_ms;
    dev->distance.sample_count++;
    dev->last_error = ATK_MS53L1M_ERROR_NONE;
    dev->consecutive_error_count = 0U;
    dev->state = ATK_MS53L1M_STATE_RUNNING;
}

static HAL_StatusTypeDef atk_write_u8(ATK_MS53L1M_Device_t *dev, uint16_t reg, uint8_t value)
{
    return HAL_I2C_Mem_Write(dev->i2c,
                             dev->addr_hal,
                             reg,
                             I2C_MEMADD_SIZE_16BIT,
                             &value,
                             1U,
                             ATK_MS53L1M_I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef atk_read_u8(ATK_MS53L1M_Device_t *dev, uint16_t reg, uint8_t *value)
{
    return HAL_I2C_Mem_Read(dev->i2c,
                            dev->addr_hal,
                            reg,
                            I2C_MEMADD_SIZE_16BIT,
                            value,
                            1U,
                            ATK_MS53L1M_I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef atk_read_u16_be(ATK_MS53L1M_Device_t *dev, uint16_t reg, uint16_t *value)
{
    uint8_t rx[2];
    HAL_StatusTypeDef ret;

    ret = HAL_I2C_Mem_Read(dev->i2c,
                           dev->addr_hal,
                           reg,
                           I2C_MEMADD_SIZE_16BIT,
                           rx,
                           2U,
                           ATK_MS53L1M_I2C_TIMEOUT_MS);
    if (ret == HAL_OK)
    {
        *value = (uint16_t)(((uint16_t)rx[0] << 8U) | rx[1]);
    }
    return ret;
}

ATK_MS53L1M_Result_t ATK_MS53L1M_Init(ATK_MS53L1M_Device_t *dev, I2C_HandleTypeDef *i2c, uint32_t now_ms)
{
    HAL_StatusTypeDef hal_ret;

    if ((dev == 0) || (i2c == 0))
    {
        return ATK_MS53L1M_RESULT_PARAM_ERROR;
    }

    memset(dev, 0, sizeof(*dev));
    dev->i2c = i2c;
    dev->addr_7bit = ATK_MS53L1M_I2C_ADDR_7BIT;
    dev->addr_hal = ATK_MS53L1M_I2C_ADDR_HAL;
    dev->state = ATK_MS53L1M_STATE_UNINIT;
    dev->last_poll_ms = now_ms;
    dev->last_recover_ms = now_ms;

    hal_ret = HAL_I2C_IsDeviceReady(dev->i2c, dev->addr_hal, 2U, ATK_MS53L1M_I2C_TIMEOUT_MS);
    if (hal_ret != HAL_OK)
    {
        dev->state = ATK_MS53L1M_STATE_ERROR;
        dev->last_error = ATK_MS53L1M_ERROR_I2C;
        dev->error_count = 1U;
        return ATK_MS53L1M_RESULT_I2C_ERROR;
    }

    return ATK_MS53L1M_StartDefault(dev, now_ms);
}

ATK_MS53L1M_Result_t ATK_MS53L1M_StartDefault(ATK_MS53L1M_Device_t *dev, uint32_t now_ms)
{
    HAL_StatusTypeDef hal_ret;

    if ((dev == 0) || (dev->i2c == 0))
    {
        return ATK_MS53L1M_RESULT_PARAM_ERROR;
    }

    hal_ret = ATK_MS53L1M_PlatformStartRanging(dev);
    if (hal_ret != HAL_OK)
    {
        atk_mark_error(dev, ATK_MS53L1M_ERROR_I2C);
        return ATK_MS53L1M_RESULT_I2C_ERROR;
    }

    memset(&dev->filter, 0, sizeof(dev->filter));
    memset(&dev->distance, 0, sizeof(dev->distance));
    dev->state = ATK_MS53L1M_STATE_RUNNING;
    dev->last_error = ATK_MS53L1M_ERROR_NONE;
    dev->consecutive_error_count = 0U;
    dev->last_poll_ms = now_ms;
    return ATK_MS53L1M_RESULT_OK;
}

ATK_MS53L1M_Result_t ATK_MS53L1M_ReadDistanceRaw(ATK_MS53L1M_Device_t *dev, uint16_t *distance_mm)
{
    HAL_StatusTypeDef hal_ret;

    if ((dev == 0) || (dev->i2c == 0) || (distance_mm == 0))
    {
        return ATK_MS53L1M_RESULT_PARAM_ERROR;
    }

    hal_ret = ATK_MS53L1M_PlatformReadDistance(dev, distance_mm);
    if (hal_ret != HAL_OK)
    {
        atk_mark_error(dev, ATK_MS53L1M_ERROR_I2C);
        return ATK_MS53L1M_RESULT_I2C_ERROR;
    }

    (void)ATK_MS53L1M_PlatformClearInterrupt(dev);
    return ATK_MS53L1M_RESULT_OK;
}

ATK_MS53L1M_Result_t ATK_MS53L1M_FilterUpdate(ATK_MS53L1M_Device_t *dev, uint16_t raw_mm, uint32_t now_ms)
{
    uint16_t med_mm;
    uint8_t range_valid;

    if (dev == 0)
    {
        return ATK_MS53L1M_RESULT_PARAM_ERROR;
    }

    dev->distance.raw_distance_mm = raw_mm;
    range_valid = atk_range_is_valid(raw_mm);

    if ((raw_mm > 0U) && (raw_mm <= ATK_MS53L1M_STOP_DISTANCE_MM))
    {
        if (dev->filter.close_confirm_count < 255U)
        {
            dev->filter.close_confirm_count++;
        }
    }
    else
    {
        dev->filter.close_confirm_count = 0U;
    }

    /* Near-distance priority: after two close samples, update immediately. */
    if (dev->filter.close_confirm_count >= ATK_MS53L1M_STOP_CONFIRM_COUNT)
    {
        atk_accept_distance(dev, raw_mm, now_ms);
        dev->distance.filtered_distance_mm = raw_mm;
        dev->distance.last_valid_distance_mm = raw_mm;
        dev->distance.is_warning = 1U;
        dev->distance.is_slow = 1U;
        dev->distance.is_too_close = 1U;
        return ATK_MS53L1M_RESULT_OK;
    }

    if (range_valid == 0U)
    {
        dev->last_error = ATK_MS53L1M_ERROR_RANGE_INVALID;
        return ATK_MS53L1M_RESULT_INVALID_DATA;
    }

    dev->filter.median_window[dev->filter.median_index] = raw_mm;
    dev->filter.median_index = (uint8_t)((dev->filter.median_index + 1U) % ATK_MS53L1M_MEDIAN_WINDOW);
    if (dev->filter.median_count < ATK_MS53L1M_MEDIAN_WINDOW)
    {
        dev->filter.median_count++;
    }

    med_mm = atk_median_u16(dev->filter.median_window, dev->filter.median_count);

    if ((dev->filter.last_accepted_mm != 0U) &&
        (atk_abs_diff_u16(med_mm, dev->filter.last_accepted_mm) > ATK_MS53L1M_JUMP_REJECT_MM))
    {
        dev->filter.reject_count++;
        if (dev->filter.reject_count < ATK_MS53L1M_JUMP_CONFIRM_COUNT)
        {
            dev->last_error = ATK_MS53L1M_ERROR_RANGE_INVALID;
            return ATK_MS53L1M_RESULT_INVALID_DATA;
        }
    }
    else
    {
        dev->filter.reject_count = 0U;
    }

    atk_accept_distance(dev, med_mm, now_ms);
    return ATK_MS53L1M_RESULT_OK;
}

ATK_MS53L1M_Result_t ATK_MS53L1M_Update(ATK_MS53L1M_Device_t *dev, uint32_t now_ms)
{
    uint8_t ready = 0U;
    uint16_t raw_mm = 0U;
    HAL_StatusTypeDef hal_ret;
    ATK_MS53L1M_Result_t ret;

    if ((dev == 0) || (dev->i2c == 0))
    {
        return ATK_MS53L1M_RESULT_PARAM_ERROR;
    }

    if ((dev->distance.last_valid_ms != 0U) &&
        ((uint32_t)(now_ms - dev->distance.last_valid_ms) > ATK_MS53L1M_DATA_TIMEOUT_MS))
    {
        dev->distance.valid = 0U;
        dev->last_error = ATK_MS53L1M_ERROR_TIMEOUT;
        dev->state = ATK_MS53L1M_STATE_ERROR;
    }

    if ((uint32_t)(now_ms - dev->last_poll_ms) < ATK_MS53L1M_UPDATE_PERIOD_MS)
    {
        return ATK_MS53L1M_RESULT_NO_UPDATE;
    }
    dev->last_poll_ms = now_ms;

    if (dev->state != ATK_MS53L1M_STATE_RUNNING)
    {
        if ((uint32_t)(now_ms - dev->last_recover_ms) >= ATK_MS53L1M_RECOVER_PERIOD_MS)
        {
            return ATK_MS53L1M_Recover(dev, now_ms);
        }
        return ATK_MS53L1M_RESULT_BUSY;
    }

    hal_ret = ATK_MS53L1M_PlatformIsDataReady(dev, &ready);
    if (hal_ret != HAL_OK)
    {
        atk_mark_error(dev, ATK_MS53L1M_ERROR_I2C);
        return ATK_MS53L1M_RESULT_I2C_ERROR;
    }

    if (ready == 0U)
    {
        dev->last_error = ATK_MS53L1M_ERROR_NOT_READY;
        return ATK_MS53L1M_RESULT_NO_UPDATE;
    }

    ret = ATK_MS53L1M_ReadDistanceRaw(dev, &raw_mm);
    if (ret != ATK_MS53L1M_RESULT_OK)
    {
        return ret;
    }

    return ATK_MS53L1M_FilterUpdate(dev, raw_mm, now_ms);
}

ATK_MS53L1M_Result_t ATK_MS53L1M_GetDistance(const ATK_MS53L1M_Device_t *dev, ATK_MS53L1M_Distance_t *out)
{
    if ((dev == 0) || (out == 0))
    {
        return ATK_MS53L1M_RESULT_PARAM_ERROR;
    }

    *out = dev->distance;
    return (dev->distance.valid != 0U) ? ATK_MS53L1M_RESULT_OK : ATK_MS53L1M_RESULT_TIMEOUT;
}

ATK_MS53L1M_Result_t ATK_MS53L1M_Recover(ATK_MS53L1M_Device_t *dev, uint32_t now_ms)
{
    HAL_StatusTypeDef hal_ret;

    if ((dev == 0) || (dev->i2c == 0))
    {
        return ATK_MS53L1M_RESULT_PARAM_ERROR;
    }

    dev->last_recover_ms = now_ms;
    hal_ret = HAL_I2C_IsDeviceReady(dev->i2c, dev->addr_hal, 2U, ATK_MS53L1M_I2C_TIMEOUT_MS);
    if (hal_ret != HAL_OK)
    {
        atk_mark_error(dev, ATK_MS53L1M_ERROR_I2C);
        return ATK_MS53L1M_RESULT_I2C_ERROR;
    }

    return ATK_MS53L1M_StartDefault(dev, now_ms);
}

ATK_MS53L1M_State_t ATK_MS53L1M_GetState(const ATK_MS53L1M_Device_t *dev)
{
    if (dev == 0)
    {
        return ATK_MS53L1M_STATE_UNINIT;
    }
    return dev->state;
}

uint8_t ATK_MS53L1M_IsFresh(const ATK_MS53L1M_Device_t *dev, uint32_t now_ms)
{
    if ((dev == 0) || (dev->distance.valid == 0U))
    {
        return 0U;
    }

    return ((uint32_t)(now_ms - dev->distance.last_valid_ms) <= ATK_MS53L1M_DATA_TIMEOUT_MS) ? 1U : 0U;
}

__weak HAL_StatusTypeDef ATK_MS53L1M_PlatformStartRanging(ATK_MS53L1M_Device_t *dev)
{
    uint8_t boot_state = 0U;
    uint8_t model_id = 0U;
    uint32_t start_ms;

    if ((dev == 0) || (dev->i2c == 0))
    {
        return HAL_ERROR;
    }

    /* Optional identity read. Some compatible modules do not expose a strict model byte, so
     * the value is not used as a hard failure condition.
     */
    (void)atk_read_u8(dev, VL53L1X_REG_IDENTIFICATION_MODEL_ID, &model_id);

    start_ms = HAL_GetTick();
    do
    {
        if (atk_read_u8(dev, VL53L1X_REG_FIRMWARE_SYSTEM_STATUS, &boot_state) != HAL_OK)
        {
            return HAL_ERROR;
        }
        if ((boot_state & 0x01U) != 0U)
        {
            break;
        }
    } while ((uint32_t)(HAL_GetTick() - start_ms) < 100U);

    if ((boot_state & 0x01U) == 0U)
    {
        return HAL_ERROR;
    }

    if (atk_write_u8(dev, VL53L1X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01U) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* 0x40 starts autonomous ranging on a configured VL53L1X. If your bare chip
     * has not been configured by an ST ULD init sequence, override the weak hooks.
     */
    return atk_write_u8(dev, VL53L1X_REG_SYSTEM_MODE_START, 0x40U);
}

__weak HAL_StatusTypeDef ATK_MS53L1M_PlatformIsDataReady(ATK_MS53L1M_Device_t *dev, uint8_t *ready)
{
    uint8_t gpio_status = 0U;
    HAL_StatusTypeDef ret;

    if ((dev == 0) || (ready == 0))
    {
        return HAL_ERROR;
    }

    ret = atk_read_u8(dev, VL53L1X_REG_GPIO_TIO_HV_STATUS, &gpio_status);
    if (ret == HAL_OK)
    {
        *ready = ((gpio_status & 0x01U) == 0U) ? 1U : 0U;
    }
    return ret;
}

__weak HAL_StatusTypeDef ATK_MS53L1M_PlatformReadDistance(ATK_MS53L1M_Device_t *dev, uint16_t *distance_mm)
{
    uint8_t range_status = 0U;

    if ((dev == 0) || (distance_mm == 0))
    {
        return HAL_ERROR;
    }

    /* Read status for diagnostics, but do not reject here; range filtering is done above. */
    (void)atk_read_u8(dev, VL53L1X_REG_RESULT_RANGE_STATUS, &range_status);
    return atk_read_u16_be(dev, VL53L1X_REG_RESULT_DISTANCE_MM, distance_mm);
}

__weak HAL_StatusTypeDef ATK_MS53L1M_PlatformClearInterrupt(ATK_MS53L1M_Device_t *dev)
{
    if (dev == 0)
    {
        return HAL_ERROR;
    }
    return atk_write_u8(dev, VL53L1X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01U);
}
