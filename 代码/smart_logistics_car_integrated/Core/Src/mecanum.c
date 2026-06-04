#include "mecanum.h"
#include "chassis_control.h"
#include "modbus_rtu.h"
#include "zdt_x42_modbus.h"

void Mecanum_Init(UART_HandleTypeDef *motor_uart)
{
    Modbus_Init(motor_uart);
    ChassisControl_Init();
}

MecanumResult_t Mecanum_SetVelocity(float vx_mps, float vy_mps, float wz_radps)
{
    MecanumVelocity_t vel;

    vel.vx_mps = vx_mps;
    vel.vy_mps = vy_mps;
    vel.wz_radps = wz_radps;

    return ChassisControl_SetVelocity(&vel);
}

MecanumResult_t Mecanum_MoveForward(float speed_mps)
{
    return Mecanum_SetVelocity(0.0f, speed_mps, 0.0f);
}

MecanumResult_t Mecanum_MoveBackward(float speed_mps)
{
    return Mecanum_SetVelocity(0.0f, -speed_mps, 0.0f);
}

MecanumResult_t Mecanum_MoveLeft(float speed_mps)
{
    return Mecanum_SetVelocity(speed_mps, 0.0f, 0.0f);
}

MecanumResult_t Mecanum_MoveRight(float speed_mps)
{
    return Mecanum_SetVelocity(-speed_mps, 0.0f, 0.0f);
}

MecanumResult_t Mecanum_MoveVector(float vx_mps, float vy_mps)
{
    return Mecanum_SetVelocity(vx_mps, vy_mps, 0.0f);
}

MecanumResult_t Mecanum_Rotate(float wz_radps)
{
    return Mecanum_SetVelocity(0.0f, 0.0f, wz_radps);
}

MecanumResult_t Mecanum_Stop(void)
{
    return ChassisControl_Stop();
}

MecanumResult_t Mecanum_EmergencyStop(void)
{
    return ChassisControl_EmergencyStop();
}

MecanumResult_t Mecanum_ClearEmergencyStop(void)
{
    return ChassisControl_ClearEmergencyStop();
}

MecanumResult_t Mecanum_ClearFault(void)
{
    return ChassisControl_ClearFault();
}

void Mecanum_PeriodicTask(void)
{
    ChassisControl_PeriodicTask();
}

MecanumState_t Mecanum_GetState(void)
{
    return ChassisControl_GetState();
}

MecanumVelocity_t Mecanum_GetTargetVelocity(void)
{
    return ChassisControl_GetTargetVelocity();
}

MecanumResult_t Mecanum_TestMoveForward(float speed_mps)
{
    return Mecanum_MoveForward(speed_mps);
}

MecanumResult_t Mecanum_TestMoveLeft(float speed_mps)
{
    return Mecanum_MoveLeft(speed_mps);
}

MecanumResult_t Mecanum_TestRotate(float wz_radps)
{
    return Mecanum_Rotate(wz_radps);
}

MecanumResult_t RS485_TestDirectionLevel(uint8_t motor_addr, float *rpm_out)
{
    return ZDTX42_TestReadSpeed(motor_addr, rpm_out);
}
