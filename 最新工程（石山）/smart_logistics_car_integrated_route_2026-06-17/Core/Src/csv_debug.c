#include "csv_debug.h"

#if (CSV_DEBUG_ENABLE != 0U)

#include "imu_app.h"
#include <stddef.h>
#include <stdio.h>

static int32_t csv_scale_milli(float value)
{
    float scaled = value * 1000.0f;

    if (scaled >= 0.0f) {
        return (int32_t)(scaled + 0.5f);
    }
    return (int32_t)(scaled - 0.5f);
}

static int csv_append_milli(char *buf, size_t size, int offset, int32_t milli)
{
    uint32_t abs_milli;
    const char *sign = "";

    if ((buf == 0) || (offset < 0) || ((size_t)offset >= size)) {
        return offset;
    }

    if (milli < 0) {
        sign = "-";
        abs_milli = (uint32_t)(-milli);
    } else {
        abs_milli = (uint32_t)milli;
    }

    return offset + snprintf(&buf[offset],
                             size - (size_t)offset,
                             ", %s%lu.%03lu",
                             sign,
                             (unsigned long)(abs_milli / 1000UL),
                             (unsigned long)(abs_milli % 1000UL));
}

void CSV_Debug_Loop(UART_HandleTypeDef *huart)
{
    static uint32_t s_csv_counter = 0U;
    char tx[160];
    int len;

    if (huart == 0) {
        return;
    }

    if ((s_csv_counter++ % CSV_DEBUG_PERIOD_DIV) != 0U) {
        return;
    }

    len = snprintf(tx, sizeof(tx), "%lu", (unsigned long)HAL_GetTick());
    len = csv_append_milli(tx, sizeof(tx), len, csv_scale_milli(g_imu_ax_g));
    len = csv_append_milli(tx, sizeof(tx), len, csv_scale_milli(g_imu_ay_g));
    len = csv_append_milli(tx, sizeof(tx), len, csv_scale_milli(g_imu_az_g));
    len = csv_append_milli(tx, sizeof(tx), len, csv_scale_milli(g_imu_gx_dps));
    len = csv_append_milli(tx, sizeof(tx), len, csv_scale_milli(g_imu_gy_dps));
    len = csv_append_milli(tx, sizeof(tx), len, csv_scale_milli(g_imu_gz_dps));
    len = csv_append_milli(tx, sizeof(tx), len, csv_scale_milli(g_imu_roll_deg));
    len = csv_append_milli(tx, sizeof(tx), len, csv_scale_milli(g_imu_pitch_deg));
    len = csv_append_milli(tx, sizeof(tx), len, csv_scale_milli(g_imu_yaw_deg));

    if ((len < 0) || (len > ((int)sizeof(tx) - 3))) {
        return;
    }

    tx[len++] = '\r';
    tx[len++] = '\n';
    tx[len] = '\0';

    /* TODO: 改为 DMA 发送；当前 10Hz 文本输出先使用阻塞发送便于调试。 */
    (void)HAL_UART_Transmit(huart,
                            (uint8_t *)tx,
                            (uint16_t)len,
                            CSV_DEBUG_TX_TIMEOUT_MS);
}

#endif /* CSV_DEBUG_ENABLE */
