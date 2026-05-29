#ifndef ATK_MS53L1M_H
#define ATK_MS53L1M_H

#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ATK-MS53L1M / VL53L1X: 7-bit I2C address 0x29, HAL uses 0x52. */
#define ATK_MS53L1M_I2C_ADDR_7BIT          (0x29U)
#define ATK_MS53L1M_I2C_ADDR_HAL           ((uint16_t)(ATK_MS53L1M_I2C_ADDR_7BIT << 1U))
#define ATK_MS53L1M_I2C_ADDR_8BIT          ATK_MS53L1M_I2C_ADDR_HAL

#define ATK_MS53L1M_I2C_TIMEOUT_MS         (5U)
#define ATK_MS53L1M_UPDATE_PERIOD_MS       (20U)
#define ATK_MS53L1M_DATA_TIMEOUT_MS        (500U)
#define ATK_MS53L1M_RECOVER_PERIOD_MS      (1000U)

#define ATK_MS53L1M_MIN_VALID_MM           (40U)
#define ATK_MS53L1M_MAX_VALID_MM           (4000U)
#define ATK_MS53L1M_WARN_DISTANCE_MM       (600U)
#define ATK_MS53L1M_SLOW_DISTANCE_MM       (300U)
#define ATK_MS53L1M_STOP_DISTANCE_MM       (150U)

#define ATK_MS53L1M_MEDIAN_WINDOW          (3U)
#define ATK_MS53L1M_AVG_WINDOW             (5U)
#define ATK_MS53L1M_JUMP_REJECT_MM         (300U)
#define ATK_MS53L1M_STOP_CONFIRM_COUNT     (2U)
#define ATK_MS53L1M_JUMP_CONFIRM_COUNT     (3U)

typedef enum
{
    ATK_MS53L1M_STATE_UNINIT = 0,
    ATK_MS53L1M_STATE_RUNNING,
    ATK_MS53L1M_STATE_ERROR
} ATK_MS53L1M_State_t;

typedef enum
{
    ATK_MS53L1M_ERROR_NONE = 0,
    ATK_MS53L1M_ERROR_PARAM,
    ATK_MS53L1M_ERROR_I2C,
    ATK_MS53L1M_ERROR_NOT_READY,
    ATK_MS53L1M_ERROR_RANGE_INVALID,
    ATK_MS53L1M_ERROR_TIMEOUT
} ATK_MS53L1M_Error_t;

typedef enum
{
    ATK_MS53L1M_RESULT_OK = 0,
    ATK_MS53L1M_RESULT_BUSY,
    ATK_MS53L1M_RESULT_NO_UPDATE,
    ATK_MS53L1M_RESULT_I2C_ERROR,
    ATK_MS53L1M_RESULT_INVALID_DATA,
    ATK_MS53L1M_RESULT_TIMEOUT,
    ATK_MS53L1M_RESULT_PARAM_ERROR
} ATK_MS53L1M_Result_t;

typedef struct
{
    uint16_t raw_distance_mm;
    uint16_t filtered_distance_mm;
    uint16_t last_valid_distance_mm;
    uint8_t valid;
    uint8_t is_warning;
    uint8_t is_slow;
    uint8_t is_too_close;
    uint32_t last_valid_ms;
    uint32_t sample_count;
} ATK_MS53L1M_Distance_t;

typedef struct
{
    uint16_t median_window[ATK_MS53L1M_MEDIAN_WINDOW];
    uint16_t avg_window[ATK_MS53L1M_AVG_WINDOW];
    uint8_t median_index;
    uint8_t avg_index;
    uint8_t median_count;
    uint8_t avg_count;
    uint16_t last_accepted_mm;
    uint16_t reject_count;
    uint8_t close_confirm_count;
} ATK_MS53L1M_Filter_t;

typedef struct
{
    I2C_HandleTypeDef *i2c;
    uint8_t addr_7bit;
    uint16_t addr_hal;

    ATK_MS53L1M_State_t state;
    ATK_MS53L1M_Error_t last_error;
    ATK_MS53L1M_Distance_t distance;
    ATK_MS53L1M_Filter_t filter;

    uint16_t error_count;
    uint16_t consecutive_error_count;
    uint32_t last_poll_ms;
    uint32_t last_recover_ms;
} ATK_MS53L1M_Device_t;

ATK_MS53L1M_Result_t ATK_MS53L1M_Init(ATK_MS53L1M_Device_t *dev, I2C_HandleTypeDef *i2c, uint32_t now_ms);
ATK_MS53L1M_Result_t ATK_MS53L1M_StartDefault(ATK_MS53L1M_Device_t *dev, uint32_t now_ms);
ATK_MS53L1M_Result_t ATK_MS53L1M_Update(ATK_MS53L1M_Device_t *dev, uint32_t now_ms);
ATK_MS53L1M_Result_t ATK_MS53L1M_ReadDistanceRaw(ATK_MS53L1M_Device_t *dev, uint16_t *distance_mm);
ATK_MS53L1M_Result_t ATK_MS53L1M_FilterUpdate(ATK_MS53L1M_Device_t *dev, uint16_t raw_mm, uint32_t now_ms);
ATK_MS53L1M_Result_t ATK_MS53L1M_GetDistance(const ATK_MS53L1M_Device_t *dev, ATK_MS53L1M_Distance_t *out);
ATK_MS53L1M_Result_t ATK_MS53L1M_Recover(ATK_MS53L1M_Device_t *dev, uint32_t now_ms);
ATK_MS53L1M_State_t ATK_MS53L1M_GetState(const ATK_MS53L1M_Device_t *dev);
uint8_t ATK_MS53L1M_IsFresh(const ATK_MS53L1M_Device_t *dev, uint32_t now_ms);

/* Weak platform hooks. Default implementation uses minimal VL53L1X direct-register access.
 * If your direct VL53L1X module needs the official ST ULD init sequence, override these
 * four functions in a separate .c file and keep the upper application code unchanged.
 */
HAL_StatusTypeDef ATK_MS53L1M_PlatformStartRanging(ATK_MS53L1M_Device_t *dev);
HAL_StatusTypeDef ATK_MS53L1M_PlatformIsDataReady(ATK_MS53L1M_Device_t *dev, uint8_t *ready);
HAL_StatusTypeDef ATK_MS53L1M_PlatformReadDistance(ATK_MS53L1M_Device_t *dev, uint16_t *distance_mm);
HAL_StatusTypeDef ATK_MS53L1M_PlatformClearInterrupt(ATK_MS53L1M_Device_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* ATK_MS53L1M_H */
