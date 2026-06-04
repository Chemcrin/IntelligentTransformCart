#ifndef LASER_APP_H
#define LASER_APP_H

#include "atk_ms53l1m.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern volatile ATK_MS53L1M_Status_t g_laser_status;
extern volatile uint16_t g_laser_distance_mm;
extern volatile uint16_t g_laser_raw_distance_mm;
extern volatile uint16_t g_laser_module_id;
extern volatile uint32_t g_laser_sample_count;
extern volatile uint32_t g_laser_last_tick_ms;
extern volatile uint16_t g_laser_error_count;
extern volatile uint8_t laser_ok;
extern volatile uint8_t laser_warning;
extern volatile uint8_t laser_slow;
extern volatile uint8_t laser_too_close;

void Laser_App_Init(void);
void Laser_App_Loop(void);
const ATK_MS53L1M_Device_t *Laser_App_GetDevice(void);

#ifdef __cplusplus
}
#endif

#endif
