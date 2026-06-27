#ifndef CSV_DEBUG_H
#define CSV_DEBUG_H

#include "config.h"
#include "stm32f1xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (CSV_DEBUG_ENABLE != 0U)
void CSV_Debug_Loop(UART_HandleTypeDef *huart);
#else
static inline void CSV_Debug_Loop(UART_HandleTypeDef *huart)
{
    (void)huart;
}
#endif

#ifdef __cplusplus
}
#endif

#endif
