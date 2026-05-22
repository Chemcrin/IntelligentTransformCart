#ifndef __JY61P_H__
#define __JY61P_H__

#include "stm32f1xx_hal.h"

/**
 * JY61P 六轴姿态传感器 — UART 驱动
 *
 * 硬件连接 (STM32F103C8T6):
 *   PB10 (USART3_TX)  →  JY61P-RX
 *   PB11 (USART3_RX)  →  JY61P-TX
 *   PB6  (I2C1_SCL)   →  JY61P-SCL (本驱动未使用)
 *   PB7  (I2C1_SDA)   →  JY61P-SDA (本驱动未使用)
 *
 * UART 参数: 115200-8-N-1
 * 数据输出速率: 默认 100Hz, 可配置 0.2~200Hz
 */

/* ---- 数据包定义 ---- */
#define JY61P_HEADER        0x55    /* 帧头 */
#define JY61P_TYPE_ACCEL    0x51    /* 加速度包 */
#define JY61P_TYPE_GYRO     0x52    /* 角速度包 */
#define JY61P_TYPE_ANGLE    0x53    /* 姿态角包 */
#define JY61P_TYPE_MAG      0x54    /* 磁场包 */
#define JY61P_PACKET_SIZE   11      /* 每帧 11 字节 */

/* ---- 量程与分辨率 ---- */
#define JY61P_ACC_SCALE     16.0f
#define JY61P_GYRO_SCALE    2000.0f
#define JY61P_ANGLE_SCALE   180.0f
#define JY61P_RESOLUTION    32768.0f

/* ---- 数据就绪标志位 ---- */
#define JY61P_FLAG_ACCEL    0x01
#define JY61P_FLAG_GYRO     0x02
#define JY61P_FLAG_ANGLE    0x04

/* ---- 传感器数据结构 ---- */
typedef struct {
    float ax, ay, az;       /* 加速度 (g)      */
    float gx, gy, gz;       /* 角速度 (°/s)    */
    float roll, pitch, yaw; /* 姿态角 (度)     */
    float temp;             /* 温度 (°C)       */
} JY61P_Data;

/* ================================================================
 * 初始化与接收
 * ================================================================ */

/** 初始化 USART3: PB10=TX, PB11=RX, 115200-8-N-1 */
void JY61P_UART_Init(UART_HandleTypeDef *huart);

/** 启动单字节中断接收 (main 初始化后调用一次) */
void JY61P_StartRecv(UART_HandleTypeDef *huart);

/**
 * 字节解析 — 在 HAL_UART_RxCpltCallback 中调用:
 *
 *   void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
 *       if (huart->Instance == USART3) {
 *           static uint8_t ch;
 *           HAL_UART_Receive_IT(&huart3, &ch, 1);  // 先重新启动
 *           JY61P_FeedByte(ch);                     // 再解析
 *       }
 *   }
 */
void JY61P_FeedByte(uint8_t byte);

/* ================================================================
 * 数据获取
 * ================================================================ */

/** 返回就绪标志位 (JY61P_FLAG_ACCEL | GYRO | ANGLE) */
uint8_t JY61P_Ready(void);

/** 取出最新数据并清除就绪标志 */
void JY61P_GetData(JY61P_Data *data);

/** 仅取出姿态角, 有新数据返回 1 */
uint8_t JY61P_GetAngle(float *roll, float *pitch, float *yaw);

/** 令 Z 轴归零 (以当前朝向为 0°) */
void JY61P_ZeroYaw(UART_HandleTypeDef *huart);

/** 加速度计校准 (模块水平静置) */
void JY61P_Calibrate(UART_HandleTypeDef *huart);

#endif
