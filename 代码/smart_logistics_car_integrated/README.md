# 智能物流搬运小车整合工程

本目录将传感器模块和麦克纳姆轮运动模块整合为一套 STM32CubeIDE 可导入的 `Core/Drivers` 代码包。建议先在 STM32CubeIDE/CubeMX 中按本文 IOC 配置生成目标芯片工程，再将本目录文件复制覆盖到工程根目录。

## 工程自查结论

| 项目 | 结论 |
|---|---|
| 输入资料完整性 | 两个 zip 均可读取，模块代码完整 |
| CubeIDE 完整度 | 两个 zip 未包含 `.ioc`、`.project`、启动文件、链接脚本和 HAL 基础工程 |
| 整合策略 | 以 CubeMX 生成的空工程为底座，导入本目录 `Core` 与 `Drivers` |
| 接口差异 | 桥接文档中的 `Board_App_GetSnapshot()` 在实际代码中为 `Board_App_GetSensorSnapshot()`，已按实际代码适配 |
| 主要风险 | 需要按实车确认目标芯片、USART3 引脚、RS485 A/B 接线和四轮电机方向 |

## 目录内容

| 路径 | 说明 |
|---|---|
| `Core/Inc/atk_ms53l1m.h`、`Core/Src/atk_ms53l1m.c` | ATK-MS53L1M/VL53L1X 激光测距应用驱动 |
| `Core/Inc/jy61p.h`、`Core/Src/jy61p.c` | JY61P 姿态模块 I2C 驱动 |
| `Core/Inc/imu_app.h`、`Core/Src/imu_app.c` | IMU 应用层与车体坐标转换 |
| `Core/Inc/laser_app.h`、`Core/Src/laser_app.c` | 激光测距应用层、滤波与安全距离判断 |
| `Core/Inc/sensors.h`、`Core/Src/sensors.c` | 传感器模块统一快照 |
| `Core/Inc/board_app.h`、`Core/Src/board_app.c` | 板级传感器调度入口 |
| `Core/Inc/sensor_bridge.h`、`Core/Src/sensor_bridge.c` | 传感器桥接层，输出 rad/m 单位快照 |
| `Core/Inc/mecanum*.h`、`Core/Src/mecanum*.c` | 麦克纳姆轮运动学与底盘接口 |
| `Core/Inc/modbus_rtu.h`、`Core/Src/modbus_rtu.c` | RS485 Modbus RTU 通信层 |
| `Core/Inc/zdt_x42_modbus.h`、`Core/Src/zdt_x42_modbus.c` | ZDT-X42 电机 Modbus 指令封装 |
| `Core/Inc/motor_manager.h`、`Core/Src/motor_manager.c` | 四轮电机管理与反馈 |
| `Templates/main_integrated_example.c` | 可合并到 `Core/Src/main.c` 的集成主循环模板 |
| `Drivers/VL53L1X` | VL53L1X 官方 ULD 移植层与核心驱动 |
| `Docs/工程整合过程说明.md` | 本次整合过程和后续模块接入说明 |
| `Docs/CubeIDE导入与IOC配置.md` | CubeIDE 导入、编译路径和 IOC 引脚配置说明 |

## CubeIDE 导入方式

| 步骤 | 操作 |
|---|---|
| 1 | 在 STM32CubeIDE 中新建目标芯片工程，建议选择实际控制板对应 STM32F1 系列芯片 |
| 2 | 在 `.ioc` 中按 `Docs/CubeIDE导入与IOC配置.md` 配置 I2C1 和 USART3；默认不需要配置 `RS485_DIR` GPIO |
| 3 | 生成代码后，将本目录的 `Core/Inc`、`Core/Src`、`Drivers/VL53L1X` 复制到 CubeIDE 工程根目录 |
| 4 | 将 `Templates/main_integrated_example.c` 内容合并到 CubeMX 生成的 `Core/Src/main.c` 用户代码区 |
| 5 | 在工程 Include Paths 中加入 `Drivers/VL53L1X/core/inc` 与 `Drivers/VL53L1X/platform/inc` |
| 6 | 确保 `Drivers/VL53L1X/core/src/*.c` 和 `Drivers/VL53L1X/platform/src/*.c` 均参与编译 |
| 7 | 编译、下载前先架空车轮，确认四轮方向和 RS485 A/B 接线 |

## 默认外设

| 外设 | 默认配置 | 用途 |
|---|---|---|
| I2C1 | PB6=SCL、PB7=SDA、100 kHz 或 400 kHz | JY61P 与 ATK-MS53L1M 共用 |
| USART3 | PB10=TX、PB11=RX、115200 8N1 | RS485 Modbus 控制 ZDT-X42 电机 |
| RS485_DIR GPIO | 默认不使用 | 官方图示的 RS485 通讯模块不需要额外方向脚 |

## 首次烧录建议

| 检查项 | 建议 |
|---|---|
| 车轮 | 架空测试，避免方向错误导致冲撞 |
| 电机地址 | 四个 ZDT-X42 地址设为 `0x01`、`0x02`、`0x03`、`0x04` |
| 激光安全 | `MS53L1M_STOP_DISTANCE_MM` 默认 150 mm，过近会触发外部急停 |
| 方向校准 | 若前进、横移或旋转方向不对，优先调整 `mecanum_config.h` 和 `MotorManager_SetDirectionSigns()` |
