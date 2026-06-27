#include "vl53l1_platform.h"
#include "vl53l1_api.h"
#include "stm32f1xx_hal.h"

#define VL53L1_I2C_TIMEOUT_MS 5U

static uint8_t g_i2c_buffer[8];

static I2C_HandleTypeDef *vl53l1_i2c_handle(VL53L1_Dev_t *pdev)
{
    return (pdev == NULL) ? NULL : pdev->I2cHandle;
}

VL53L1_Error VL53L1_WriteMulti(VL53L1_Dev_t *pdev, uint16_t index, uint8_t *pdata, uint32_t count)
{
    I2C_HandleTypeDef *hi2c = vl53l1_i2c_handle(pdev);

    if ((hi2c == NULL) || (pdata == NULL))
    {
        return VL53L1_ERROR_CONTROL_INTERFACE;
    }

    if (HAL_I2C_Mem_Write(hi2c,
                          pdev->I2cDevAddr,
                          index,
                          I2C_MEMADD_SIZE_16BIT,
                          pdata,
                          (uint16_t)count,
                          VL53L1_I2C_TIMEOUT_MS) != HAL_OK)
    {
        return VL53L1_ERROR_CONTROL_INTERFACE;
    }

    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_ReadMulti(VL53L1_Dev_t *pdev, uint16_t index, uint8_t *pdata, uint32_t count)
{
    I2C_HandleTypeDef *hi2c = vl53l1_i2c_handle(pdev);

    if ((hi2c == NULL) || (pdata == NULL))
    {
        return VL53L1_ERROR_CONTROL_INTERFACE;
    }

    if (HAL_I2C_Mem_Read(hi2c,
                         pdev->I2cDevAddr,
                         index,
                         I2C_MEMADD_SIZE_16BIT,
                         pdata,
                         (uint16_t)count,
                         VL53L1_I2C_TIMEOUT_MS) != HAL_OK)
    {
        return VL53L1_ERROR_CONTROL_INTERFACE;
    }

    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_WrByte(VL53L1_Dev_t *pdev, uint16_t index, uint8_t data)
{
    return VL53L1_WriteMulti(pdev, index, &data, 1);
}

VL53L1_Error VL53L1_WrWord(VL53L1_Dev_t *pdev, uint16_t index, uint16_t data)
{
    g_i2c_buffer[0] = (uint8_t)(data >> 8);
    g_i2c_buffer[1] = (uint8_t)(data & 0xFF);
    return VL53L1_WriteMulti(pdev, index, g_i2c_buffer, 2);
}

VL53L1_Error VL53L1_WrDWord(VL53L1_Dev_t *pdev, uint16_t index, uint32_t data)
{
    g_i2c_buffer[0] = (uint8_t)(data >> 24);
    g_i2c_buffer[1] = (uint8_t)(data >> 16);
    g_i2c_buffer[2] = (uint8_t)(data >> 8);
    g_i2c_buffer[3] = (uint8_t)(data & 0xFF);
    return VL53L1_WriteMulti(pdev, index, g_i2c_buffer, 4);
}

VL53L1_Error VL53L1_RdByte(VL53L1_Dev_t *pdev, uint16_t index, uint8_t *pdata)
{
    return VL53L1_ReadMulti(pdev, index, pdata, 1);
}

VL53L1_Error VL53L1_RdWord(VL53L1_Dev_t *pdev, uint16_t index, uint16_t *pdata)
{
    VL53L1_Error status = VL53L1_ReadMulti(pdev, index, g_i2c_buffer, 2);
    if (status == VL53L1_ERROR_NONE)
    {
        *pdata = ((uint16_t)g_i2c_buffer[0] << 8) | g_i2c_buffer[1];
    }
    return status;
}

VL53L1_Error VL53L1_RdDWord(VL53L1_Dev_t *pdev, uint16_t index, uint32_t *pdata)
{
    VL53L1_Error status = VL53L1_ReadMulti(pdev, index, g_i2c_buffer, 4);
    if (status == VL53L1_ERROR_NONE)
    {
        *pdata = ((uint32_t)g_i2c_buffer[0] << 24) |
                 ((uint32_t)g_i2c_buffer[1] << 16) |
                 ((uint32_t)g_i2c_buffer[2] << 8) |
                 g_i2c_buffer[3];
    }
    return status;
}

VL53L1_Error VL53L1_WaitUs(VL53L1_Dev_t *pdev, int32_t wait_us)
{
    (void)pdev;
    if (wait_us > 0)
    {
        uint32_t wait_ms = ((uint32_t)wait_us + 999U) / 1000U;
        HAL_Delay(wait_ms);
    }
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_WaitMs(VL53L1_Dev_t *pdev, int32_t wait_ms)
{
    (void)pdev;
    if (wait_ms > 0)
    {
        HAL_Delay((uint32_t)wait_ms);
    }
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_GetTimerFrequency(int32_t *ptimer_freq_hz)
{
    *ptimer_freq_hz = 1000;
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_GetTickCount(uint32_t *ptime_ms)
{
    *ptime_ms = HAL_GetTick();
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_WaitValueMaskEx(VL53L1_Dev_t *pdev,
                                    uint32_t timeout_ms,
                                    uint16_t index,
                                    uint8_t value,
                                    uint8_t mask,
                                    uint32_t poll_delay_ms)
{
    uint32_t start = HAL_GetTick();
    uint8_t data = 0;

    while ((HAL_GetTick() - start) < timeout_ms)
    {
        if (VL53L1_RdByte(pdev, index, &data) != VL53L1_ERROR_NONE)
        {
            return VL53L1_ERROR_CONTROL_INTERFACE;
        }

        if ((data & mask) == value)
        {
            return VL53L1_ERROR_NONE;
        }

        HAL_Delay(poll_delay_ms);
    }

    return VL53L1_ERROR_TIME_OUT;
}
