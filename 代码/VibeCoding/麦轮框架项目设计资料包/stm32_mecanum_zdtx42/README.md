# STM32F103C8T6 ZDT_X42 麦克纳姆轮模块说明

## 1. 代码文件清单

| 路径 | 作用 |
|---|---|
| `Core/Inc/mecanum.h` | 上层调用入口，提供初始化、速度控制、停止、急停、测试接口 |
| `Core/Src/mecanum.c` | 上层接口转发实现 |
| `Core/Inc/mecanum_config.h` | 机械参数、地址、限速、RS485 方向电平、ZDT 缩放等集中配置 |
| `Core/Inc/mecanum_types.h` | 公共状态码、状态机、速度结构体、电机状态结构体 |
| `Core/Inc/chassis_control.h` | 底盘状态机与周期任务接口 |
| `Core/Src/chassis_control.c` | 速度限幅、命令超时、反馈轮询、故障/急停保护、周期下发 |
| `Core/Inc/mecanum_kinematics.h` | 麦轮运动学接口 |
| `Core/Src/mecanum_kinematics.c` | 逆运动学、正运动学、m/s 与 RPM 换算、RPM 限幅/斜率限制 |
| `Core/Inc/motor_manager.h` | 四电机管理接口 |
| `Core/Src/motor_manager.c` | 电机地址表、方向系数、四轮同步下发、反馈缓存 |
| `Core/Inc/zdt_x42_modbus.h` | ZDT_X42 寄存器与命令接口 |
| `Core/Src/zdt_x42_modbus.c` | ZDT_X42 使能、速度模式、停止、同步触发、读转速 |
| `Core/Inc/modbus_rtu.h` | Modbus RTU 通信接口 |
| `Core/Src/modbus_rtu.c` | CRC16、RS485 半双工方向切换、UART3 阻塞式收发 |

## 2. 坐标系与运动接口

| 变量 | 正方向 | 单位 |
|---|---|---|
| `vx_mps` | 小车左侧 | `m/s` |
| `vy_mps` | 小车车头方向 | `m/s` |
| `wz_radps` | 绕 Z 轴旋转，实际正方向由 `MECANUM_WZ_SIGN` 标定 | `rad/s` |

常用调用：

```c
Mecanum_Init(&huart3);
Mecanum_MoveForward(0.20f);
Mecanum_MoveLeft(0.15f);
Mecanum_MoveVector(0.10f, 0.20f);   // 斜向移动：左 + 前
Mecanum_Rotate(0.30f);
Mecanum_Stop();
Mecanum_EmergencyStop();
```

主循环中必须周期调用：

```c
while (1)
{
    Mecanum_PeriodicTask();
}
```

## 3. 导入 STM32CubeIDE 工程

| 步骤 | 操作 |
|---|---|
| 1 | 使用 CubeMX/CubeIDE 先创建 `STM32F103C8T6` HAL 工程 |
| 2 | 将本目录下 `Core/Inc/*.h` 复制到目标工程 `Core/Inc` |
| 3 | 将本目录下 `Core/Src/*.c` 复制到目标工程 `Core/Src` |
| 4 | 在 `main.c` 中包含 `#include "mecanum.h"` |
| 5 | 在 `MX_USART3_UART_Init()`、`MX_GPIO_Init()` 之后调用 `Mecanum_Init(&huart3);` |
| 6 | 在主循环中调用 `Mecanum_PeriodicTask();` |
| 7 | 根据实车修改 `mecanum_config.h` 中的待标定参数 |

`main.c` 示例：

```c
#include "mecanum.h"

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART3_UART_Init();

    Mecanum_Init(&huart3);

    while (1)
    {
        Mecanum_PeriodicTask();
    }
}
```

## 4. IOC 引脚与外设配置

| 外设/引脚 | 配置 | 说明 |
|---|---|---|
| `USART3_TX` | `PB10`，Alternate Function Push Pull | 连接 RS485 模块 DI |
| `USART3_RX` | `PB11`，Input | 连接 RS485 模块 RO |
| `USART3` | Asynchronous，`115200`，`8N1`，无硬件流控 | 与 ZDT_X42 默认 Modbus RTU 通信 |
| `RS485_DIR` | 任意空闲 GPIO Output，例如 `PB1` | 连接 RS485 模块 `DE` 与 `/RE`，默认 RX |
| `SYS` | Serial Wire | 保留 SWD 调试 |
| `Timebase` | SysTick 或 TIM | 需要 `HAL_GetTick()` 正常提供 1 ms tick |

如果你的 RS485 方向脚不是 `PB1`，请在 CubeMX 中给该引脚设置 User Label：`RS485_DIR`。若生成的 `main.h` 已包含 `RS485_DIR_GPIO_Port` 和 `RS485_DIR_Pin`，代码会自动使用。

## 5. 必须标定的参数

| 参数 | 默认值 | 标定方式 |
|---|---:|---|
| `RS485_DIR_TX_LEVEL` / `RS485_DIR_RX_LEVEL` | `SET` / `RESET` | 调用 `RS485_TestDirectionLevel(0x01, &rpm)` 读转速，若一直超时，先反转电平 |
| `ZDT_SPEED_MODE_SCALE` | `1.0f` | 单电机下发 `60/120/300 RPM`，观察反馈和实际转速 |
| `ZDT_SPEED_FEEDBACK_SCALE` | `1.0f` | 读取 `0x0044` 与实际转速对比 |
| `motor_dir_sign[4]` | `{+1,+1,+1,+1}` | 调用 `Mecanum_TestMotorDirection(test_rpm)`，逐轮记录方向 |
| `MECANUM_WZ_SIGN` | `+1.0f` | 调用 `Mecanum_TestRotate(+0.3f)`，若旋转方向与定义相反则改为 `-1.0f` |
| `RPM_LIMIT` / `ACC_LIMIT_RPM_S` | `500` / `2000` | 由实车稳定性、电源能力、负载决定 |

## 6. 推荐测试顺序

| 顺序 | 测试项 | 接口 |
|---:|---|---|
| 1 | RS485 方向脚与 UART3 通信 | `RS485_TestDirectionLevel()` |
| 2 | 单电机读转速 | `ZDTX42_TestReadSpeed()` |
| 3 | 单电机速度缩放 | `ZDTX42_TestSingleMotor()` |
| 4 | 四电机地址与轮位 | 依次测试 `0x01` 到 `0x04` |
| 5 | 四轮方向系数 | `Mecanum_TestMotorDirection()` |
| 6 | 前进/后退/左移/右移 | `Mecanum_MoveForward()` 等 |
| 7 | 斜向移动 | `Mecanum_MoveVector(vx, vy)` |
| 8 | 原地旋转 | `Mecanum_Rotate()` |
| 9 | 停止与急停 | `Mecanum_Stop()`、`Mecanum_EmergencyStop()` |

## 7. 注意事项

- 四轮地址固定：FL=`0x01`，FR=`0x02`，RL=`0x03`，RR=`0x04`，不要通过改地址修方向。
- 坐标系固定：X 左正、Y 前正、Z 上正。
- 广播地址 `0x00` 只用于同步触发和广播急停，不等待响应。
- 当前实现采用阻塞式 HAL UART 收发，逻辑简单稳定；若后续控制周期更紧，可在保持接口不变的前提下替换为 DMA/中断事务状态机。
- 急停状态不会自动恢复，必须显式调用 `Mecanum_ClearEmergencyStop()`。
