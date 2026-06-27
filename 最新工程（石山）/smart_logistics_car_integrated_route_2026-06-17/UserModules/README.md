# UserModules

当前正式参与构建的用户模块只有：

- `remote_cmd/`：树莓派/上位机 USART2 文本命令、状态回传、IMU/激光回传、安全联锁。

已删除的历史测试模块：

- `sensor_uart3_stream_test/`
- `uart3_sensor_stream_temp/`
- `bringup_mode/`

正式运行方案中 `USART3/PB10/PB11` 保持空置，不再作为传感器流或电机链路使用。
