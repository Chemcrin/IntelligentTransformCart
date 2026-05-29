#ifndef LASER_APP_H
#define LASER_APP_H

#include "atk_ms53l1m.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern volatile uint16_t g_laser_raw_distance_mm;
extern volatile uint16_t g_laser_filtered_distance_mm;
extern volatile uint16_t g_laser_distance_mm;
extern volatile uint8_t g_laser_valid;
extern volatile uint8_t g_laser_ok;
extern volatile uint8_t g_laser_is_warning;
extern volatile uint8_t g_laser_is_slow;
extern volatile uint8_t g_laser_is_too_close;
extern volatile uint32_t g_laser_last_valid_ms;
extern volatile uint16_t g_laser_error_count;
extern volatile uint8_t g_laser_state;

void Laser_App_Init(I2C_HandleTypeDef *i2c);
void Laser_App_Loop(void);
const ATK_MS53L1M_Device_t *Laser_App_GetDevice(void);

#ifdef __cplusplus
}
#endif

#endif /* LASER_APP_H */
