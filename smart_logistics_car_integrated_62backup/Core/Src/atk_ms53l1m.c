#include "atk_ms53l1m.h"
#include "vl53l1_api.h"
#include <string.h>

static VL53L1_Dev_t g_vl53l1_dev;

static void atk_ms53l1m_reset_filter(ATK_MS53L1M_Filter_t *filter)
{
    memset(filter, 0, sizeof(*filter));
}

static void atk_ms53l1m_push_u16(uint16_t *window, uint8_t size, uint8_t *index, uint8_t *count, uint16_t value)
{
    window[*index] = value;
    *index = (uint8_t)((*index + 1U) % size);
    if (*count < size)
    {
        (*count)++;
    }
}

static uint16_t atk_ms53l1m_median(const uint16_t *src, uint8_t count)
{
    uint16_t tmp[MS53L1M_MEDIAN_WINDOW];
    uint8_t i;
    uint8_t j;

    if (count == 0U)
    {
        return 0U;
    }

    for (i = 0U; i < count; i++)
    {
        tmp[i] = src[i];
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

static uint16_t atk_ms53l1m_average(const uint16_t *src, uint8_t count)
{
    uint32_t sum = 0U;
    uint8_t i;

    if (count == 0U)
    {
        return 0U;
    }

    for (i = 0U; i < count; i++)
    {
        sum += src[i];
    }

    return (uint16_t)(sum / count);
}

static uint16_t atk_ms53l1m_filter_sample(ATK_MS53L1M_Device_t *dev, uint16_t sample_mm)
{
    ATK_MS53L1M_Filter_t *filter = &dev->filter;
    uint16_t median_mm;
    uint16_t filtered_mm;
    uint16_t last = filter->last_accepted_mm;

    if ((sample_mm < MS53L1M_MIN_VALID_MM) || (sample_mm > MS53L1M_MAX_VALID_MM))
    {
        filter->reject_count++;
        return last;
    }

    if ((last != 0U) && (sample_mm > last) && ((uint16_t)(sample_mm - last) > MS53L1M_JUMP_REJECT_MM))
    {
        filter->reject_count++;
        sample_mm = last;
    }

    atk_ms53l1m_push_u16(filter->median_window, MS53L1M_MEDIAN_WINDOW, &filter->median_index, &filter->median_count, sample_mm);
    median_mm = atk_ms53l1m_median(filter->median_window, filter->median_count);

    atk_ms53l1m_push_u16(filter->avg_window, MS53L1M_AVG_WINDOW, &filter->avg_index, &filter->avg_count, median_mm);
    filtered_mm = atk_ms53l1m_average(filter->avg_window, filter->avg_count);

    filter->last_accepted_mm = filtered_mm;
    return filtered_mm;
}

static void atk_ms53l1m_update_flags(ATK_MS53L1M_Device_t *dev, uint16_t distance_mm)
{
    ATK_MS53L1M_Distance_t *distance = &dev->distance;

    distance->is_warning = (distance_mm <= MS53L1M_WARN_DISTANCE_MM) ? 1U : 0U;
    distance->is_slow = (distance_mm <= MS53L1M_SLOW_DISTANCE_MM) ? 1U : 0U;

    if (distance_mm <= MS53L1M_STOP_DISTANCE_MM)
    {
        if (dev->filter.too_close_count < 255U)
        {
            dev->filter.too_close_count++;
        }
    }
    else
    {
        dev->filter.too_close_count = 0U;
    }

    distance->is_too_close = (dev->filter.too_close_count >= MS53L1M_TOO_CLOSE_CONFIRM_COUNT) ? 1U : 0U;
}

void ATK_MS53L1M_Reset(ATK_MS53L1M_Device_t *dev)
{
    if (dev == NULL)
    {
        return;
    }

    memset(dev, 0, sizeof(*dev));
    dev->state = ATK_MS53L1M_STATE_RESET;
    dev->last_error = ATK_MS53L1M_ERROR_HAL;
}

ATK_MS53L1M_Status_t ATK_MS53L1M_Init(ATK_MS53L1M_Device_t *dev, I2C_HandleTypeDef *hi2c)
{
    VL53L1_Error vlerr;
    uint16_t module_id = 0U;

    if ((dev == NULL) || (hi2c == NULL))
    {
        return ATK_MS53L1M_ERROR_NULL;
    }

    ATK_MS53L1M_Reset(dev);
    dev->i2c = hi2c;
    dev->addr_7bit = ATK_MS53L1M_I2C_ADDR_7BIT;
    dev->addr_hal = ATK_MS53L1M_I2C_ADDR_HAL;
    dev->init_ms = HAL_GetTick();
    dev->last_poll_ms = HAL_GetTick();

    g_vl53l1_dev.I2cDevAddr = dev->addr_hal;
    g_vl53l1_dev.comms_type = 1U;
    g_vl53l1_dev.comms_speed_khz = 100U;
    g_vl53l1_dev.new_data_ready_poll_duration_ms = 0U;
    g_vl53l1_dev.I2cHandle = hi2c;

    if (HAL_I2C_IsDeviceReady(hi2c, dev->addr_hal, 2U, 10U) != HAL_OK)
    {
        dev->state = ATK_MS53L1M_STATE_ERROR;
        dev->last_error = ATK_MS53L1M_ERROR_NOT_FOUND;
        return dev->last_error;
    }

    vlerr = VL53L1_RdWord(&g_vl53l1_dev, ATK_MS53L1M_MODULE_ID_REG, &module_id);
    if (vlerr != VL53L1_ERROR_NONE)
    {
        dev->state = ATK_MS53L1M_STATE_ERROR;
        dev->last_error = ATK_MS53L1M_ERROR_VL53L1;
        return dev->last_error;
    }

    dev->module_id = module_id;
    if (module_id != ATK_MS53L1M_MODULE_ID_EXPECTED)
    {
        dev->state = ATK_MS53L1M_STATE_ERROR;
        dev->last_error = ATK_MS53L1M_ERROR_ID;
        return dev->last_error;
    }

    vlerr = VL53L1_WaitDeviceBooted(&g_vl53l1_dev);
    if (vlerr != VL53L1_ERROR_NONE)
    {
        dev->state = ATK_MS53L1M_STATE_ERROR;
        dev->last_error = ATK_MS53L1M_ERROR_VL53L1;
        return dev->last_error;
    }

    vlerr = VL53L1_DataInit(&g_vl53l1_dev);
    if (vlerr != VL53L1_ERROR_NONE)
    {
        dev->state = ATK_MS53L1M_STATE_ERROR;
        dev->last_error = ATK_MS53L1M_ERROR_VL53L1;
        return dev->last_error;
    }

    vlerr = VL53L1_StaticInit(&g_vl53l1_dev);
    if (vlerr != VL53L1_ERROR_NONE)
    {
        dev->state = ATK_MS53L1M_STATE_ERROR;
        dev->last_error = ATK_MS53L1M_ERROR_VL53L1;
        return dev->last_error;
    }

    vlerr = VL53L1_SetDistanceMode(&g_vl53l1_dev, VL53L1_DISTANCEMODE_LONG);
    if (vlerr != VL53L1_ERROR_NONE)
    {
        dev->state = ATK_MS53L1M_STATE_ERROR;
        dev->last_error = ATK_MS53L1M_ERROR_VL53L1;
        return dev->last_error;
    }

    vlerr = VL53L1_SetMeasurementTimingBudgetMicroSeconds(&g_vl53l1_dev, 50000U);
    if (vlerr != VL53L1_ERROR_NONE)
    {
        dev->state = ATK_MS53L1M_STATE_ERROR;
        dev->last_error = ATK_MS53L1M_ERROR_VL53L1;
        return dev->last_error;
    }

    vlerr = VL53L1_SetInterMeasurementPeriodMilliSeconds(&g_vl53l1_dev, 50U);
    if (vlerr != VL53L1_ERROR_NONE)
    {
        dev->state = ATK_MS53L1M_STATE_ERROR;
        dev->last_error = ATK_MS53L1M_ERROR_VL53L1;
        return dev->last_error;
    }

    vlerr = VL53L1_StartMeasurement(&g_vl53l1_dev);
    if (vlerr != VL53L1_ERROR_NONE)
    {
        dev->state = ATK_MS53L1M_STATE_ERROR;
        dev->last_error = ATK_MS53L1M_ERROR_VL53L1;
        return dev->last_error;
    }

    atk_ms53l1m_reset_filter(&dev->filter);
    dev->state = ATK_MS53L1M_STATE_READY;
    dev->last_error = ATK_MS53L1M_OK;
    dev->initialized = 1U;
    return ATK_MS53L1M_OK;
}

ATK_MS53L1M_Status_t ATK_MS53L1M_IsDataReady(ATK_MS53L1M_Device_t *dev, uint8_t *ready)
{
    VL53L1_Error vlerr;

    if ((dev == NULL) || (ready == NULL) || (dev->initialized == 0U))
    {
        return ATK_MS53L1M_ERROR_NULL;
    }

    vlerr = VL53L1_GetMeasurementDataReady(&g_vl53l1_dev, ready);
    if (vlerr != VL53L1_ERROR_NONE)
    {
        dev->last_error = ATK_MS53L1M_ERROR_VL53L1;
        return dev->last_error;
    }

    return ATK_MS53L1M_OK;
}

ATK_MS53L1M_Status_t ATK_MS53L1M_ReadDistance(ATK_MS53L1M_Device_t *dev, uint16_t *distance_mm)
{
    VL53L1_RangingMeasurementData_t data;
    VL53L1_Error vlerr;
    uint8_t ready = 0U;
    uint16_t filtered_mm;

    if ((dev == NULL) || (distance_mm == NULL) || (dev->initialized == 0U))
    {
        return ATK_MS53L1M_ERROR_NULL;
    }

    vlerr = VL53L1_GetMeasurementDataReady(&g_vl53l1_dev, &ready);
    if (vlerr != VL53L1_ERROR_NONE)
    {
        dev->error_count++;
        dev->consecutive_error_count++;
        dev->last_error = ATK_MS53L1M_ERROR_VL53L1;
        return dev->last_error;
    }

    if (ready == 0U)
    {
        dev->last_error = ATK_MS53L1M_ERROR_NOT_READY;
        return dev->last_error;
    }

    vlerr = VL53L1_GetRangingMeasurementData(&g_vl53l1_dev, &data);
    if (vlerr != VL53L1_ERROR_NONE)
    {
        dev->error_count++;
        dev->consecutive_error_count++;
        dev->last_error = ATK_MS53L1M_ERROR_VL53L1;
        return dev->last_error;
    }

    if ((data.RangeStatus != 0U) || (data.RangeMilliMeter <= 0))
    {
        dev->error_count++;
        dev->consecutive_error_count++;
        dev->last_error = ATK_MS53L1M_ERROR_RANGE;
        (void)VL53L1_ClearInterruptAndStartMeasurement(&g_vl53l1_dev);
        return dev->last_error;
    }

    filtered_mm = atk_ms53l1m_filter_sample(dev, (uint16_t)data.RangeMilliMeter);
    if (filtered_mm == 0U)
    {
        dev->error_count++;
        dev->consecutive_error_count++;
        dev->last_error = ATK_MS53L1M_ERROR_RANGE;
        (void)VL53L1_ClearInterruptAndStartMeasurement(&g_vl53l1_dev);
        return dev->last_error;
    }

    dev->distance.raw_distance_mm = (uint16_t)data.RangeMilliMeter;
    dev->distance.filtered_distance_mm = filtered_mm;
    dev->distance.last_valid_distance_mm = filtered_mm;
    dev->distance.valid = 1U;
    dev->distance.sample_count++;
    dev->distance.last_valid_ms = HAL_GetTick();
    atk_ms53l1m_update_flags(dev, filtered_mm);

    *distance_mm = filtered_mm;
    dev->last_error = ATK_MS53L1M_OK;
    dev->consecutive_error_count = 0U;

    vlerr = VL53L1_ClearInterruptAndStartMeasurement(&g_vl53l1_dev);
    if (vlerr != VL53L1_ERROR_NONE)
    {
        dev->error_count++;
        dev->consecutive_error_count++;
        dev->last_error = ATK_MS53L1M_ERROR_VL53L1;
    }

    return dev->last_error;
}

ATK_MS53L1M_Status_t ATK_MS53L1M_Poll(ATK_MS53L1M_Device_t *dev, uint32_t now_ms)
{
    uint16_t distance_mm = 0U;

    if ((dev == NULL) || (dev->initialized == 0U))
    {
        return ATK_MS53L1M_ERROR_NULL;
    }

    if ((uint32_t)(now_ms - dev->last_poll_ms) < MS53L1M_UPDATE_PERIOD_MS)
    {
        return ATK_MS53L1M_ERROR_NOT_READY;
    }
    dev->last_poll_ms = now_ms;

    if (ATK_MS53L1M_ReadDistance(dev, &distance_mm) == ATK_MS53L1M_OK)
    {
        dev->distance.valid = 1U;
        dev->distance.last_valid_ms = now_ms;
        dev->state = ATK_MS53L1M_STATE_RUNNING;
        return ATK_MS53L1M_OK;
    }

    if ((dev->distance.last_valid_ms != 0U) && (ATK_MS53L1M_IsTimedOut(dev, now_ms) != 0U))
    {
        dev->distance.valid = 0U;
        dev->distance.is_warning = 0U;
        dev->distance.is_slow = 0U;
        dev->distance.is_too_close = 0U;
        dev->state = ATK_MS53L1M_STATE_ERROR;
        dev->last_error = ATK_MS53L1M_ERROR_TIMEOUT;
    }

    return dev->last_error;
}

ATK_MS53L1M_Status_t ATK_MS53L1M_SetDistanceMode(ATK_MS53L1M_Device_t *dev, ATK_MS53L1M_DistanceMode_t mode)
{
    VL53L1_DistanceModes vl_mode;
    VL53L1_Error vlerr;

    if ((dev == NULL) || (dev->initialized == 0U))
    {
        return ATK_MS53L1M_ERROR_NULL;
    }

    switch (mode)
    {
    case ATK_MS53L1M_DISTANCE_SHORT:
        vl_mode = VL53L1_DISTANCEMODE_SHORT;
        break;
    case ATK_MS53L1M_DISTANCE_MEDIUM:
        vl_mode = VL53L1_DISTANCEMODE_MEDIUM;
        break;
    case ATK_MS53L1M_DISTANCE_LONG:
    default:
        vl_mode = VL53L1_DISTANCEMODE_LONG;
        break;
    }

    vlerr = VL53L1_SetDistanceMode(&g_vl53l1_dev, vl_mode);
    if (vlerr != VL53L1_ERROR_NONE)
    {
        dev->last_error = ATK_MS53L1M_ERROR_VL53L1;
        return dev->last_error;
    }

    return ATK_MS53L1M_OK;
}

uint8_t ATK_MS53L1M_IsTimedOut(const ATK_MS53L1M_Device_t *dev, uint32_t now_ms)
{
    if ((dev == NULL) || (dev->distance.last_valid_ms == 0U))
    {
        return 1U;
    }

    return ((uint32_t)(now_ms - dev->distance.last_valid_ms) > MS53L1M_DATA_TIMEOUT_MS) ? 1U : 0U;
}
