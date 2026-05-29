# ATK_JY_NewVer_GPT

STM32F103C8T6 / STM32C8T6 传感器姿态与距离感知代码包。

- JY61P：I2C1 读取姿态、角速度、加速度，含上电静止校准、yaw 互补滤波、车体系映射。
- ATK-MS53L1M / VL53L1X：I2C1 读取距离，含中值滤波、滑动平均、跳变剔除、近距离优先。
- Board App：提供 `Board_App_Init()`、`Board_App_Loop()`、`SENSOR_Snapshot_t` 快照。

导入和 IOC 配置见：`Docs/项目说明_导入与IOC配置.md`。
