# UART3 传感器数据回传测试说明

本测试文件仅用于临时验证 JY61P 陀螺仪和 ATK-MS53L1M 激光测距数据回传，不用于小车正式运行。测试时 `USART3` 改为直连树莓派串口，`PB10` 为 STM32 TX，`PB11` 为 STM32 RX，串口参数为 `115200 8N1`。

## 新增文件

| 文件 | 用途 |
|---|---|
| `UserModules/sensor_uart3_stream_test/sensor_uart3_stream_test.h` | 测试串口回传模块头文件 |
| `UserModules/sensor_uart3_stream_test/sensor_uart3_stream_test.c` | 将 IMU 和激光数据打包成 CSV，并通过 `huart3` 发送 |
| `Templates/main_uart3_sensor_stream_test.c` | 测试版 main 模板，只跑传感器采集和 UART3 回传 |
| `Docs/UART3传感器数据回传测试说明.md` | 本说明文件 |

## 临时替换/新增到 CubeIDE 工程

| 操作 | 目标工程文件/位置 | 使用本目录中的文件 |
|---|---|---|
| 临时替换或合并 | `Core/Src/main.c` | `Templates/main_uart3_sensor_stream_test.c` |
| 临时新增源码 | `UserModules/sensor_uart3_stream_test/sensor_uart3_stream_test.c` | 同名文件 |
| 临时新增头文件 | `UserModules/sensor_uart3_stream_test/sensor_uart3_stream_test.h` | 同名文件 |
| 临时新增 Include Path | `UserModules/sensor_uart3_stream_test` | 让 `main.c` 能 include 测试头文件 |

如果 CubeMX 工程没有把 `gpio.c`、`i2c.c`、`usart.c` 独立生成，请不要直接整文件覆盖 `Core/Src/main.c`；保留 CubeMX 生成的 `SystemClock_Config()`、`MX_GPIO_Init()`、`MX_I2C1_Init()`、`MX_USART3_UART_Init()` 等函数，只替换 `include`、初始化顺序和 `while(1)` 用户代码区域。

## 测试版 main 行为

| 阶段 | 行为 |
|---|---|
| 初始化 | `HAL_Init()`、`SystemClock_Config()`、`MX_GPIO_Init()`、`MX_I2C1_Init()`、`MX_USART3_UART_Init()` |
| 传感器 | 调用 `Board_App_Init()` 初始化 JY61P 和 ATK-MS53L1M |
| 串口 | 调用 `SensorUart3Test_Init(&huart3)`，启动 CSV 表头输出 |
| 循环 | 每 5 ms 更新传感器，每 50 ms 通过 UART3 发出一帧数据 |
| 禁用项 | 不调用 `Mecanum_Init(&huart3)`，不运行电机 RS485，不运行正式遥控命令 |

## UART3 接线

| STM32 | 树莓派 | 说明 |
|---|---|---|
| PB10 / USART3_TX | RXD | STM32 数据输出到树莓派 |
| PB11 / USART3_RX | TXD | 本测试暂不解析树莓派命令，可接可不接 |
| GND | GND | 必须共地 |

测试时不要同时把 PB10/PB11 接到电机 RS485 模块。当前正式工程中 `USART3` 原本用于 ZDT-X42 电机 Modbus 总线，本测试临时占用该串口给树莓派。

## CSV 数据格式

启动后先输出一行标识和一行表头，之后持续输出 CSV。为了避免 STM32 工程开启浮点 printf，所有小数都按 1000 倍整数发送。

| 字段后缀 | 含义 | 树莓派端还原 |
|---|---|---|
| `_mdeg` | 毫度 | 除以 1000 得到 degree |
| `_mdps` | 毫度每秒 | 除以 1000 得到 degree/s |
| `_mg` | 毫 g | 除以 1000 得到 g |
| `_mc` | 毫摄氏度 | 除以 1000 得到 degC |
| `_mm` | 毫米 | 直接使用 |

树莓派快速查看：

```bash
stty -F /dev/serial0 115200 raw -echo
cat /dev/serial0
```

## 恢复正式运行

| 恢复项 | 操作 |
|---|---|
| `Core/Src/main.c` | 恢复正式版 main，重新调用 `Mecanum_Init(&huart3)`、`RemoteCmd_Init()` 等正式逻辑 |
| 测试模块 | 从编译列表移除 `sensor_uart3_stream_test.c`，或删除对应 Include Path |
| 接线 | 将 PB10/PB11 从树莓派串口恢复到正式 RS485/电机通信接线 |

