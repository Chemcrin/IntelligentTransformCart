#include "laser_app.h"

static ATK_MS53L1M_Device_t s_ms53l1m;

volatile uint16_t g_laser_raw_distance_mm = 0U;
volatile uint16_t g_laser_filtered_distance_mm = 0U;
volatile uint16_t g_laser_distance_mm = 0U;
volatile uint8_t g_laser_valid = 0U;
volatile uint8_t g_laser_ok = 0U;
volatile uint8_t g_laser_is_warning = 0U;
volatile uint8_t g_laser_is_slow = 0U;
volatile uint8_t g_laser_is_too_close = 0U;
volatile uint32_t g_laser_last_valid_ms = 0U;
volatile uint16_t g_laser_error_count = 0U;
volatile uint8_t g_laser_state = ATK_MS53L1M_STATE_UNINIT;

static void laser_app_sync_public_vars(void)
{
    ATK_MS53L1M_Distance_t dist;
    (void)ATK_MS53L1M_GetDistance(&s_ms53l1m, &dist);

    g_laser_raw_distance_mm = dist.raw_distance_mm;
    g_laser_filtered_distance_mm = dist.filtered_distance_mm;
    g_laser_distance_mm = dist.filtered_distance_mm;
    g_laser_valid = dist.valid;
    g_laser_ok = dist.valid;
    g_laser_is_warning = dist.is_warning;
    g_laser_is_slow = dist.is_slow;
    g_laser_is_too_close = dist.is_too_close;
    g_laser_last_valid_ms = dist.last_valid_ms;
    g_laser_error_count = s_ms53l1m.error_count;
    g_laser_state = (uint8_t)s_ms53l1m.state;
}

void Laser_App_Init(I2C_HandleTypeDef *i2c)
{
    uint32_t now_ms = HAL_GetTick();
    (void)ATK_MS53L1M_Init(&s_ms53l1m, i2c, now_ms);
    laser_app_sync_public_vars();
}

void Laser_App_Loop(void)
{
    uint32_t now_ms = HAL_GetTick();
    (void)ATK_MS53L1M_Update(&s_ms53l1m, now_ms);
    laser_app_sync_public_vars();
}

const ATK_MS53L1M_Device_t *Laser_App_GetDevice(void)
{
    return &s_ms53l1m;
}
