# CubeIDE 导入与 IOC 配置

## 新建工程

| 项目 | 配置 |
|---|---|
| IDE | STM32CubeIDE |
| 工程类型 | STM32 Project |
| MCU | 选择实际控制板芯片；当前代码头文件按 STM32F1 HAL 编写 |
| 代码生成 | Generate peripheral initialization as pair of `.c/.h` files 建议开启 |

## IOC 外设配置

| 外设 | 配置项 | 推荐值 |
|---|---|---|
| I2C1 | Mode | I2C |
| I2C1 | SCL/SDA | PB6/PB7 |
| I2C1 | Speed | 100 kHz 起步，稳定后可试 400 kHz |
| I2C1 | 用途 | JY61P 地址 `0x50`，ATK-MS53L1M 地址 `0x29` |
| USART3 | Mode | Asynchronous |
| USART3 | TX/RX | PB10/PB11 |
| USART3 | Baud | 115200 |
| USART3 | Word/Parity/Stop | 8 bit / None / 1 stop |
| GPIO Output | Name | `RS485_DIR` |
| GPIO Output | Pin | 默认 PB1，可按硬件修改 |
| GPIO Output | Level | 初始低电平，表示接收 |
| SYS | Debug | Serial Wire |

## main.h 宏要求

CubeMX 给 RS485 方向脚命名为 `RS485_DIR` 后，`main.h` 中应自动生成类似宏：

```c
#define RS485_DIR_Pin GPIO_PIN_1
#define RS485_DIR_GPIO_Port GPIOB
```

若硬件不是 PB1，请在 CubeMX 中修改引脚，或在 `Core/Inc/mecanum_config.h` 中修改默认宏。

## 需要加入的 Include Paths

| 路径 |
|---|
| `Core/Inc` |
| `Drivers/VL53L1X/core/inc` |
| `Drivers/VL53L1X/platform/inc` |

## 需要参与编译的源码

| 路径 |
|---|
| `Core/Src/*.c` |
| `Drivers/VL53L1X/core/src/*.c` |
| `Drivers/VL53L1X/platform/src/*.c` |

## main.c 合并方式

将 `Templates/main_integrated_example.c` 的用户代码合并到 CubeMX 生成的 `Core/Src/main.c`：

| 区域 | 操作 |
|---|---|
| Includes | 增加 `board_app.h`、`sensor_bridge.h`、`mecanum.h`、`chassis_control.h` |
| Init | 在 `MX_GPIO_Init()`、`MX_I2C1_Init()`、`MX_USART3_UART_Init()` 后调用 `Board_App_Init()` 和 `Mecanum_Init(&huart3)` |
| while(1) | 按示例顺序调用 `Board_App_Loop()`、`Sensor_GetSnapshot()`、安全判断、`Mecanum_PeriodicTask()` |

## 烧录前检查表

| 检查项 | 标准 |
|---|---|
| I2C | PB6/PB7 上拉正常，JY61P 与 ATK-MS53L1M 地址不冲突 |
| RS485 | A/B 未接反，方向脚电平与模块 DE/RE 逻辑一致 |
| 电机地址 | FL/FR/RL/RR 分别为 `0x01/0x02/0x03/0x04` |
| 车轮 | 首次测试必须架空 |
| 急停 | 激光遮挡到 150 mm 内时底盘停止 |
