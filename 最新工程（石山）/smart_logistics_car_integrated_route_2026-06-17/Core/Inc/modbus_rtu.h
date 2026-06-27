#ifndef MODBUS_RTU_H
#define MODBUS_RTU_H

#include "stm32f1xx_hal.h"
#include "mecanum_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    MODBUS_STATE_IDLE = 0,
    MODBUS_STATE_TX,
    MODBUS_STATE_RX,
    MODBUS_STATE_ERROR
} ModbusState_t;

void Modbus_Init(UART_HandleTypeDef *huart);
uint16_t Modbus_CRC16(const uint8_t *data, uint16_t len);
MecanumResult_t Modbus_SendRaw(const uint8_t *tx, uint16_t tx_len);
MecanumResult_t Modbus_Transceive(const uint8_t *tx, uint16_t tx_len,
                                  uint8_t *rx, uint16_t rx_max_len,
                                  uint16_t *rx_len, uint32_t timeout_ms);
MecanumResult_t Modbus_WriteMultipleRegisters(uint8_t addr, uint16_t start_reg,
                                              const uint16_t *regs, uint16_t reg_count,
                                              uint8_t wait_response);
MecanumResult_t Modbus_ReadInputRegisters(uint8_t addr, uint16_t start_reg,
                                          uint16_t reg_count, uint16_t *out_regs);
ModbusState_t Modbus_GetState(void);
void RS485_RX_ENABLE(void);
void RS485_TX_ENABLE(void);

#ifdef __cplusplus
}
#endif

#endif
