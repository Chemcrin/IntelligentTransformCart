#include "zdt_x42_modbus.h"
#include "modbus_rtu.h"
#include "mecanum_config.h"
#include "mecanum_kinematics.h"

static uint16_t ZDTX42_RpmToSpeedReg(float rpm_abs)
{
    float scaled = rpm_abs * ZDT_SPEED_MODE_SCALE + 0.5f;

    if (scaled < 0.0f) {
        scaled = 0.0f;
    }
    if (scaled > 65535.0f) {
        scaled = 65535.0f;
    }

    return (uint16_t)scaled;
}

void ZDTX42_Init(void)
{
}

MecanumResult_t ZDTX42_EnableMotor(uint8_t motor_id, uint8_t enable)
{
    uint16_t regs[2];

    regs[0] = (enable != 0u) ? ZDT_ENABLE_CODE : ZDT_DISABLE_CODE;
    regs[1] = ZDT_SYNC_NOW_FLAG;

    return Modbus_WriteMultipleRegisters(motor_id, ZDT_REG_ENABLE, regs, 2u, motor_id != MOTOR_ADDR_BROAD);
}

MecanumResult_t ZDTX42_SetMotorSpeedRpm(uint8_t motor_id, float rpm_signed, uint8_t sync_wait)
{
    uint16_t regs[4];
    float rpm_abs = rpm_signed;

    if (rpm_abs >= 0.0f) {
        regs[0] = ZDT_DIR_CW;
    } else {
        regs[0] = ZDT_DIR_CCW;
        rpm_abs = -rpm_abs;
    }

    regs[1] = ZDT_DEFAULT_ACCEL_RPM_S;
    regs[2] = ZDTX42_RpmToSpeedReg(rpm_abs);
    regs[3] = (sync_wait != 0u) ? ZDT_SYNC_WAIT_FLAG : ZDT_SYNC_NOW_FLAG;

    return Modbus_WriteMultipleRegisters(motor_id, ZDT_REG_SPEED_MODE, regs, 4u, motor_id != MOTOR_ADDR_BROAD);
}

MecanumResult_t ZDTX42_SetMotorSpeed(uint8_t motor_id, float speed_mps)
{
    return ZDTX42_SetMotorSpeedRpm(motor_id, Mecanum_WheelMpsToRpm(speed_mps), 0u);
}

MecanumResult_t ZDTX42_StopMotor(uint8_t motor_id)
{
    uint16_t reg = ZDT_STOP_CODE;

    return Modbus_WriteMultipleRegisters(motor_id, ZDT_REG_STOP_IMMEDIATE, &reg, 1u, motor_id != MOTOR_ADDR_BROAD);
}

MecanumResult_t ZDTX42_StopAllMotors(void)
{
    MecanumResult_t ret;

    ret = ZDTX42_SetMotorSpeedRpm(MOTOR_ADDR_FL, 0.0f, 1u);
    if (ret != MECANUM_OK) return ret;
    ret = ZDTX42_SetMotorSpeedRpm(MOTOR_ADDR_FR, 0.0f, 1u);
    if (ret != MECANUM_OK) return ret;
    ret = ZDTX42_SetMotorSpeedRpm(MOTOR_ADDR_RL, 0.0f, 1u);
    if (ret != MECANUM_OK) return ret;
    ret = ZDTX42_SetMotorSpeedRpm(MOTOR_ADDR_RR, 0.0f, 1u);
    if (ret != MECANUM_OK) return ret;

    return ZDTX42_SyncTrigger();
}

MecanumResult_t ZDTX42_EmergencyStopAll(void)
{
    return ZDTX42_StopMotor(MOTOR_ADDR_BROAD);
}

MecanumResult_t ZDTX42_SyncTrigger(void)
{
    uint16_t reg = ZDT_SYNC_TRIGGER_CODE;

    return Modbus_WriteMultipleRegisters(MOTOR_ADDR_BROAD, ZDT_REG_SYNC_TRIGGER, &reg, 1u, 0u);
}

MecanumResult_t ZDTX42_ReadMotorSpeed(uint8_t motor_id, float *rpm_out)
{
    uint16_t regs[2];
    MecanumResult_t ret;
    int sign;

    if (rpm_out == 0) {
        return MECANUM_BAD_PARAM;
    }

    ret = Modbus_ReadInputRegisters(motor_id, ZDT_REG_SPEED_FEEDBACK, 2u, regs);
    if (ret != MECANUM_OK) {
        return ret;
    }

    if ((regs[0] != 0u) && (regs[0] != 1u)) {
        return MECANUM_ERROR;
    }

    sign = (regs[0] == 0u) ? 1 : -1;
    *rpm_out = ((float)sign * (float)regs[1]) / ZDT_SPEED_FEEDBACK_SCALE;

    return MECANUM_OK;
}

MecanumResult_t ZDTX42_TestSingleMotor(uint8_t addr, float rpm)
{
    return ZDTX42_SetMotorSpeedRpm(addr, rpm, 0u);
}

MecanumResult_t ZDTX42_TestReadSpeed(uint8_t addr, float *rpm_out)
{
    return ZDTX42_ReadMotorSpeed(addr, rpm_out);
}
