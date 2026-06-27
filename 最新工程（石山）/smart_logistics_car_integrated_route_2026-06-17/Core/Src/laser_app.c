#include "laser_app.h"

extern I2C_HandleTypeDef hi2c1;

#define LASER_APP_RETRY_PERIOD_MS 1000U

static ATK_MS53L1M_Device_t g_laser;
static uint32_t g_laser_next_retry_ms = 0U;

volatile ATK_MS53L1M_Status_t g_laser_status = ATK_MS53L1M_ERROR_HAL;
volatile uint16_t g_laser_distance_mm = 0U;
volatile uint16_t g_laser_raw_distance_mm = 0U;
volatile uint16_t g_laser_module_id = 0U;
volatile uint32_t g_laser_sample_count = 0U;
volatile uint32_t g_laser_last_tick_ms = 0U;
volatile uint16_t g_laser_error_count = 0U;
volatile uint8_t laser_ok = 0U;
volatile uint8_t laser_warning = 0U;
volatile uint8_t laser_slow = 0U;
volatile uint8_t laser_too_close = 0U;

static void laser_app_publish(uint32_t now_ms)
{
    const ATK_MS53L1M_Distance_t *distance = &g_laser.distance;

    laser_ok = (distance->valid != 0U) && (ATK_MS53L1M_IsTimedOut(&g_laser, now_ms) == 0U);
    laser_warning = distance->is_warning;
    laser_slow = distance->is_slow;
    laser_too_close = distance->is_too_close;

    g_laser_status = g_laser.last_error;
    g_laser_module_id = g_laser.module_id;
    g_laser_raw_distance_mm = distance->raw_distance_mm;
    g_laser_distance_mm = distance->filtered_distance_mm;
    g_laser_sample_count = distance->sample_count;
    g_laser_last_tick_ms = distance->last_valid_ms;
    g_laser_error_count = g_laser.error_count;
}

void Laser_App_Init(void)
{
    uint32_t now_ms = HAL_GetTick();

    g_laser_status = ATK_MS53L1M_Init(&g_laser, &hi2c1);
    if (g_laser_status != ATK_MS53L1M_OK)
    {
        g_laser_next_retry_ms = now_ms + LASER_APP_RETRY_PERIOD_MS;
    }
    laser_app_publish(now_ms);
}

void Laser_App_Loop(void)
{
    uint32_t now_ms = HAL_GetTick();

    if (g_laser.initialized == 0U)
    {
        if ((int32_t)(now_ms - g_laser_next_retry_ms) >= 0)
        {
            g_laser_status = ATK_MS53L1M_Init(&g_laser, &hi2c1);
            g_laser_next_retry_ms = now_ms + LASER_APP_RETRY_PERIOD_MS;
        }
        laser_app_publish(now_ms);
        return;
    }

    g_laser_status = ATK_MS53L1M_Poll(&g_laser, now_ms);
    if ((g_laser_status != ATK_MS53L1M_OK) &&
        (g_laser_status != ATK_MS53L1M_ERROR_NOT_READY) &&
        (g_laser_status != ATK_MS53L1M_ERROR_TIMEOUT))
    {
        g_laser_next_retry_ms = now_ms + LASER_APP_RETRY_PERIOD_MS;
    }

    if ((g_laser_status == ATK_MS53L1M_ERROR_TIMEOUT) ||
        (g_laser.consecutive_error_count >= 10U))
    {
        g_laser.initialized = 0U;
        g_laser_next_retry_ms = now_ms + LASER_APP_RETRY_PERIOD_MS;
    }

    laser_app_publish(now_ms);
}

const ATK_MS53L1M_Device_t *Laser_App_GetDevice(void)
{
    return &g_laser;
}
