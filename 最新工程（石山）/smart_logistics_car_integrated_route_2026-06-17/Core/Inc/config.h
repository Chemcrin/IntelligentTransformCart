#ifndef CONFIG_H
#define CONFIG_H

/* Fixed-route standalone build: no Raspberry Pi, no laser, no IMU dependency. */
#define FIXED_ROUTE_STANDALONE_ENABLE 1U

/* 1: enable full IMU CSV output; 0: compile CSV code out. */
#define CSV_DEBUG_ENABLE             0U

/* The official hardware uses USART1 as RS485 motor bus, so CSV debug output is
 * routed through the existing USART2 Raspberry Pi/debug link in this firmware.
 */
#define CSV_DEBUG_PERIOD_DIV         10U
#define CSV_DEBUG_TX_TIMEOUT_MS      40U

#endif
