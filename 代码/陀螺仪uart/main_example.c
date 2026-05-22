/**
 * main.c — JY61P 陀螺仪 UART 读取示例
 *
 * 硬件: STM32F103C8T6
 *   PB10 (USART3_TX) → JY61P-RX
 *   PB11 (USART3_RX) → JY61P-TX
 *
 * CubeMX 配置:
 *   1) USART3: PB10=TX, PB11=RX, 115200-8-N-1, NVIC 开启中断
 *   2) USART1: PA9=TX, 115200 (用于 printf)
 *   3) SysTick: 必须开启
 *
 * 集成步骤:
 *   1) jy61p.h → Inc/,  jy61p.c → Src/
 *   2) main.c 中 #include "jy61p.h"
 *   3) 将下方 HAL_UART_RxCpltCallback 复制到 main.c
 *   4) main() 中调用 JY61P_StartRecv(&huart3)
 */

#include "main.h"
#include "jy61p.h"
#include <stdio.h>

extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart1;


/* ================================================================
 * 务必在 main.c 中加入此回调 (如果 CubeMX 未生成)
 * ================================================================ */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3) {
        static uint8_t ch;
        HAL_UART_Receive_IT(huart, &ch, 1);   /* 重新启动接收 */
        JY61P_FeedByte(ch);                   /* 喂给解析器 */
    }

    /* 其他 UART 中断在此处理 (如 USART1 空闲中断等) */
}


/* ================================================================
 * printf 重定向到 USART1 (调试用)
 * ================================================================ */
#ifdef __GNUC__
int _write(int fd, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}
#else
int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
#endif


/* ================================================================
 * 主函数
 * ================================================================ */
int main(void)
{
    JY61P_Data imu;

    HAL_Init();
    SystemClock_Config();       /* CubeMX 生成 */

    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_USART3_UART_Init();      /* CubeMX 生成 */

    JY61P_StartRecv(&huart3);   /* 启动串口中断接收 */

    printf("JY61P UART Demo\r\n");

    /* ---- 可选初始配置 ---- */

    /* 加速度计校准: 模块水平静置, 调用后等 3 秒 */
    JY61P_Calibrate(&huart3);
    HAL_Delay(3000);

    /* Z 轴归零 */
    JY61P_ZeroYaw(&huart3);
    HAL_Delay(500);

    /* ---- 主循环 ---- */
    while (1)
    {
        /* 方式1: 读取全部数据 */
        if (JY61P_Ready()) {
            JY61P_GetData(&imu);
            printf("R:%6.1f  P:%6.1f  Y:%6.1f | "
                   "A:%5.2f %5.2f %5.2f | "
                   "G:%7.1f %7.1f %7.1f | T:%.1fC\r\n",
                   imu.roll, imu.pitch, imu.yaw,
                   imu.ax, imu.ay, imu.az,
                   imu.gx, imu.gy, imu.gz,
                   imu.temp);
        }

        /* 方式2: 仅取姿态角 */
#if 0
        float roll, pitch, yaw;
        if (JY61P_GetAngle(&roll, &pitch, &yaw)) {
            printf("R:%.1f P:%.1f Y:%.1f\r\n", roll, pitch, yaw);
        }
#endif

        HAL_Delay(10);  /* 配合模块 100Hz 输出 */
    }
}


/* ================================================================
 * 参考: 角度转向控制
 * ================================================================ */
#if 0
void TurnToAngle(float target)
{
    float yaw, err;
    const float KP = 1.0f;

    JY61P_ZeroYaw(&huart3);
    HAL_Delay(500);

    while (1) {
        if (JY61P_GetAngle(NULL, NULL, &yaw)) {
            err = target - yaw;
            if (err >  180.0f) err -= 360.0f;
            if (err < -180.0f) err += 360.0f;

            /* set_motor(err * KP); */

            if (err < 2.0f && err > -2.0f) break;
        }
        HAL_Delay(20);
    }
}
#endif
