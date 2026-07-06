# 629 工程整改代码导入说明

## 适用工程

| 项目 | 内容 |
|---|---|
| 原工程路径 | `D:\STM32CubeIDE\workspace_1.19.0\smart_logistics_car_integrated_route_2026-06-17` |
| 导入方式 | 将本目录内同相对路径文件复制/覆盖到 CubeIDE 工程对应位置 |
| 原工程文件 | 本次没有直接修改原工程，请先在 CubeIDE 或文件管理器中备份原文件 |
| 目标 | RS485 电机只发不收；固定路线保留时间开环；仅用 IMU yaw 对直线前进/后退轻量纠偏 |

## 文件清单

| 本包文件 | 导入到工程位置 | 作用 | 是否新增路径 |
|---|---|---|---|
| `Core/Inc/config.h` | `Core/Inc/config.h` | 增加 `IMU_ASSIST_ENABLE`、`ROUTE_STRAIGHT_YAW_ASSIST_ENABLE` 两个总开关 | 否 |
| `Core/Inc/mecanum_config.h` | `Core/Inc/mecanum_config.h` | 调整直线 yaw 纠偏参数，增加 IMU 超时和最大 yaw 偏差阈值 | 否 |
| `Core/Inc/straight_control.h` | `Core/Inc/straight_control.h` | 状态结构增加 IMU 年龄和故障标志 | 否 |
| `Core/Src/straight_control.c` | `Core/Src/straight_control.c` | 只使用 IMU yaw 纠偏，移除电机反馈 RPM 依赖 | 否 |
| `Core/Src/sensor_bridge.c` | `Core/Src/sensor_bridge.c` | IMU 辅助模式下直接从 `imu_app` 取 yaw，不拉起激光/完整 `Board_App` | 否 |
| `Core/Src/main.c` | `Core/Src/main.c` | 固定路线模式下单独初始化 I2C1 和 IMU，并在循环中调用 `IMU_App_Loop()` | 否 |
| `Core/Src/chassis_control.c` | `Core/Src/chassis_control.c` | 移除周期性电机反馈更新调用，仅保留速度下发 | 否 |
| `Core/Inc/motor_manager.h` | `Core/Inc/motor_manager.h` | 移除电机反馈相关接口声明 | 否 |
| `Core/Src/motor_manager.c` | `Core/Src/motor_manager.c` | 移除空反馈更新函数和状态读取接口 | 否 |
| `Core/Inc/zdt_x42_modbus.h` | `Core/Inc/zdt_x42_modbus.h` | 移除 ZDT 速度读取接口声明 | 否 |
| `Core/Src/zdt_x42_modbus.c` | `Core/Src/zdt_x42_modbus.c` | 移除 ZDT 速度读取实现 | 否 |
| `Core/Inc/modbus_rtu.h` | `Core/Inc/modbus_rtu.h` | 移除 Modbus 读输入寄存器接口声明 | 否 |
| `Core/Src/modbus_rtu.c` | `Core/Src/modbus_rtu.c` | 移除 Modbus 读输入寄存器实现，保留写寄存器/发送路径 | 否 |
| `Core/Inc/mecanum.h` | `Core/Inc/mecanum.h` | 移除 RS485 读速度测试接口声明 | 否 |
| `Core/Src/mecanum.c` | `Core/Src/mecanum.c` | 移除 RS485 读速度测试接口实现 | 否 |
| `UserModules/competition_route/competition_route.c` | `UserModules/competition_route/competition_route.c` | 按 629 指南更新比赛路线，并在固定路线启动时打开直线 yaw 辅助 | 否 |
| `UserModules/competition_route/competition_route.h` | `UserModules/competition_route/competition_route.h` | 更新固定路线纠偏说明 | 否 |

## CubeMX / IOC 配置检查

| 配置项 | 当前工程状态 | 是否需要修改 |
|---|---|---|
| I2C1 | 已配置，PB6=SCL，PB7=SDA，100 kHz | 不需要 |
| USART1 | 已配置，PA9/PA10，用作 RS485/ZDT 电机总线 | 不需要 |
| USART2 | 已配置，固定路线独立模式不启用树莓派链路 | 不需要 |
| GPIO | 无新增 RS485 方向脚，`MODBUS_USE_RS485_DIR_GPIO=0` | 不需要 |
| Include Path | `Core/Inc` 和 `UserModules/competition_route` 已在原工程可用 | 不需要新增 |

注意：不要用 CubeMX 重新生成代码后直接覆盖本次文件；如必须重新生成，请再导入本包对应文件。

## 关键开关

| 宏 | 推荐值 | 说明 |
|---|---:|---|
| `FIXED_ROUTE_STANDALONE_ENABLE` | `1U` | 保持固定路线独立运行，不依赖树莓派/激光 |
| `IMU_ASSIST_ENABLE` | `1U` | 仅启用 I2C1 + JY61P IMU，不启动完整 `Board_App` |
| `ROUTE_STRAIGHT_YAW_ASSIST_ENABLE` | `1U` | 固定路线直线前进/后退段启用 yaw 纠偏 |
| `CSV_DEBUG_ENABLE` | `0U` | 保持关闭，避免占用调试串口 |

## 新路线表

| 顺序 | 动作 | 数值 | 说明 |
|---:|---|---:|---|
| 1 | 左移 | 180 mm | `+vx` |
| 2 | 前行 | 3400 mm | 直线 yaw 辅助可生效 |
| 3 | 逆时针转 | 90 deg | 时间开环旋转 |
| 4 | 前行 | 800 mm | 直线 yaw 辅助可生效 |
| 5 | 停止 | 60000 ms | 夹球 |
| 6 | 逆时针转 | 90 deg | 时间开环旋转 |
| 7 | 前行 | 1000 mm | 直线 yaw 辅助可生效 |
| 8 | 停止 | 30000 ms | 扫码 |
| 9 | 右移 | 220 mm | `-vx` |
| 10 | 前行 | 1350 mm | 直线 yaw 辅助可生效 |
| 11 | 左移 | 220 mm | `+vx` |
| 12 | 前行 | 500 mm | 直线 yaw 辅助可生效 |
| 13 | 右移 | 220 mm | `-vx` |

## 启动与回退逻辑

| 场景 | 行为 |
|---|---|
| 上电后 IMU 正常并完成校准 | 固定路线启动前执行一次 yaw 归零，直线段保持进入该段时的 yaw |
| IMU 未找到、未校准或超时 | 固定路线继续按原时间开环行驶，不叠加 `wz` |
| 直线段 yaw 偏差超过约 25 deg | 当前直线段放弃纠偏，避免异常数据强行拉偏 |
| 横移/旋转/停止段 | 自动重置直线纠偏状态，不做 yaw 修正 |
| RS485 电机总线 | 只发送使能/速度/停止命令，不读取电机状态 |

## 导入步骤

|  步骤 | 操作                                                                        |
| --: | ------------------------------------------------------------------------- |
|   1 | 关闭 CubeIDE 中正在运行的调试会话                                                     |
|   2 | 在工程目录中备份上表列出的原文件                                                          |
|   3 | 将 `stm32cubeide_import_629_fix` 目录内的 `Core`、`UserModules` 文件按相同相对路径复制到原工程 |
|   4 | 在 CubeIDE 中右键工程，选择 `Refresh`                                              |
|   5 | 执行 `Project > Clean`，然后重新 Build                                           |
|   6 | 上电后保持小车静止约 2~3 秒，等待 IMU 校准                                                |
|   7 | 先架空车轮或低速空载测试四轮方向，再进行实地路线测试                                                |

## 实车调参建议

| 参数 | 位置 | 初始值 | 调整建议 |
|---|---|---:|---|
| `STRAIGHT_CONTROL_MAX_WZ_RADPS` | `Core/Inc/mecanum_config.h` | `0.15f` | 摆动明显就减小，纠偏不足再增大 |
| `STRAIGHT_CONTROL_YAW_DEADBAND_RAD` | `Core/Inc/mecanum_config.h` | `0.01745f` | 约 1 deg，可放宽到 2 deg |
| `STRAIGHT_YAW_KP` | `Core/Inc/mecanum_config.h` | `1.00f` | 蛇形摆动减小，修正慢增大 |
| `STRAIGHT_YAW_KD` | `Core/Inc/mecanum_config.h` | `0.01f` | 抑制摆动用，谨慎增大 |
| `STRAIGHT_IMU_TIMEOUT_MS` | `Core/Inc/mecanum_config.h` | `300u` | I2C 偶发慢可增大到 500 ms |

