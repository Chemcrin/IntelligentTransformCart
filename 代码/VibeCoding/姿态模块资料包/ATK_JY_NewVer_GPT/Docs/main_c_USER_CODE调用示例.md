# main.c USER CODE 

以下代码只放在 `USER CODE BEGIN` / `USER CODE END` 区域，避免 CubeMX 重新生成代码时被覆盖。

## 1. Includes 区域

```c
/* USER CODE BEGIN Includes */
#include "board_app.h"
/* USER CODE END Includes */
```

## 2. 初始化区域

找到 `main()` 中外设初始化之后的位置：

```c
MX_GPIO_Init();
MX_I2C1_Init();
MX_USART1_UART_Init();
```

在其后加入：

```c
/* USER CODE BEGIN 2 */
Board_App_Init();
/* USER CODE END 2 */
```

## 3. 主循环区域

```c
while (1)
{
  /* USER CODE BEGIN WHILE */
  Board_App_Loop();

  SENSOR_Snapshot_t snap;
  Board_App_GetSnapshot(&snap);

  if (snap.laser_is_too_close)
  {
      /* 电机模块在这里通过 UART1 请求停车 */
  }

  if (snap.imu_valid && snap.imu_calibrated)
  {
      /* 可使用 snap.car_yaw_est_deg 做直线纠偏 */
  }
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
  /* USER CODE END 3 */
}
```

## 4. UART 回调区域，可选

如果电机模块使用 UART 中断，可在 `main.c` 或对应 UART 文件中加入：

```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    Board_App_UartRxCpltCallback(huart);

    /* 其他电机协议接收处理 */
}
```

传感器 I2C 读取不要放进 UART 回调或中断回调中。
