/*
 * Optional template: override ATK_MS53L1M weak platform hooks with the official
 * ST VL53L1X Ultra Lite Driver (ULD). Do not copy this file into Core/Src until
 * you have added ST's VL53L1X_api.c/.h and platform I2C port to your project.
 *
 * Typical ST ULD calls used here:
 *   VL53L1X_SensorInit(dev_addr)
 *   VL53L1X_StartRanging(dev_addr)
 *   VL53L1X_CheckForDataReady(dev_addr, &ready)
 *   VL53L1X_GetDistance(dev_addr, &distance_mm)
 *   VL53L1X_ClearInterrupt(dev_addr)
 */

#include "atk_ms53l1m.h"
#include "VL53L1X_api.h"

HAL_StatusTypeDef ATK_MS53L1M_PlatformStartRanging(ATK_MS53L1M_Device_t *dev)
{
    uint16_t addr;

    if (dev == 0)
    {
        return HAL_ERROR;
    }

    addr = dev->addr_7bit; /* ST ULD normally uses 7-bit device address. Verify your port. */

    if (VL53L1X_SensorInit(addr) != 0)
    {
        return HAL_ERROR;
    }

    if (VL53L1X_StartRanging(addr) != 0)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef ATK_MS53L1M_PlatformIsDataReady(ATK_MS53L1M_Device_t *dev, uint8_t *ready)
{
    uint8_t data_ready = 0U;
    uint16_t addr;

    if ((dev == 0) || (ready == 0))
    {
        return HAL_ERROR;
    }

    addr = dev->addr_7bit;
    if (VL53L1X_CheckForDataReady(addr, &data_ready) != 0)
    {
        return HAL_ERROR;
    }

    *ready = data_ready;
    return HAL_OK;
}

HAL_StatusTypeDef ATK_MS53L1M_PlatformReadDistance(ATK_MS53L1M_Device_t *dev, uint16_t *distance_mm)
{
    uint16_t addr;

    if ((dev == 0) || (distance_mm == 0))
    {
        return HAL_ERROR;
    }

    addr = dev->addr_7bit;
    if (VL53L1X_GetDistance(addr, distance_mm) != 0)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef ATK_MS53L1M_PlatformClearInterrupt(ATK_MS53L1M_Device_t *dev)
{
    uint16_t addr;

    if (dev == 0)
    {
        return HAL_ERROR;
    }

    addr = dev->addr_7bit;
    if (VL53L1X_ClearInterrupt(addr) != 0)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}
