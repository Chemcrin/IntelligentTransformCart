#include "main.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"
#include "board_app.h"
#include "sensor_bridge.h"
#include "mecanum.h"
#include "chassis_control.h"

#define APP_LOOP_PERIOD_MS          10U
#define APP_TEST_SPEED_MPS          0.12f
#define APP_TEST_ROTATE_RADPS       0.25f
#define APP_LASER_STOP_LEVEL        3U

void SystemClock_Config(void);

static void App_RunMotionDemo(uint32_t elapsed_ms)
{
    if (elapsed_ms < 3000U) {
        (void)Mecanum_MoveForward(APP_TEST_SPEED_MPS);
    } else if (elapsed_ms < 5000U) {
        (void)Mecanum_Stop();
    } else if (elapsed_ms < 8000U) {
        (void)Mecanum_MoveBackward(APP_TEST_SPEED_MPS);
    } else if (elapsed_ms < 10000U) {
        (void)Mecanum_Stop();
    } else if (elapsed_ms < 13000U) {
        (void)Mecanum_MoveLeft(APP_TEST_SPEED_MPS);
    } else if (elapsed_ms < 15000U) {
        (void)Mecanum_Stop();
    } else if (elapsed_ms < 18000U) {
        (void)Mecanum_MoveRight(APP_TEST_SPEED_MPS);
    } else if (elapsed_ms < 20000U) {
        (void)Mecanum_Stop();
    } else if (elapsed_ms < 23000U) {
        (void)Mecanum_Rotate(APP_TEST_ROTATE_RADPS);
    } else {
        (void)Mecanum_Stop();
    }
}

int main(void)
{
    uint32_t demo_start_ms;

    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART3_UART_Init();

    Board_App_Init();
    Mecanum_Init(&huart3);
    demo_start_ms = HAL_GetTick();

    while (1) {
        SensorSnapshot_t snap;

        Board_App_Loop();
        Sensor_GetSnapshot(&snap);

        if ((snap.laser_valid == false) || (snap.laser_warning >= APP_LASER_STOP_LEVEL)) {
            ChassisControl_SetExternalEmergency(1U);
        } else {
            ChassisControl_SetExternalEmergency(0U);
            (void)ChassisControl_ClearEmergencyStop();
            App_RunMotionDemo(HAL_GetTick() - demo_start_ms);
        }

        Mecanum_PeriodicTask();
        HAL_Delay(APP_LOOP_PERIOD_MS);
    }
}
