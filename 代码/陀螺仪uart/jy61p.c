#include "jy61p.h"

/* ================================================================
 * 内部状态
 * ================================================================ */

static uint8_t  rx_buf[10];   /* 帧头之后的数据: [0]=type [1..8]=payload [9]=checksum */
static uint8_t  rx_idx;       /* 已接收字节数 (0..9) */
static uint8_t  rx_locked;    /* 0=搜索帧头  1=收包中 */

static JY61P_Data g_data;     /* 最新解析数据 */
static uint8_t    g_flags;    /* 就绪标志位 */

/* ================================================================
 * 辅助
 * ================================================================ */

static int16_t to_i16(uint8_t lo, uint8_t hi)
{
    return (int16_t)(((uint16_t)hi << 8) | lo);
}

static float to_unit(int16_t raw, float range)
{
    return (float)raw / JY61P_RESOLUTION * range;
}

/* ================================================================
 * 状态机 — 逐字节解析 11 字节数据包
 *
 * 包结构:
 *   [0] 0x55          帧头
 *   [1] Type          0x51/0x52/0x53/0x54
 *   [2..9] Payload    4 个 int16 (小端), 前 3 个为传感器值, 第 4 个为温度
 *   [10] Checksum     (0x55 + byte[1..9]) & 0xFF
 * ================================================================ */

void JY61P_FeedByte(uint8_t b)
{
    uint8_t type, i, sum;
    int16_t v1, v2, v3, temp;

    if (!rx_locked) {
        if (b == JY61P_HEADER) {
            rx_locked = 1;
            rx_idx = 0;
        }
        return;
    }

    rx_buf[rx_idx++] = b;

    if (rx_idx < 10) return;

    /* ---- 校验 ---- */
    rx_locked = 0;
    type = rx_buf[0];

    sum = JY61P_HEADER;
    for (i = 0; i < 9; i++) sum += rx_buf[i];
    if (sum != rx_buf[9]) return;

    /* ---- 解析 (小端: buf[1]=L, buf[2]=H, ...) ---- */
    v1   = to_i16(rx_buf[1], rx_buf[2]);
    v2   = to_i16(rx_buf[3], rx_buf[4]);
    v3   = to_i16(rx_buf[5], rx_buf[6]);
    temp = to_i16(rx_buf[7], rx_buf[8]);

    g_data.temp = temp / 100.0f;

    switch (type) {
    case JY61P_TYPE_ACCEL:
        g_data.ax = to_unit(v1, JY61P_ACC_SCALE);
        g_data.ay = to_unit(v2, JY61P_ACC_SCALE);
        g_data.az = to_unit(v3, JY61P_ACC_SCALE);
        g_flags |= JY61P_FLAG_ACCEL;
        break;

    case JY61P_TYPE_GYRO:
        g_data.gx = to_unit(v1, JY61P_GYRO_SCALE);
        g_data.gy = to_unit(v2, JY61P_GYRO_SCALE);
        g_data.gz = to_unit(v3, JY61P_GYRO_SCALE);
        g_flags |= JY61P_FLAG_GYRO;
        break;

    case JY61P_TYPE_ANGLE:
        g_data.roll  = to_unit(v1, JY61P_ANGLE_SCALE);
        g_data.pitch = to_unit(v2, JY61P_ANGLE_SCALE);
        g_data.yaw   = to_unit(v3, JY61P_ANGLE_SCALE);
        g_flags |= JY61P_FLAG_ANGLE;
        break;

    default:
        break;
    }
}

/* ================================================================
 * 数据获取
 * ================================================================ */

uint8_t JY61P_Ready(void)
{
    return g_flags;
}

/** 取出最新数据并清除标志 */
void JY61P_GetData(JY61P_Data *out)
{
    if (out) {
        *out = g_data;
        g_flags = 0;
    }
}

/** 仅取出姿态角 */
uint8_t JY61P_GetAngle(float *roll, float *pitch, float *yaw)
{
    if (!(g_flags & JY61P_FLAG_ANGLE)) return 0;

    if (roll)  *roll  = g_data.roll;
    if (pitch) *pitch = g_data.pitch;
    if (yaw)   *yaw   = g_data.yaw;
    g_flags &= (uint8_t)(~JY61P_FLAG_ANGLE);
    return 1;
}

/* ================================================================
 * UART 接收启动
 * ================================================================ */

/** 启动 1 字节中断接收, 收到后硬件触发 HAL_UART_RxCpltCallback */
void JY61P_StartRecv(UART_HandleTypeDef *huart)
{
    static uint8_t ch;
    HAL_UART_Receive_IT(huart, &ch, 1);
}

/* ================================================================
 * 配置指令
 *
 * 指令格式: 0xFF 0xAA <CMD>
 *
 * 寄存器写入格式: 0xFF 0xAA <REG> <VAL_L> <VAL_H>
 * 写入前可能需要解锁: 向 0x69 写入 0xB588
 * ================================================================ */

/** 发送 3 字节指令 */
static void send_cmd(UART_HandleTypeDef *huart, uint8_t cmd)
{
    uint8_t buf[3] = {0xFF, 0xAA, cmd};
    HAL_UART_Transmit(huart, buf, 3, 100);
}

/** 发送 5 字节寄存器写入: 0xFF 0xAA <REG> <VAL_L> <VAL_H> */
static void write_reg(UART_HandleTypeDef *huart, uint8_t reg, uint16_t val)
{
    uint8_t buf[5];
    buf[0] = 0xFF;
    buf[1] = 0xAA;
    buf[2] = reg;
    buf[3] = (uint8_t)(val & 0xFF);         /* 低字节在前 */
    buf[4] = (uint8_t)((val >> 8) & 0xFF);  /* 高字节在后 */
    HAL_UART_Transmit(huart, buf, 5, 100);
}

/** 解锁并写入寄存器, 自动保存 */
static void write_reg_safe(UART_HandleTypeDef *huart, uint8_t reg, uint16_t val)
{
    write_reg(huart, 0x69, 0x88B5);   /* 解锁 */
    HAL_Delay(100);
    write_reg(huart, reg, val);        /* 写值 */
    HAL_Delay(100);
    write_reg(huart, 0x00, 0x0000);   /* 保存 */
    HAL_Delay(100);
}

/** Z 轴归零 */
void JY61P_ZeroYaw(UART_HandleTypeDef *huart)
{
    send_cmd(huart, 0x52);
}

/** 加速度计校准 */
void JY61P_Calibrate(UART_HandleTypeDef *huart)
{
    send_cmd(huart, 0x67);
}

/* ================================================================
 * USART3 手动初始化 (CubeMX 已生成 MX_USART3_UART_Init 则跳过)
 * ================================================================ */

void JY61P_UART_Init(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_USART3_CLK_ENABLE();

    /** PB10 = TX (复用推挽) */
    gpio.Pin   = GPIO_PIN_10;
    gpio.Mode  = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);

    /** PB11 = RX (浮空输入) */
    gpio.Pin  = GPIO_PIN_11;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &gpio);

    /** USART3 115200-8-N-1 */
    huart->Instance          = USART3;
    huart->Init.BaudRate     = 115200;
    huart->Init.WordLength   = UART_WORDLENGTH_8B;
    huart->Init.StopBits     = UART_STOPBITS_1;
    huart->Init.Parity       = UART_PARITY_NONE;
    huart->Init.Mode         = UART_MODE_TX_RX;
    huart->Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart->Init.OverSampling = UART_OVERSAMPLING_16;

    HAL_UART_Init(huart);
}
