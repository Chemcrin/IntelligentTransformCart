# `final_smartcart` 工程分析报告

## 一、工程概述

这是一个基于 **STM32F103C8T6**（64KB Flash, 20KB RAM）的麦克纳姆轮智能物流小车竞赛项目。硬件配置：

|硬件|接口|引脚|
|---|---|---|
|JY61P IMU（姿态传感器）|I2C1|PB6/PB7|
|ZDT-X42-V2 四电机驱动|USART1 (RS485 Modbus)|PA9/PA10|
|调试串口|USART2|PA2/PA3|
|SWD 下载调试|SYS|PA13/PA14|

核心功能：**上电后自动执行固定路线**（7步：直行→旋转→直行→等待→旋转→直行+横移→多段组合），全程使用 IMU 偏航角做闭环校正。

---

## 二、软件架构（分层良好）

```
competition_route  (固定路线状态机)
       ↓
    mecanum        (顶层API，薄封装)
       ↓
 chassis_control   (底盘控制聚合器)
   ↓         ↓
straight_control  mecanum_kinematics
(PID直行校正)     (麦克纳姆运动学)
       ↓
  motor_manager    (四电机生命周期管理)
       ↓
  zdt_x42_modbus   (ZDT-X42-V2 协议层)
       ↓
   modbus_rtu      (Modbus RTU 帧/CRC16)
       ↓
   USART1 HAL      (硬件抽象层)
```

同时有独立的 `sensor_bridge` → `jy61p` (IMU驱动) 链路为上层提供姿态数据。

---

## 三、严重问题

### 1. 主循环阻塞式架构 — 时序不可控

[main.c:38-44](vscode-webview://1l8k96jve1e500icvklp11846tg1bndilev7pbm710da6jlse8a3/Core/Src/main.c#L38-L44) 的主循环：

```c
while (1) {
    SensorBridge_Loop();      // I2C 读取 IMU
    CompetitionRoute_Loop();  // 路线状态机 → 发 Modbus 命令给 4 个电机
    Mecanum_PeriodicTask();   // 再次发 Modbus 命令给 4 个电机
    HAL_Delay(5U);            // 忙等 5ms
}
```

问题链条：

- `CompetitionRoute_Loop()` 调用 `Mecanum_SetVelocity()` → `ChassisControl_SetVelocity()` 只**更新目标值**，不立即发送
- 实际发送在 `Mecanum_PeriodicTask()` → `ChassisControl_PeriodicTask()` → `MotorManager_SetWheelRpm()` 中
- `MotorManager_SetWheelRpm()` 对 **4 个电机逐个发送 Modbus 命令**，每次是**阻塞式** `HAL_UART_Transmit`，115200 波特率下每帧约 1.4ms
- 如果任何一个电机不响应，系统就死等在超时上

这意味着实际循环周期远超 5ms，可能达到 20-50ms，**实时性完全不可控**。

### 2. MotorManager_UpdateFeedback 是空函数 — 直行 L/R PID 永不生效

[motor_manager.c:86-89](vscode-webview://1l8k96jve1e500icvklp11846tg1bndilev7pbm710da6jlse8a3/Core/Src/motor_manager.c#L86-L89)：

```c
MecanumResult_t MotorManager_UpdateFeedback(void)
{
    return MECANUM_OK;  // 什么也不做！
}
```

`g_motor_states[].rpm_fb` 永远为 0.0f。而 [straight_control.c:106-142](vscode-webview://1l8k96jve1e500icvklp11846tg1bndilev7pbm710da6jlse8a3/Core/Src/straight_control.c#L106-L142) 的 `straight_read_left_right_rpm()` 依赖 `rpm_fb` 来做左右轮速差校正。结果就是**直行控制器的 L/R 漂移补偿 PID 完全不起作用**。

### 3. 紧急停止逻辑被静默绕过

[chassis_control.c:116-122](vscode-webview://1l8k96jve1e500icvklp11846tg1bndilev7pbm710da6jlse8a3/Core/Src/chassis_control.c#L116-L122)：

```c
if (g_external_emergency != 0u) {
    g_external_emergency = 0u;  // 读取后直接清零，不做任何处理！
}
if ((g_chassis_state == MECANUM_STATE_EMERGENCY_STOP) || 
    (g_chassis_state == MECANUM_STATE_FAULT)) {
    g_chassis_state = MECANUM_STATE_READY;  // 紧急停止状态直接恢复成READY！
}
```

紧急停止标志被读取后直接丢弃，状态被无条件恢复。**紧急停止功能形同虚设**。

### 4. Modbus 接收假设固定帧长 — 违反 Modbus RTU 协议

[modbus_rtu.c:121](vscode-webview://1l8k96jve1e500icvklp11846tg1bndilev7pbm710da6jlse8a3/Core/Src/modbus_rtu.c#L121)：

```c
hal_ret = HAL_UART_Receive(g_modbus_uart, rx, rx_max_len, timeout_ms);
```

Modbus RTU 协议依赖 **3.5 字符时间的帧间隔** 来判断帧结束，帧的实际长度取决于功能码和数据内容。这里却假设响应帧必定等于 `rx_max_len`（预期长度）。如果从机返回的字节数不同（如异常响应），会直接等到超时或读到错误长度的数据。

---

## 四、中等问题

### 5. 大量重复的工具函数

以下函数在多个文件中被各自实现了一遍：

|函数|出现位置|
|---|---|
|`absf` / `route_absf` / `straight_absf` / `absf_local`|`competition_route.c`, `straight_control.c`, `mecanum_kinematics.c`, `jy61p.c`|
|`clampf` / `route_clampf` / `straight_clampf` / `clampf_local` / `clampf_chassis`|同上分布|
|`wrap_180` / `route_wrap_pi` / `straight_wrap_pi` / `jy61p_wrap_180`|角度包裹函数重复4次|

应提取到单一的 `math_utils.h/c` 中。

### 6. `mecanum.c` 是纯转发层 — 无实际价值

[mecanum.c](vscode-webview://1l8k96jve1e500icvklp11846tg1bndilev7pbm710da6jlse8a3/Core/Src/mecanum.c) 共 129 行，**所有函数** 都是直接转发到 `chassis_control.c` 或 `zdt_x42_modbus.c`。例如：

```c
MecanumResult_t Mecanum_MoveForward(float speed_mps) {
    return Mecanum_SetVelocity(0.0f, speed_mps, 0.0f);  // 只改参数
}
MecanumResult_t Mecanum_Stop(void) {
    return ChassisControl_Stop();  // 直接转发
}
```

这个中间层增加了调用链复杂度但没有提供任何抽象价值。

### 7. 测试函数混入生产 API

[mecanum.h:31-34](vscode-webview://1l8k96jve1e500icvklp11846tg1bndilev7pbm710da6jlse8a3/Core/Inc/mecanum.h#L31-L34) 暴露的测试接口：

- `Mecanum_TestMoveForward` / `Mecanum_TestMoveLeft` / `Mecanum_TestRotate`
- `RS485_TestDirectionLevel`

[motor_manager.h:21](vscode-webview://1l8k96jve1e500icvklp11846tg1bndilev7pbm710da6jlse8a3/Core/Inc/motor_manager.h#L21) 暴露的：

- `Mecanum_TestMotorDirection` — 内部调用 `HAL_Delay(800)` + `HAL_Delay(400)` **逐个电机测试**，耗时 4.8 秒！

这些应放到独立的调试模块。

### 8. 激光传感器全是桩代码

[sensor_bridge.c:67-87](vscode-webview://1l8k96jve1e500icvklp11846tg1bndilev7pbm710da6jlse8a3/Core/Src/sensor_bridge.c#L67-L87) 中所有 Laser 函数返回硬编码值（false, -1.0f, 0）。虽然 `config.h` 说明不需要激光，但这些死代码仍然占用 Flash 空间和维护心智负担。`SensorSnapshot_t` 结构体中也有多个无用的激光字段。

### 9. NMI_Handler 死循环 — 过于激进

[stm32f1xx_it.c:76-78](vscode-webview://1l8k96jve1e500icvklp11846tg1bndilev7pbm710da6jlse8a3/Core/Src/stm32f1xx_it.c#L76-L78)：

```c
void NMI_Handler(void) {
    while (1) {}  // NMI 触发后 CPU 永久锁死
}
```

NMI 可能由时钟安全系统（CSS）触发，属于可恢复事件。直接死循环意味着任何时钟异常都会让小车永久停机。

### 10. 缺少看门狗

工程中完全没有配置 IWDG 或 WWDG。对于自主运行的移动机器人，如果程序跑飞，电机会保持最后的转速设定不变，小车会失控。

---

## 五、代码质量问题

### 11. 未使用的死代码

|代码|位置|
|---|---|
|`Mecanum_ForwardKinematics()`|`mecanum_kinematics.c:49-68` — 从未被调用|
|`FIXED_ROUTE_STANDALONE_ENABLE` 宏|`config.h:7` — 定义了但没有任何 `#if` 检查|
|`CSV_DEBUG_ENABLE` 及相关宏|`config.h:10-16` — 完全未被引用|
|`ZDTX42_Init()`|`zdt_x42_modbus.c:20-22` — 空函数体|
|`ZDTX42_SyncTrigger()`|`zdt_x42_modbus.c:80-85` — 从未被调用|
|`CHASSIS_COMMAND_TIMEOUT_MS` / `MOTOR_TIMEOUT_FAULT_LIMIT`|`mecanum_config.h:43-44` — 设为0禁用|

### 12. 魔法数字

```c
// jy61p.c
if (jy61p_absf(data->ax_g) > 20.0f)   // 20.0f 是什么？应为 JY61P_ACC_PLAUSIBLE_MAX_G
if (jy61p_absf(data->gx_dps) > 2200.0f) // 同理

// competition_route.c
route_calc_duration_ms 中的 0.01f, 0.05f, 0.5f, 1000.0f // 无宏定义
```

### 13. NULL 检查风格不一致

```c
if (vel == 0)      // chassis_control.c — 用 == 0
if (dev == NULL)    // jy61p.c — 用 == NULL
if (rpm == 0)       // motor_manager.c
if (data == NULL)   // jy61p.c
```

### 14. `HAL_Delay` 在 `Modbus_InterFrameGap` 中的使用

[modbus_rtu.c:82](vscode-webview://1l8k96jve1e500icvklp11846tg1bndilev7pbm710da6jlse8a3/Core/Src/modbus_rtu.c#L82) 在发送前调用 `HAL_Delay(MODBUS_INTER_FRAME_GAP_MS)` (2ms)。`HAL_Delay` 基于 SysTick 中断，如果在中断上下文中调用会死锁。虽然当前不在 ISR 中，但这是脆弱的做法。

---

## 六、简化和改进建议

### 优先级排序：

**P0 — 影响安全和基本功能：**

1. **实现 `MotorManager_UpdateFeedback`**：周期性读取电机实际转速（可通过 Modbus 逐个读取或使用轮询策略），使 L/R PID 能正常工作
2. **修复紧急停止逻辑**：`EMERGENCY_STOP` 状态应该阻止速度命令下发，而不是被静默重置
3. **添加独立看门狗 (IWDG)**：在 `main.c` 初始化中配置，在循环中喂狗
4. **检查主循环时序**：在最坏情况下（4个电机全部超时），一轮循环的耗时是多少？需要做最坏情况分析

**P1 — 影响代码质量和可维护性：** 5. **消除 Modbus 阻塞问题**：如果不能改用 DMA，至少将 4 个电机的命令分散到多个循环周期中发送（每个周期只发一个电机的命令） 6. **提取公共工具函数**：创建 `common/math_utils.h`，合并所有 `absf/clampf/wrap_pi` 实现 7. **分离测试代码**：将 `Test*` 函数移到独立的 `test_utils.c` 8. **移除死代码或添加条件编译**：激光桩代码可以用 `#if LASER_ENABLE` 包裹；`ForwardKinematics` 可以保留但加注释说明用途

**P2 — 改进和优化：** 9. **合并或精简 mecanum 层**：让 `competition_route` 直接调用 `ChassisControl_SetVelocity()`，跳过 `mecanum.c` 的纯转发 10. **Modbus 接收改为基于帧间隔**：用定时器检测 3.5 字符时间的空闲来判断帧结束 11. **添加 `.gitignore`**：排除 `Debug/`、`.settings/` 等构建产物 12. **NMI_Handler 改为记录错误后尝试恢复**：至少区分致命的和可恢复的 NMI 源 13. **添加系统状态 LED 指示**：用 GPIO 指示运行/故障/完成状态，方便现场调试

---

## 七、总结

这个工程的**分层架构设计是合理的**，IMU 驱动和运动学模块写得比较扎实。核心问题集中在：

- **实时性**：同步阻塞式通信使得控制周期不可预测
- **反馈缺失**：电机实际转速从未被读取，导致直行 L/R 校正 PID 无法工作
- **安全机制破损**：紧急停止和故障状态被静默绕过
- **代码冗余**：大量重复工具函数、纯转发层、死代码

这些问题中，反馈缺失和安全机制是最需要优先解决的。是否需要我开始实施某个具体的改进？