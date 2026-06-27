#ifndef ATK_MS53L1M_H
#define ATK_MS53L1M_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ATK_MS53L1M_I2C_ADDR_7BIT              0x29U
#define ATK_MS53L1M_I2C_ADDR_8BIT              (ATK_MS53L1M_I2C_ADDR_7BIT << 1)
#define ATK_MS53L1M_I2C_ADDR_HAL               ATK_MS53L1M_I2C_ADDR_8BIT
#define ATK_MS53L1M_MODULE_ID_REG              0x010FU
#define ATK_MS53L1M_MODULE_ID_EXPECTED         0xEACCU

#define MS53L1M_UPDATE_PERIOD_MS               150U
#define MS53L1M_DATA_TIMEOUT_MS                500U
#define MS53L1M_MIN_VALID_MM                   40U
#define MS53L1M_MAX_VALID_MM                   4000U
#define MS53L1M_WARN_DISTANCE_MM               500U
#define MS53L1M_SLOW_DISTANCE_MM               300U
#define MS53L1M_STOP_DISTANCE_MM               100U
#define MS53L1M_MEDIAN_WINDOW                  3U
#define MS53L1M_AVG_WINDOW                     5U
#define MS53L1M_JUMP_REJECT_MM                 300U
#define MS53L1M_TOO_CLOSE_CONFIRM_COUNT        2U

typedef enum
{
    ATK_MS53L1M_OK = 0,
    ATK_MS53L1M_ERROR_NULL,
    ATK_MS53L1M_ERROR_HAL,
    ATK_MS53L1M_ERROR_NOT_FOUND,
    ATK_MS53L1M_ERROR_ID,
    ATK_MS53L1M_ERROR_VL53L1,
    ATK_MS53L1M_ERROR_NOT_READY,
    ATK_MS53L1M_ERROR_RANGE,
    ATK_MS53L1M_ERROR_TIMEOUT
} ATK_MS53L1M_Status_t;

typedef enum
{
    ATK_MS53L1M_STATE_RESET = 0,
    ATK_MS53L1M_STATE_READY,
    ATK_MS53L1M_STATE_RUNNING,
    ATK_MS53L1M_STATE_ERROR
} ATK_MS53L1M_State_t;

typedef enum
{
    ATK_MS53L1M_DISTANCE_SHORT = 1,
    ATK_MS53L1M_DISTANCE_MEDIUM,
    ATK_MS53L1M_DISTANCE_LONG
} ATK_MS53L1M_DistanceMode_t;

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
    uint16_t median_window[MS53L1M_MEDIAN_WINDOW];
    uint16_t avg_window[MS53L1M_AVG_WINDOW];
    uint8_t median_count;
    uint8_t median_index;
    uint8_t avg_count;
    uint8_t avg_index;
    uint16_t last_accepted_mm;
    uint16_t reject_count;
    uint8_t too_close_count;
} ATK_MS53L1M_Filter_t;

typedef struct
{
    I2C_HandleTypeDef *i2c;
    uint8_t addr_7bit;
    uint16_t addr_hal;
    uint16_t module_id;
    ATK_MS53L1M_State_t state;
    ATK_MS53L1M_Status_t last_error;
    ATK_MS53L1M_Distance_t distance;
    ATK_MS53L1M_Filter_t filter;
    uint32_t init_ms;
    uint32_t last_poll_ms;
    uint16_t error_count;
    uint16_t consecutive_error_count;
    uint8_t initialized;
} ATK_MS53L1M_Device_t;

typedef ATK_MS53L1M_Status_t ATK_MS53L1M_StatusTypeDef;
typedef ATK_MS53L1M_Device_t ATK_MS53L1M_HandleTypeDef;

void ATK_MS53L1M_Reset(ATK_MS53L1M_Device_t *dev);
ATK_MS53L1M_Status_t ATK_MS53L1M_Init(ATK_MS53L1M_Device_t *dev, I2C_HandleTypeDef *hi2c);
ATK_MS53L1M_Status_t ATK_MS53L1M_Poll(ATK_MS53L1M_Device_t *dev, uint32_t now_ms);
ATK_MS53L1M_Status_t ATK_MS53L1M_ReadDistance(ATK_MS53L1M_Device_t *dev, uint16_t *distance_mm);
ATK_MS53L1M_Status_t ATK_MS53L1M_IsDataReady(ATK_MS53L1M_Device_t *dev, uint8_t *ready);
ATK_MS53L1M_Status_t ATK_MS53L1M_SetDistanceMode(ATK_MS53L1M_Device_t *dev, ATK_MS53L1M_DistanceMode_t mode);
uint8_t ATK_MS53L1M_IsTimedOut(const ATK_MS53L1M_Device_t *dev, uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif
