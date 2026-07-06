# smart_logistics_car_integrated_route_2026-06-17 工程现状与问题梳理

整理时间：2026-06-29  
工程路径：`D:\STM32CubeIDE\workspace_1.19.0\smart_logistics_car_integrated_route_2026-06-17`  
检查方式：静态阅读 CubeMX 配置、主程序、底盘控制、固定路线、IMU、激光测距、Modbus/ZDT 电机、远程命令模块与 Debug 构建产物。

## 1. 总体结论

| 维度 | 当前结论 | 关键依据 |
|---|---|---|
| 当前固件形态 | 当前是“固定路线独立运行”版本，不依赖树莓派、不启用激光、不启用 IMU。 | `Core\Inc\config.h:5` 中 `FIXED_ROUTE_STANDALONE_ENABLE 1U`。 |
| 主循环实际运行内容 | 上电后初始化 GPIO、USART1、电机总线、比赛路线；主循环只跑 `CompetitionRoute_Loop()` 和 `Mecanum_PeriodicTask()`。 | `Core\Src\main.c:99-136`。 |
| 路线控制方式 | 路线是开环时间控制：平移按距离/速度换算时间，旋转按角度/角速度换算时间，不等陀螺仪航向。 | `UserModules\competition_route\competition_route.h:18-20`，`competition_route.c:66-107`。 |
| 陀螺仪航向校准 | 代码中有 JY61P 陀螺仪初始化、软件零偏校准、yaw 归零、硬件加速度校准接口；但当前固件不会调用 IMU 初始化和轮询，因此校准在当前运行版本不生效。 | `main.c` 中 `Board_App_Init/Loop` 被独立模式跳过；`imu_app.c:69-85` 只有被调用才会初始化/轮询。 |
| 激光测距避障 | 驱动代码存在，支持 VL53L1X/ATK-MS53L1M 测距、滤波和 500/300/100mm 告警；但当前固件不会初始化或轮询激光。 | `laser_app.c:40-67`，`atk_ms53l1m.h:17-23`，`config.h:5`。 |
| 步进电机通信 | 常规运行中 STM32 只向 ZDT_X42 发送使能/速度/停止 Modbus 写帧，不等待写应答；读取速度函数存在但运行期反馈函数为空。 | `zdt_x42_modbus.c:31,51` 传 `wait_response=0`；`motor_manager.c:86-89` 反馈函数空实现。 |
| 树莓派/上位机通信 | USART2 命令协议代码存在，但当前独立模式不初始化 USART2、RemoteCmd，也不启用命令接收中断回调。 | `main.c:103,115,131-133,179-186` 被 `FIXED_ROUTE_STANDALONE_ENABLE` 条件控制。 |
| 当前可实现功能 | 上电自动启动固定路线，输出四轮速度给四个 ZDT_X42 步进驱动器，完成前进、旋转、后退、横移、停止等动作。 | `competition_route.c:12-24`，`mecanum.c`，`chassis_control.c`。 |
| 当前主要风险 | 路线没有里程计、没有 IMU 闭环、没有激光保护、没有电机反馈、没有树莓派心跳保护；实际场地偏差会累积。 | 多处简化宏和空实现。 |

## 2. 工程与构建现状

| 项目 | 当前状态 |
|---|---|
| MCU | STM32F103C8T6，CubeMX 工程名 `smart_logistics_car_integrated_route_2026-06-17`。 |
| CubeMX 配置 | I2C1、USART1、USART2、SWD、SysTick。 |
| 工具链 | STM32 GNU Tools，Debug `makefile` 显示 `GNU Tools for STM32 (13.3.rel1)`。 |
| 已有构建产物 | `Debug` 下有 `.elf/.hex/.bin/.map/.list`，最新 `.elf/.map/.list` 时间为 2026-06-23 19:58:19，`.bin/.hex` 时间为 2026-06-23 17:43:13。 |
| 当前源码特点 | 有多个 `.simplify_bak` 文件，说明工程经历过“简化版本”改造；当前正式参与构建的目录包括 `Core/Src`、VL53L1X 驱动、`competition_route`、`remote_cmd`。 |
| 本次检查是否重新编译 | 未重新编译，只做静态检查和 Debug 产物状态检查。 |

## 3. 当前使用到的硬件与接口

| 硬件/模块 | 接口 | 当前固件实际状态 | 备注 |
|---|---|---|---|
| STM32F103C8T6 | 主控 | 使用中 | `.ioc` 明确 MCU 为 `STM32F103C8T6`。 |
| 四个 ZDT_X42 步进电机/驱动器 | USART1 PA9/PA10，115200，Modbus RTU/RS485 | 当前实际使用 | `Mecanum_Init(&huart1)`，四电机地址 1-4。 |
| RS485 方向控制 | 无 MCU 方向 GPIO | 当前代码不控制 DE/RE | `MODBUS_USE_RS485_DIR_GPIO 0u`，依赖外部自动收发模块或硬件默认方向。 |
| JY61P IMU/陀螺仪 | I2C1 PB6/PB7，地址 0x50 | 代码存在，当前固件不启用 | 独立模式下 `MX_I2C1_Init()` 和 `Board_App_Init()` 都跳过。 |
| ATK-MS53L1M / VL53L1X 激光测距 | I2C1 PB6/PB7，地址 0x29 | 代码存在，当前固件不启用 | 支持长距离模式和滤波告警，但不会轮询。 |
| 树莓派/上位机 | USART2 PA2/PA3，115200 | 代码存在，当前固件不启用 | 独立模式下不初始化 USART2 和 `RemoteCmd`。 |
| SWD 下载调试 | PA13/PA14 | 使用中 | CubeMX 配置为 Serial Wire。 |

## 4. 当前上电后的实际执行流程

| 顺序 | 代码动作 | 当前是否执行 | 说明 |
|---|---|---|---|
| 1 | `HAL_Init()`、`SystemClock_Config()` | 是 | 系统时钟使用 HSI，无 PLL，主频配置为 8MHz。 |
| 2 | `MX_GPIO_Init()` | 是 | 只打开 GPIOA/GPIOB 时钟。 |
| 3 | `MX_I2C1_Init()` | 否 | 被 `FIXED_ROUTE_STANDALONE_ENABLE == 0U` 包住。 |
| 4 | `MX_USART1_UART_Init()` | 是 | 电机 RS485/ZDT 总线。 |
| 5 | `MX_USART2_UART_Init()` | 否 | 独立模式跳过，树莓派通信不可用。 |
| 6 | `Board_App_Init()` | 否 | 因此 IMU 和激光都不初始化。 |
| 7 | `Mecanum_Init(&huart1)` | 是 | 初始化 Modbus、底盘、电机管理，并尝试使能四个电机。 |
| 8 | `RemoteCmd_Init(&huart2)` | 否 | 上位机命令不接收。 |
| 9 | `CompetitionRoute_Init()` | 是 | 默认自动启动固定路线，进入等待准备态。 |
| 10 | 主循环 `CompetitionRoute_Loop()` | 是 | 200ms 启动等待后运行路线。 |
| 11 | 主循环 `Mecanum_PeriodicTask()` | 是 | 每 20ms 左右把目标速度换算成四轮 RPM 并下发。 |
| 12 | 主循环 `Board_App_Loop/RemoteCmd_Loop/CSV_Debug_Loop` | 否 | 传感器、远程命令、CSV 调试都不跑。 |

## 5. 当前固定运行线路

坐标约定来自 `competition_route.h`：

| 速度量 | 含义 |
|---|---|
| `+vx` | 左横移 |
| `-vx` | 右横移 |
| `+vy` | 前进 |
| `-vy` | 后退 |
| `+wz` | 逆时针旋转，实际方向可由 `MECANUM_WZ_SIGN` 标定 |

默认速度：

| 动作 | 默认速度 |
|---|---|
| 前进/后退 | 0.18 m/s |
| 横移 | 0.14 m/s |
| 旋转 | 0.45 rad/s |

当前路线步骤：

| 步骤 | 动作 | 参数 | 目标速度 | 按代码估算持续时间 | 说明 |
|---:|---|---:|---:|---:|---|
| 1 | 前进 | 3300 mm | 0.18 m/s | 18333 ms | `vy=+0.18` |
| 2 | 旋转 | +90 deg | 0.45 rad/s | 3491 ms | `wz=+0.45`，开环时间旋转 |
| 3 | 前进 | 1000 mm | 0.18 m/s | 5556 ms | `vy=+0.18` |
| 4 | 旋转 | +90 deg | 0.45 rad/s | 3491 ms | `wz=+0.45` |
| 5 | 前进 | 1000 mm | 0.18 m/s | 5556 ms | `vy=+0.18` |
| 6 | 后退 | -1000 mm | 0.18 m/s | 5556 ms | `vy=-0.18` |
| 7 | 前进 | 500 mm | 0.18 m/s | 2778 ms | `vy=+0.18` |
| 8 | 右横移 | -350 mm | 0.14 m/s | 2500 ms | `vx=-0.14` |
| 9 | 前进 | 1500 mm | 0.18 m/s | 8333 ms | `vy=+0.18` |
| 10 | 左横移 | +400 mm | 0.14 m/s | 2857 ms | `vx=+0.14` |
| 11 | 前进 | 700 mm | 0.18 m/s | 3889 ms | `vy=+0.18` |
| 12 | 停止 | 0 | 0 | 0 | 设置速度为 0，状态进入 FINISHED |

大致总运动时间约 63.9 秒，加上启动等待 200ms。这个时间只代表代码按速度换算出的理论时间，不代表实际轮胎打滑、地面摩擦、负载、电池电压后的真实距离。

## 6. 底盘与四轮电机控制

### 6.1 机械与运动参数

| 参数 | 当前值 | 位置 |
|---|---:|---|
| 轮径 | 0.075 m | `mecanum_config.h:17` |
| 轮距/左右宽度 | 0.320 m | `mecanum_config.h:18` |
| 轴距/前后长度 | 0.220 m | `mecanum_config.h:19` |
| 最大 `vx` | 0.60 m/s | `mecanum_config.h:25` |
| 最大 `vy` | 0.60 m/s | `mecanum_config.h:26` |
| 最大 `wz` | 1.50 rad/s | `mecanum_config.h:27` |
| 轮速限制 | 500 RPM | `mecanum_config.h:28` |
| 加速度限制 | 2000 RPM/s | `mecanum_config.h:29` |
| 控制周期 | 20 ms | `mecanum_config.h:41` |
| 电机反馈周期配置 | 50 ms | `mecanum_config.h:42`，但反馈函数为空 |

### 6.2 四轮地址

| 轮子 | 地址 |
|---|---:|
| 左前 FL | 0x01 |
| 右前 FR | 0x02 |
| 左后 RL | 0x03 |
| 右后 RR | 0x04 |

### 6.3 控制链路

| 层级 | 文件 | 作用 |
|---|---|---|
| 路线层 | `competition_route.c` | 根据路线步骤设置 `vx/vy/wz`。 |
| 统一接口 | `mecanum.c` | 对外提供前进、后退、横移、旋转、停止。 |
| 底盘控制 | `chassis_control.c` | 保存目标速度，定期应用直行控制、逆解、限速、下发 RPM。 |
| 运动学 | `mecanum_kinematics.c` | 麦轮逆运动学，将 `vx/vy/wz` 转四轮 RPM。 |
| 电机管理 | `motor_manager.c` | 逐个电机发 RPM，维护在线状态字段。 |
| ZDT 协议 | `zdt_x42_modbus.c` | 生成 ZDT 使能、速度、停止、读速度命令。 |
| Modbus RTU | `modbus_rtu.c` | 组帧、CRC、UART 发送、可选接收。 |

### 6.4 “步进电机现在是否只收不发”

| 问题 | 当前判断 |
|---|---|
| 常规运行是否读取电机数据 | 否。`MotorManager_UpdateFeedback()` 直接返回 `MECANUM_OK`，没有调用 `ZDTX42_ReadMotorSpeed()`。 |
| 速度/使能写命令是否等待电机响应 | 否。`ZDTX42_EnableMotor()` 和 `ZDTX42_SetMotorSpeedRpm()` 调用 `Modbus_WriteMultipleRegisters(..., wait_response=0u)`。 |
| 代码有没有读电机速度能力 | 有。`ZDTX42_ReadMotorSpeed()` 会通过 `Modbus_ReadInputRegisters()` 读取速度反馈。 |
| 当前运行期是否用反馈闭环 | 没有。电机 `rpm_fb` 不更新，直行控制中的左右轮反馈分支理论存在但拿不到真实反馈。 |
| 更准确表述 | 从 STM32 固件当前运行逻辑看，电机驱动器常规只“接收”控制帧；STM32 不主动读取它们的运行反馈，也不等待写命令应答。 |

## 7. 陀螺仪 / IMU 现状

### 7.1 当前是否上电初始化

| 问题 | 当前结论 |
|---|---|
| JY61P 上电后当前固件会初始化吗 | 不会。 |
| I2C1 当前固件会初始化吗 | 不会。独立模式下 `MX_I2C1_Init()` 被跳过。 |
| `IMU_App_Init()` 当前会被调用吗 | 不会。因为 `Board_App_Init()` 被跳过。 |
| `IMU_App_Loop()` 当前会轮询吗 | 不会。因为 `Board_App_Loop()` 被跳过。 |
| IMU 变量当前是否会更新 | 不会，基本保持初始值，例如 `imu_valid=0`、`g_imu_update_count=0`。 |

### 7.2 代码中已实现的 IMU 能力

| 能力 | 代码位置 | 说明 |
|---|---|---|
| I2C 地址 | `jy61p.h:11-12` | JY61P 7 位地址 0x50。 |
| 读取寄存器 | `jy61p.c:247-260` | 从 `JY61P_REG_START` 连续读取 13 个 16 位寄存器。 |
| 读取数据 | `jy61p.c:59-84` | 解码加速度、角速度、roll、pitch、yaw、温度。 |
| 数据合理性检查 | `jy61p.c:89-108` | 限制加速度、角速度、角度范围。 |
| 软件陀螺零偏校准 | `jy61p.c:116-144` | 静止状态累计 200 个样本。 |
| yaw 融合估计 | `jy61p.c:148-197` | 用陀螺积分和模块 yaw 角做互补修正。 |
| 软件 yaw 归零 | `jy61p.c:348-357` | 设置 `yaw_offset_deg`，把估计 yaw 清零。 |
| 硬件 yaw 归零 | `jy61p.c:381-392` | 写 JY61P 解锁、零航向、保存命令。 |
| 硬件加速度校准 | `jy61p.c:395-407` | 写 JY61P 加速度校准命令并保存。 |

### 7.3 IMU 当前给 STM32 提供了哪些数据

严格按“当前固件实际运行”来说：没有提供有效实时数据，因为未初始化、未轮询。

如果把独立模式关闭并启用 `Board_App`，IMU 代码会向 STM32 发布以下数据：

| 数据 | 变量 | 单位/含义 |
|---|---|---|
| 估计航向 | `car_yaw_est_deg` | deg，软件融合后的车体 yaw |
| 车体俯仰 | `car_pitch_deg` | deg |
| 车体横滚 | `car_roll_deg` | deg |
| yaw 角速度 | `gyro_yaw_dps` | deg/s |
| 前向加速度 | `acc_forward_g` | g |
| 右向加速度 | `acc_right_g` | g |
| 下向加速度 | `acc_down_g` | g |
| 温度 | `temperature_c` / `g_imu_temp_c` | 摄氏度 |
| 原始欧拉角 | `g_imu_roll_deg/pitch/yaw` | deg |
| 原始加速度 | `g_imu_ax_g/ay_g/az_g` | g |
| 原始角速度 | `g_imu_gx_dps/gy_dps/gz_dps` | deg/s |
| 状态 | `imu_valid`、`imu_calibrated`、`g_imu_state`、`g_imu_last_error` | 有效性、校准状态、错误码 |

### 7.4 航向校准是否生效

| 场景 | 是否生效 | 原因 |
|---|---|---|
| 当前独立固定路线固件 | 不生效 | IMU 不初始化、不轮询；路线旋转也设置为按时间开环。 |
| 关闭独立模式后，普通传感器链路 | 部分可生效 | `IMU_App_Init()` 会调用 `JY61P_StartGyroCalibration()`，但需要车静止并采满 200 个样本；`ZERO` 命令可做软件 yaw 归零。 |
| 当前路线旋转控制 | 不使用 yaw | `COMPETITION_ROUTE_USE_IMU_ROTATION 0U`，且 `competition_route.c` 旋转步骤按时间结束。 |
| 直行闭环 | 当前默认不启用 | `STRAIGHT_CONTROL_ENABLE_DEFAULT 0u`，固定路线启动还会 `Mecanum_EnableStraightControl(0U)`。 |

## 8. 激光测距现状

| 问题 | 当前结论 |
|---|---|
| 激光当前是否上电初始化 | 不会。I2C1 和 `Laser_App_Init()` 都被独立模式跳过。 |
| 当前是否参与避障或急停 | 不参与。固定路线安全检查在独立模式下直接放行。 |
| 代码中是否支持激光 | 支持。ATK-MS53L1M/VL53L1X 初始化、读 ID、长距离模式、150ms 周期测距、滤波都存在。 |
| 告警阈值 | 500mm warning、300mm slow、100mm stop，100mm 需连续 2 次确认。 |
| RemoteCmd 中激光安全 | 也被简化关闭，`REMOTE_CMD_ENABLE_LASER_SAFETY 0U`。 |

## 9. 树莓派 / 上位机命令链路

| 项目 | 当前状态 |
|---|---|
| USART2 初始化 | 当前独立模式不初始化。 |
| USART2 中断 | `usart.c` 中配置了 USART2 中断，但只有初始化 USART2 时才会启用；当前不启用。 |
| `RemoteCmd_Init()` | 当前不调用，因此不会启动 `HAL_UART_Receive_IT()`。 |
| 命令协议 | 代码支持 `HB`、`VEL vx vy wz`、`STOP`、`ESTOP`、`CLR`、`ZERO`、`GYROCAL`、`ACCCAL`、`STAT?`、`IMU?`、`LASER?`、`SENS?`、`SENS_STREAM ON/OFF`、`SC ON/OFF/?`。 |
| 心跳保护 | 代码宏默认关闭，`REMOTE_CMD_REQUIRE_HEARTBEAT 0U`。 |
| 激光保护 | 代码宏默认关闭，`REMOTE_CMD_ENABLE_LASER_SAFETY 0U`。 |
| 当前固定路线是否等待树莓派 | 不等待，`COMPETITION_ROUTE_REQUIRE_RPI_LINK 0U`。 |

## 10. 当前可以实现的功能

| 功能 | 当前是否可用 | 说明 |
|---|---|---|
| 上电自动运行固定路线 | 可用 | `COMPETITION_ROUTE_AUTO_START 1U`，200ms 后开始。 |
| 前进/后退 | 可用 | 由固定路线调用 `Mecanum_SetVelocity(0, +/-speed, 0)`。 |
| 左右横移 | 可用 | 由固定路线调用 `Mecanum_SetVelocity(+/-speed, 0, 0)`。 |
| 原地旋转 | 可用 | 开环按时间旋转。 |
| 四轮速度解算 | 可用 | 麦轮逆运动学已实现。 |
| 四电机使能与速度下发 | 可用 | 上电初始化时使能，周期性下发 RPM。 |
| 停止 | 可用 | 路线结束或停止命令路径中都可设置零速度。 |
| 树莓派速度遥控 | 当前不可用 | 代码存在，但独立模式不启用 USART2/RemoteCmd。 |
| IMU 数据读取 | 当前不可用 | 代码存在，但当前不初始化 I2C/IMU。 |
| 激光测距 | 当前不可用 | 代码存在，但当前不初始化 I2C/激光。 |
| 航向闭环直行 | 当前不可用 | 默认关闭，且 IMU 不工作。 |
| 旋转按航向闭环 | 当前不可用 | `COMPETITION_ROUTE_USE_IMU_ROTATION 0U`，实现路径未被当前路线使用。 |
| 电机速度反馈闭环 | 当前不可用 | 读速度函数存在，但运行期反馈函数空。 |

## 11. 当前存在的问题与风险

| 优先级 | 问题 | 影响 | 证据/原因 |
|---|---|---|---|
| 高 | 固定路线完全开环 | 路径误差、轮滑、电池电压、地面变化会持续累积，最终位置和角度偏差明显。 | 路线注释明确 open-loop，旋转也不等 IMU yaw。 |
| 高 | IMU 当前未启用 | 航向校准、航向反馈、直行纠偏都不生效。 | 独立模式跳过 I2C 和 `Board_App`。 |
| 高 | 激光当前未启用 | 无避障、无近距离急停保护。 | `Laser_App_Init/Loop` 不会被调用。 |
| 高 | 电机反馈为空 | 无法知道电机是否掉线、堵转、速度是否达标。 | `MotorManager_UpdateFeedback()` 空实现。 |
| 高 | 常规写命令不等待应答 | `online` 状态只代表 UART 发送成功倾向，不代表驱动器真的执行或在线。 | `wait_response=0u`。 |
| 中 | 树莓派链路当前被绕开 | 上位机不能遥控、不能请求传感器状态、不能下发校准。 | 独立模式跳过 USART2 和 RemoteCmd。 |
| 中 | 安全联锁被简化关闭 | 心跳超时、激光故障不会锁车。 | `REMOTE_CMD_REQUIRE_HEARTBEAT 0U`、`REMOTE_CMD_ENABLE_LASER_SAFETY 0U`。 |
| 中 | RS485 方向脚未由 MCU 控制 | 如果硬件不是自动收发 RS485 模块，通信可能只能发或方向不稳定。 | `MODBUS_USE_RS485_DIR_GPIO 0u`。 |
| 中 | `ESTOP` 语义被弱化 | RemoteCmd 中 `ESTOP` 实际调用 `Mecanum_Stop()`，且外部 emergency 被清零，不是锁存式急停。 | `remote_cmd.c:444-449`。 |
| 中 | 默认系统时钟 8MHz | Modbus 发送和 DWT us 延时仍可工作，但性能裕量低；需确认与驱动器通信稳定性。 | `SystemClock_Config()` 使用 HSI，无 PLL。 |
| 低 | 文档/README 编码异常 | `UserModules\README.md` 中文显示乱码，不利于交接。 | 文件内容疑似编码不一致。 |
| 低 | Debug `default.size.stdout` 缺失 | 说明最后一次构建可能没有保留 size 输出，或被清理；不影响 `.elf/.bin` 存在。 | Debug 目录未找到该文件。 |

## 12. 建议的纠错路线

| 阶段 | 目标 | 建议动作 | 验收标准 |
|---|---|---|---|
| 1 | 确认当前固定路线版本可稳定下发电机命令 | 保持独立模式，单独测试四轮方向、RPM 标定、RS485 自动收发。 | 四轮地址和方向正确，前进/横移/旋转方向符合坐标约定。 |
| 2 | 恢复电机反馈 | 在 `MotorManager_UpdateFeedback()` 中轮询 `ZDTX42_ReadMotorSpeed()`，更新 `rpm_fb/online/timeout_cnt/crc_error_cnt`。 | 运行时能看到四轮真实 RPM，掉线能被检测。 |
| 3 | 启用 IMU 链路 | 将 `FIXED_ROUTE_STANDALONE_ENABLE` 改为 0 或拆分传感器初始化，使 I2C1、`IMU_App_Init/Loop` 独立可启用。 | `imu_valid=1`，`g_imu_update_count` 持续增加，yaw 静止漂移可接受。 |
| 4 | 验证陀螺仪校准 | 上电静止 3s 以上，确认 200 样本零偏完成；通过 `ZERO/GYROCAL` 或调试变量验证。 | `imu_calibrated=1`，yaw 归零后短时间稳定。 |
| 5 | 将旋转改为 IMU 闭环 | 让路线旋转步骤记录目标 yaw，用 PID/误差阈值结束，而不是纯时间。 | 90 度旋转误差稳定在 2-3 度以内。 |
| 6 | 恢复激光测距和安全 | 启用 `Laser_App_Init/Loop`，恢复激光异常/过近急停策略。 | `laser_ok=1`，距离读数合理，100mm 内连续触发停止。 |
| 7 | 恢复树莓派链路 | 启用 USART2、RemoteCmd、心跳和状态回传。 | 能收 `VEL/STOP/STAT?/IMU?/LASER?`，心跳丢失能停车。 |
| 8 | 路线从开环升级为半闭环/闭环 | 平移至少加入电机反馈里程估算，旋转使用 IMU；必要时加入激光或视觉定位。 | 多次运行终点偏差可重复、可调。 |

## 13. 对关键问题的直接回答

| 问题 | 回答 |
|---|---|
| 目前使用了哪些硬件 | 当前实际使用 STM32F103C8T6、USART1 RS485、四个 ZDT_X42 步进驱动/电机、SWD。代码中还准备了 JY61P IMU、ATK-MS53L1M/VL53L1X 激光、USART2 树莓派链路，但当前固件不启用。 |
| 目前可以实现什么功能 | 上电自动跑固定路线；可前进、后退、横移、旋转、停止；可周期性向四个电机发送速度。 |
| 运行线路是什么 | 3300mm 前进、90度转、1000mm 前进、90度转、1000mm 前进、1000mm 后退、500mm 前进、350mm 右横移、1500mm 前进、400mm 左横移、700mm 前进、停止。 |
| 陀螺仪对航向校准是否生效 | 当前不生效。因为当前不初始化/轮询 IMU，路线旋转也不使用 yaw。 |
| 陀螺仪现在有没有上电初始化 | 当前没有。`FIXED_ROUTE_STANDALONE_ENABLE=1` 时 I2C1 和 `Board_App_Init()` 都跳过。 |
| 陀螺仪现在给 STM32 提供哪些数据 | 当前实际没有有效数据。代码启用后可提供 yaw/pitch/roll、三轴加速度、三轴角速度、温度、有效状态、校准状态、错误计数等。 |
| 步进电机现在是否只收不发 | 常规运行可理解为“只接收控制，不被读取反馈”：STM32 给驱动器发 Modbus 写帧，不等待写应答；读速度代码存在但运行期没调用。 |
| 激光现在是否保护车辆 | 当前不保护，因为激光没有初始化/轮询，固定路线安全检查也被独立模式放行。 |
| 树莓派现在是否参与 | 当前不参与，USART2/RemoteCmd 不初始化。 |

## 14. 最短纠错建议

如果目标是比赛/实车稳定运行，建议不要直接在当前开环版本上堆路线距离，而是按下面顺序改：

| 顺序 | 最小改动 | 为什么 |
|---:|---|---|
| 1 | 先把四电机方向和地址逐个实测确认 | 方向错会让所有路线和闭环判断都失真。 |
| 2 | 给 `MotorManager_UpdateFeedback()` 补真实速度回读 | 先知道电机到底有没有执行命令。 |
| 3 | 启用 I2C1 + IMU，不一定立刻启用激光 | 航向是当前路线最大误差来源。 |
| 4 | 旋转步骤改为 yaw 闭环 | 90 度旋转比平移更容易先闭环成功。 |
| 5 | 再启用激光和上位机安全 | 安全和状态回传补齐后，调试效率会高很多。 |

