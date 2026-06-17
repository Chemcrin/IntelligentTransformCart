# 个性化物流小车 - 更新日志

> 自动维护，每天由星尘光检查项目更新情况并记录。

---

## v0.1.0 — 初始建立（2026-05-22）

- 项目初始化：添加 README、LICENSE（MIT）、.gitignore
- 代码目录结构搭建：陀螺仪 UART 驱动（JY61P）、传感器数值函数文档
- 新增 PCB 目录，添加电路板设计文件
- 添加 VibeCoding 目录：姿态模块资料包、传感器官方文档、麦轮框架项目资料

## v0.1.1 — 代码重构 & 建模资料（2026-05-29）

- 代码目录结构重整，新增 STM32 传感器驱动库 `smart_logistics_car_integrated`
  - 包含：JY61P 陀螺仪、ATK-MS53L1M 激光测距、VL53L1X API、ZDT 步进电机 Modbus-RTU 驱动
  - 麦伦运动学模块（mecanum_kinematics）、电机管理（motor_manager）、底盘控制（chassis_control）
- 新增建模资料：树莓派 4B 3D 模型（SLDPRT/SLDASM/STEP）

## v0.1.2 — 文档补充 & 清理（2026-06-04）

- 补充麦轮框架资料：麦伦运动控制系统设计文档 V4Pro/V5Pro、代码生成提示词
- 优化项目文档
- 清理冗余备份文件
- 移除临时代码运行文件 tempCodeRunnerFile.python

---

## 待观察

- 自上次提交（2026-06-04）以来已 13 天无新提交
- 当前项目处于四六级备考期，暂停开发中

---

*本日志由星尘光自动维护，每日更新。*
