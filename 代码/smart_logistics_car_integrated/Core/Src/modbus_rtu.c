#include "modbus_rtu.h"
#include "mecanum_config.h"
#include <string.h>

static UART_HandleTypeDef *g_modbus_uart = 0;
static ModbusState_t g_modbus_state = MODBUS_STATE_IDLE;

static void Modbus_DelayUs(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = (HAL_RCC_GetHCLKFreq() / 1000000u) * us;

    if ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0u) {
        return;
    }

    while ((DWT->CYCCNT - start) < cycles) {
    }
}

static void Modbus_EnableDwtCycleCounter(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void RS485_RX_ENABLE(void)
{
#if (MODBUS_USE_RS485_DIR_GPIO != 0u)
    HAL_GPIO_WritePin(RS485_DIR_GPIO_Port, RS485_DIR_Pin, RS485_DIR_RX_LEVEL);
#endif
}

void RS485_TX_ENABLE(void)
{
#if (MODBUS_USE_RS485_DIR_GPIO != 0u)
    HAL_GPIO_WritePin(RS485_DIR_GPIO_Port, RS485_DIR_Pin, RS485_DIR_TX_LEVEL);
#endif
}

void Modbus_Init(UART_HandleTypeDef *huart)
{
    g_modbus_uart = huart;
    g_modbus_state = MODBUS_STATE_IDLE;
    Modbus_EnableDwtCycleCounter();
    RS485_RX_ENABLE();
}

uint16_t Modbus_CRC16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFFu;
    uint16_t i;
    uint8_t bit;

    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (bit = 0; bit < 8u; bit++) {
            if ((crc & 0x0001u) != 0u) {
                crc = (uint16_t)((crc >> 1) ^ 0xA001u);
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}

MecanumResult_t Modbus_SendRaw(const uint8_t *tx, uint16_t tx_len)
{
    HAL_StatusTypeDef hal_ret;

    if ((g_modbus_uart == 0) || (tx == 0) || (tx_len == 0u)) {
        return MECANUM_BAD_PARAM;
    }
    if (g_modbus_state != MODBUS_STATE_IDLE) {
        return MECANUM_BUSY;
    }

    g_modbus_state = MODBUS_STATE_TX;
    RS485_RX_ENABLE();
    HAL_Delay(MODBUS_INTER_FRAME_GAP_MS);
    RS485_TX_ENABLE();
    Modbus_DelayUs(MODBUS_TX_SETTLE_US);

    hal_ret = HAL_UART_Transmit(g_modbus_uart, (uint8_t *)tx, tx_len, MODBUS_RESPONSE_TIMEOUT_MS);
    if (hal_ret == HAL_OK) {
        while (__HAL_UART_GET_FLAG(g_modbus_uart, UART_FLAG_TC) == RESET) {
        }
    }

    Modbus_DelayUs(MODBUS_TX_SETTLE_US);
    RS485_RX_ENABLE();
    g_modbus_state = MODBUS_STATE_IDLE;

    return (hal_ret == HAL_OK) ? MECANUM_OK : MECANUM_ERROR;
}

MecanumResult_t Modbus_Transceive(const uint8_t *tx, uint16_t tx_len,
                                  uint8_t *rx, uint16_t rx_max_len,
                                  uint16_t *rx_len, uint32_t timeout_ms)
{
    MecanumResult_t ret;
    HAL_StatusTypeDef hal_ret;
    uint16_t crc_calc;
    uint16_t crc_recv;

    if ((rx == 0) || (rx_len == 0) || (rx_max_len < 5u)) {
        return MECANUM_BAD_PARAM;
    }

    *rx_len = 0u;
    ret = Modbus_SendRaw(tx, tx_len);
    if (ret != MECANUM_OK) {
        return ret;
    }

    g_modbus_state = MODBUS_STATE_RX;
    memset(rx, 0, rx_max_len);

    hal_ret = HAL_UART_Receive(g_modbus_uart, rx, rx_max_len, timeout_ms);
    if (hal_ret != HAL_OK) {
        g_modbus_state = MODBUS_STATE_IDLE;
        return MECANUM_TIMEOUT;
    }

    *rx_len = rx_max_len;
    crc_calc = Modbus_CRC16(rx, (uint16_t)(rx_max_len - 2u));
    crc_recv = (uint16_t)rx[rx_max_len - 2u] | ((uint16_t)rx[rx_max_len - 1u] << 8);
    g_modbus_state = MODBUS_STATE_IDLE;

    if (crc_calc != crc_recv) {
        return MECANUM_CRC_ERROR;
    }
    if ((rx[1] & 0x80u) != 0u) {
        return MECANUM_ERROR;
    }

    return MECANUM_OK;
}

MecanumResult_t Modbus_WriteMultipleRegisters(uint8_t addr, uint16_t start_reg,
                                              const uint16_t *regs, uint16_t reg_count,
                                              uint8_t wait_response)
{
    uint8_t frame[64];
    uint16_t idx = 0u;
    uint16_t i;
    uint16_t crc;
    uint8_t rx[8];
    uint16_t rx_len;

    if ((regs == 0) || (reg_count == 0u) || (reg_count > 20u)) {
        return MECANUM_BAD_PARAM;
    }

    frame[idx++] = addr;
    frame[idx++] = 0x10u;
    frame[idx++] = (uint8_t)(start_reg >> 8);
    frame[idx++] = (uint8_t)(start_reg & 0xFFu);
    frame[idx++] = (uint8_t)(reg_count >> 8);
    frame[idx++] = (uint8_t)(reg_count & 0xFFu);
    frame[idx++] = (uint8_t)(reg_count * 2u);

    for (i = 0; i < reg_count; i++) {
        frame[idx++] = (uint8_t)(regs[i] >> 8);
        frame[idx++] = (uint8_t)(regs[i] & 0xFFu);
    }

    crc = Modbus_CRC16(frame, idx);
    frame[idx++] = (uint8_t)(crc & 0xFFu);
    frame[idx++] = (uint8_t)(crc >> 8);

    if ((addr == MOTOR_ADDR_BROAD) || (wait_response == 0u)) {
        return Modbus_SendRaw(frame, idx);
    }

    return Modbus_Transceive(frame, idx, rx, sizeof(rx), &rx_len, MODBUS_RESPONSE_TIMEOUT_MS);
}

MecanumResult_t Modbus_ReadInputRegisters(uint8_t addr, uint16_t start_reg,
                                          uint16_t reg_count, uint16_t *out_regs)
{
    uint8_t frame[8];
    uint8_t rx[32];
    uint16_t idx = 0u;
    uint16_t crc;
    uint16_t rx_len;
    uint16_t i;
    uint16_t expected_len;
    MecanumResult_t ret;

    if ((out_regs == 0) || (reg_count == 0u) || (reg_count > 12u) || (addr == MOTOR_ADDR_BROAD)) {
        return MECANUM_BAD_PARAM;
    }

    frame[idx++] = addr;
    frame[idx++] = 0x04u;
    frame[idx++] = (uint8_t)(start_reg >> 8);
    frame[idx++] = (uint8_t)(start_reg & 0xFFu);
    frame[idx++] = (uint8_t)(reg_count >> 8);
    frame[idx++] = (uint8_t)(reg_count & 0xFFu);
    crc = Modbus_CRC16(frame, idx);
    frame[idx++] = (uint8_t)(crc & 0xFFu);
    frame[idx++] = (uint8_t)(crc >> 8);

    expected_len = (uint16_t)(5u + reg_count * 2u);
    ret = Modbus_Transceive(frame, idx, rx, expected_len, &rx_len, MODBUS_RESPONSE_TIMEOUT_MS);
    if (ret != MECANUM_OK) {
        return ret;
    }
    if ((rx[0] != addr) || (rx[1] != 0x04u) || (rx[2] != (uint8_t)(reg_count * 2u))) {
        return MECANUM_ERROR;
    }

    for (i = 0; i < reg_count; i++) {
        out_regs[i] = ((uint16_t)rx[3u + i * 2u] << 8) | rx[4u + i * 2u];
    }

    return MECANUM_OK;
}

ModbusState_t Modbus_GetState(void)
{
    return g_modbus_state;
}
