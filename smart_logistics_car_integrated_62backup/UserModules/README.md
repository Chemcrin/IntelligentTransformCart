# UserModules

该目录预留给后续模块，例如上位机通信、任务状态机、路径规划、巡线/视觉等。新增模块建议只调用 `Sensor_GetSnapshot()` 和 `Mecanum_SetVelocity()` 等上层接口，不直接访问底层 I2C 或 Modbus 驱动。
