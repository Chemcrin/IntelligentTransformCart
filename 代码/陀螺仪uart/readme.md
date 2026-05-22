# JY61P 六轴姿态传感器 UART 驱动

STM32F103C8T6 + HAL 库, 通过串口读取 JY61P 陀螺仪数据。

## 硬件连接

| JY61P 引脚 | STM32 引脚  | 功能       |
|-----------|------------|-----------|
| VCC       | 3.3V       | 供电       |
| GND       | GND        | 地         |
| RX        | PB10       | USART3_TX |
| TX        | PB11       | USART3_RX |
| SCL       | PB6        | 本驱动未使用 |
| SDA       | PB7        | 本驱动未使用 |

## 快速开始

### 1. CubeMX 配置

- **USART3**: PB10=TX, PB11=RX, **115200-8-N-1**, NVIC 开启 USART3 全局中断
- 可选 USART1 (PA9=TX, 115200) 用于 printf 调试
- SysTick 必须开启

### 2. 添加文件

- `jy61p.h` → `Inc/`
- `jy61p.c` → `Src/`

### 3. 修改 main.c

```c
#include "jy61p.h"

extern UART_HandleTypeDef huart3;

/* 必须加入此回调 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3) {
        static uint8_t ch;
        HAL_UART_Receive_IT(huart, &ch, 1);
        JY61P_FeedByte(ch);
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART3_UART_Init();

    JY61P_StartRecv(&huart3);   /* 启动接收 */

    while (1) {
        float roll, pitch, yaw;
        if (JY61P_GetAngle(&roll, &pitch, &yaw)) {
            /* roll, pitch, yaw 为角度值(°) */
        }
        HAL_Delay(10);
    }
}
```

## UART 协议说明

模块上电后以 **100Hz** 速率主动输出数据包, 每包 **11 字节**:

| 偏移 | 内容 |
|------|------|
| 0    | 0x55 帧头 |
| 1    | 类型: 0x51=加速度, 0x52=角速度, 0x53=姿态角 |
| 2~9  | 4 个 int16 (小端): 3 轴数据 + 温度 |
| 10   | 校验和 (前 10 字节之和的低 8 位) |

转换公式: `实际值 = 原始值 / 32768 × 量程`

| 类型 | 量程 | 单位 |
|------|------|------|
| 加速度 | ±16 | g |
| 角速度 | ±2000 | °/s |
| 姿态角 | ±180 | ° |

## API 参考

### 初始化

| 函数 | 说明 |
|------|------|
| `JY61P_UART_Init(&huart)` | 手动初始化 USART3 (CubeMX 已生成则跳过) |
| `JY61P_StartRecv(&huart)` | 启动中断接收, 调用一次 |

### 数据读取

| 函数 | 说明 |
|------|------|
| `JY61P_Ready()` | 返回就绪标志位 |
| `JY61P_GetData(&data)` | 取出全部数据 |
| `JY61P_GetAngle(&r, &p, &y)` | 仅取姿态角, 返回 1=有新数据 |

### 配置

| 函数 | 说明 |
|------|------|
| `JY61P_ZeroYaw(&huart)` | Z 轴归零 |
| `JY61P_Calibrate(&huart)` | 加速度计校准 |

### 数据结构

```c
typedef struct {
    float ax, ay, az;       /* 加速度 (g)   */
    float gx, gy, gz;       /* 角速度 (°/s) */
    float roll, pitch, yaw; /* 姿态角 (°)   */
    float temp;             /* 温度 (°C)    */
} JY61P_Data;
```

## 注意事项

- **USART3 中断必须开启**, 否则 `HAL_UART_RxCpltCallback` 不会触发
- **Z 轴漂移**: 6 轴传感器无磁力计, Yaw 由角速度积分得出, 长时间运动后漂移; Roll/Pitch 有重力约束不会漂移
- **配置寄存器**如需精确控制回传速率等参数, 建议用 PC 端 WitMotion 上位机软件配置后保存到模块 Flash
- 若模块无输出, 检查波特率是否为 115200 (部分旧版固件默认 9600)
