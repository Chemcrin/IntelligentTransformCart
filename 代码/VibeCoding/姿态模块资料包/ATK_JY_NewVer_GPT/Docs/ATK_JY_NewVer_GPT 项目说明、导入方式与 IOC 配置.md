# ATK_JY_NewVer_GPT 项目说明、导入方式与 IOC 配置

## 1. 工程自查结论

本次代码按以下已确认条件实现：

- 主控按 STM32F103C8T6 / STM32C8T6 类资源设计。
- 开发环境按 STM32CubeIDE + HAL 库 + C 语言。
- JY61P 与 ATK-MS53L1M / VL53L1X 共用 I2C1。
- I2C1 引脚：PB6 = SCL，PB7 = SDA。
- JY61P 7-bit 地址：`0x50`，HAL 地址：`0xA0`。
- VL53L1X 7-bit 地址：`0x29`，HAL 地址：`0x52`。
- JY61P 安装方向按 `+Y` 朝车前、`+X` 朝车右处理。
- UART1 电机接口 PA9/PA10 只保留回调入口，不在传感器层发送电机命令。

需要实车验证的部分：

- JY61P 的 `car_pitch_deg`、`car_roll_deg`、`car_yaw_est_deg` 符号方向。如果方向相反，只修改 `jy61p.h` 中的 `JY61P_SIGN_*` 宏。
- VL53L1X 如果是裸芯片直连，部分模块需要官方 ST ULD 初始化序列。当前代码已经提供弱函数平台层，默认使用轻量寄存器启动；若实测无法出距离，请按 `Templates/atk_ms53l1m_st_uld_port_template.c` 覆盖弱函数。

## 2. 本项目包含的代码文件

```text
ATK_JY_NewVer_GPT/
  Core/
    Inc/
      jy61p.h              JY61P 底层驱动接口、数据结构、宏参数
      atk_ms53l1m.h        ATK-MS53L1M / VL53L1X 底层驱动接口、滤波参数、弱平台函数声明
      imu_app.h            IMU 应用层接口，对外暴露 g_imu_* 和兼容变量名
      laser_app.h          激光测距应用层接口，对外暴露 g_laser_*
      board_app.h          板级统一入口、SENSOR_Snapshot_t 快照接口
    Src/
      jy61p.c              JY61P I2C 连续读取、解析、校准、滤波、车体系映射
      atk_ms53l1m.c        VL53L1X 测距读取、滤波、异常剔除、近距离优先、弱平台层
      imu_app.c            IMU 应用层周期更新与全局变量同步
      laser_app.c          激光应用层周期更新与全局变量同步
      board_app.c          Board_App_Init / Board_App_Loop / 快照 / I2C 总线恢复
  Docs/
    项目说明_导入与IOC配置.md
    main_c_USER_CODE调用示例.md
  Templates/
    atk_ms53l1m_st_uld_port_template.c
```

## 3. 对外可直接读取的变量

### 3.1 IMU 全局变量

```c
extern volatile float g_imu_car_yaw_est_deg;
extern volatile float g_imu_car_pitch_deg;
extern volatile float g_imu_car_roll_deg;
extern volatile float g_imu_gyro_yaw_dps;
extern volatile float g_imu_acc_forward_g;
extern volatile float g_imu_acc_right_g;
extern volatile float g_imu_acc_down_g;
extern volatile float g_imu_temperature_c;
extern volatile uint8_t g_imu_valid;
extern volatile uint8_t g_imu_calibrated;
```

同时保留了架构文档中的兼容变量名：

```c
extern volatile float car_yaw_est_deg;
extern volatile float car_pitch_deg;
extern volatile float car_roll_deg;
extern volatile float gyro_yaw_dps;
extern volatile float acc_forward_g;
extern volatile float acc_right_g;
extern volatile float acc_down_g;
extern volatile float temperature_c;
extern volatile uint8_t imu_valid;
extern volatile uint8_t imu_calibrated;
```

### 3.2 激光测距全局变量

```c
extern volatile uint16_t g_laser_raw_distance_mm;
extern volatile uint16_t g_laser_filtered_distance_mm;
extern volatile uint16_t g_laser_distance_mm;
extern volatile uint8_t g_laser_valid;
extern volatile uint8_t g_laser_ok;
extern volatile uint8_t g_laser_is_warning;
extern volatile uint8_t g_laser_is_slow;
extern volatile uint8_t g_laser_is_too_close;
extern volatile uint32_t g_laser_last_valid_ms;
```

### 3.3 统一快照

```c
extern volatile SENSOR_Snapshot_t g_sensor_snapshot;
```

电机控制层建议优先使用：

```c
SENSOR_Snapshot_t snap;
Board_App_GetSnapshot(&snap);
```

## 4. STM32CubeIDE 导入方式

### 方式 A：复制到已有 CubeIDE 工程

1. 打开已有 STM32CubeIDE 工程。
2. 将本文件夹中的 `Core/Inc/*.h` 复制到你工程的 `Core/Inc/`。
3. 将本文件夹中的 `Core/Src/*.c` 复制到你工程的 `Core/Src/`。
4. 刷新工程：右键工程名 -> `Refresh`。
5. 确认 `Core/Src` 中新增 `.c` 文件已经参与编译。一般复制到 `Core/Src` 后会自动参与。
6. 在 `main.c` 的 USER CODE 区域加入 `board_app.h` 以及初始化、循环调用，具体见 `Docs/main_c_USER_CODE调用示例.md`。

### 方式 B：新建 CubeIDE 工程后复制

1. `File -> New -> STM32 Project`。
2. MCU 选择 `STM32F103C8Tx`。
3. 生成 HAL 工程。
4. 按下面的 IOC 配置 I2C1、USART1、GPIO。
5. 生成代码后，将本项目 `Core/Inc` 与 `Core/Src` 的文件复制到新工程同名目录。
6. 修改 `main.c` 的 USER CODE 区域。

## 5. IOC / CubeMX 配置方式

### 5.1 I2C1

| 项目 | 配置 |
|---|---|
| Peripheral | I2C1 |
| SCL | PB6 |
| SDA | PB7 |
| Mode | I2C Master |
| Addressing Mode | 7-bit |
| Speed | 100 kHz 起步，稳定后可评估 400 kHz |
| DMA | 不启用 |
| Interrupt | 不启用 |
| PB6/PB7 模式 | Alternate Function Open Drain |
| GPIO Pull | 建议 No Pull，使用外部上拉 |

注意：硬件上拉到 5V 时，PB6/PB7 必须保持开漏或输入，不得配置为推挽高电平输出。本代码的 I2C 恢复函数使用开漏输出，写高电平只表示释放总线。

### 5.2 USART1

| 项目 | 配置 |
|---|---|
| Peripheral | USART1 |
| TX | PA9 |
| RX | PA10 |
| Mode | Asynchronous |
| Baudrate | 按电机模块协议设置 |
| DMA/Interrupt | 由电机模块决定，传感器层不强制 |

### 5.3 SysTick

保持 HAL 默认 `HAL_GetTick()` 可用。传感器调度依赖毫秒时间戳。

## 6. main.c 调用方式

在 `main.c` 中：

1. `USER CODE BEGIN Includes` 加入：

```c
#include "board_app.h"
```

2. 在 `MX_GPIO_Init()`、`MX_I2C1_Init()`、`MX_USART1_UART_Init()` 之后加入：

```c
Board_App_Init();
```

3. 在主循环 `while (1)` 中加入：

```c
Board_App_Loop();
```

4. 电机控制层读取快照示例：

```c
SENSOR_Snapshot_t snap;
Board_App_GetSnapshot(&snap);

if (snap.laser_is_too_close)
{
    /* 这里由电机模块通过 UART1 请求停车 */
}

if (snap.imu_valid && snap.imu_calibrated)
{
    /* 可使用 snap.car_yaw_est_deg 做直线纠偏 */
}
```

## 7. 上电与调试建议

1. 上电后让小车静止约 2 秒，JY61P 会分步采集 200 个样本完成陀螺仪零偏校准。
2. 校准完成后 `g_imu_calibrated = 1`，`g_imu_valid = 1`。
3. 如果抬高车头时 `g_imu_car_pitch_deg` 方向相反，修改 `JY61P_SIGN_CAR_PITCH`。
4. 如果左右侧倾时 `g_imu_car_roll_deg` 方向相反，修改 `JY61P_SIGN_CAR_ROLL`。
5. 如果水平旋转时 yaw 方向相反，修改 `JY61P_SIGN_CAR_YAW`。
6. 激光距离低于 600 mm 时 `g_laser_is_warning = 1`。
7. 激光距离低于 300 mm 时 `g_laser_is_slow = 1`。
8. 激光距离低于 150 mm 且连续确认时 `g_laser_is_too_close = 1`。

## 8. VL53L1X 底层说明

`atk_ms53l1m.c` 中默认提供了轻量 VL53L1X 寄存器启动和读取逻辑：

- 检测 `0x29` 地址是否应答。
- 等待固件 boot ready。
- 启动 autonomous ranging。
- 查询 data ready。
- 读取距离寄存器。
- 清除中断。

如果你的 VL53L1X 模块不是预配置模块，而是严格裸芯片直连，可能必须使用官方 ST VL53L1X ULD 完整初始化。此时不需要改上层应用代码，只需用一个新的 `.c` 文件覆盖以下弱函数：

```c
HAL_StatusTypeDef ATK_MS53L1M_PlatformStartRanging(ATK_MS53L1M_Device_t *dev);
HAL_StatusTypeDef ATK_MS53L1M_PlatformIsDataReady(ATK_MS53L1M_Device_t *dev, uint8_t *ready);
HAL_StatusTypeDef ATK_MS53L1M_PlatformReadDistance(ATK_MS53L1M_Device_t *dev, uint16_t *distance_mm);
HAL_StatusTypeDef ATK_MS53L1M_PlatformClearInterrupt(ATK_MS53L1M_Device_t *dev);
```

参考模板见：`Templates/atk_ms53l1m_st_uld_port_template.c`。

## 9. 验收测试步骤

1. 只接 JY61P，确认 I2C 地址 `0x50` 应答，静止后 `g_imu_valid = 1`。
2. 只接 VL53L1X，确认 I2C 地址 `0x29` 应答，障碍物移动时 `g_laser_filtered_distance_mm` 变化。
3. 两个传感器同时接入，确认主循环不卡顿，`g_imu_*` 与 `g_laser_*` 均持续更新。
4. 运行中断开某个传感器，确认对应 `valid/ok` 变为 0，系统不死循环。
5. 静止运行 30 分钟，观察错误计数不持续快速增加。
6. 小车低速直线运行，确认 yaw 连续合理，激光阈值能触发预警、减速、停车标志。
