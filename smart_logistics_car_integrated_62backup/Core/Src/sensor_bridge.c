#include "sensor_bridge.h"
#include "board_app.h"

#define SENSOR_DEG_TO_RAD 0.01745329252f

#define SENSOR_FAULT_IMU_INVALID      0x00000001UL
#define SENSOR_FAULT_LASER_INVALID    0x00000002UL
#define SENSOR_FAULT_LASER_TOO_CLOSE  0x00000004UL

static void Sensor_BuildSnapshot(const SENSOR_Snapshot_t *raw, SensorSnapshot_t *snap)
{
    uint8_t warning = 0U;
    uint32_t faults = 0UL;

    if ((raw == 0) || (snap == 0)) {
        return;
    }

    if (raw->laser_warning != 0U) {
        warning = 1U;
    }
    if (raw->laser_slow != 0U) {
        warning = 2U;
    }
    if (raw->laser_too_close != 0U) {
        warning = 3U;
    }

    if (raw->imu_valid == 0U) {
        faults |= SENSOR_FAULT_IMU_INVALID;
    }
    if (raw->laser_ok == 0U) {
        faults |= SENSOR_FAULT_LASER_INVALID;
    }
    if (raw->laser_too_close != 0U) {
        faults |= SENSOR_FAULT_LASER_TOO_CLOSE;
    }

    snap->yaw_rad = raw->car_yaw_est_deg * SENSOR_DEG_TO_RAD;
    snap->gyro_z_rads = raw->gyro_yaw_dps * SENSOR_DEG_TO_RAD;
    snap->pitch_deg = raw->car_pitch_deg;
    snap->roll_deg = raw->car_roll_deg;
    snap->distance_m = (raw->laser_ok != 0U) ? ((float)raw->laser_filtered_distance_mm * 0.001f) : -1.0f;
    snap->raw_distance_mm = raw->laser_raw_distance_mm;
    snap->filtered_distance_mm = raw->laser_filtered_distance_mm;
    snap->imu_valid = (raw->imu_valid != 0U);
    snap->imu_calibrated = (raw->imu_calibrated != 0U);
    snap->laser_valid = (raw->laser_ok != 0U);
    snap->laser_warning = warning;
    snap->timestamp_ms = raw->snapshot_ms;
    snap->fault_flags = faults;
}

float JY61P_GetYawRad(void)
{
    SENSOR_Snapshot_t raw;

    Board_App_GetSensorSnapshot(&raw);
    return raw.car_yaw_est_deg * SENSOR_DEG_TO_RAD;
}

float JY61P_GetGyroZRadS(void)
{
    SENSOR_Snapshot_t raw;

    Board_App_GetSensorSnapshot(&raw);
    return raw.gyro_yaw_dps * SENSOR_DEG_TO_RAD;
}

float JY61P_GetPitchDeg(void)
{
    SENSOR_Snapshot_t raw;

    Board_App_GetSensorSnapshot(&raw);
    return raw.car_pitch_deg;
}

float JY61P_GetRollDeg(void)
{
    SENSOR_Snapshot_t raw;

    Board_App_GetSensorSnapshot(&raw);
    return raw.car_roll_deg;
}

bool JY61P_IsDataValid(void)
{
    SENSOR_Snapshot_t raw;

    Board_App_GetSensorSnapshot(&raw);
    return (raw.imu_valid != 0U);
}

bool JY61P_IsCalibrated(void)
{
    SENSOR_Snapshot_t raw;

    Board_App_GetSensorSnapshot(&raw);
    return (raw.imu_calibrated != 0U);
}

bool Laser_GetDistance(float *distance_m)
{
    SENSOR_Snapshot_t raw;

    if (distance_m == 0) {
        return false;
    }

    Board_App_GetSensorSnapshot(&raw);
    if (raw.laser_ok == 0U) {
        return false;
    }

    *distance_m = (float)raw.laser_filtered_distance_mm * 0.001f;
    return true;
}

uint16_t Laser_GetRawDistanceMm(void)
{
    SENSOR_Snapshot_t raw;

    Board_App_GetSensorSnapshot(&raw);
    return raw.laser_raw_distance_mm;
}

uint8_t Laser_GetWarningLevel(void)
{
    SensorSnapshot_t snap;

    Sensor_GetSnapshot(&snap);
    return snap.laser_warning;
}

bool Laser_IsOnline(void)
{
    SENSOR_Snapshot_t raw;

    Board_App_GetSensorSnapshot(&raw);
    return (raw.laser_ok != 0U);
}

void Sensor_GetSnapshot(SensorSnapshot_t *snap)
{
    SENSOR_Snapshot_t raw;

    if (snap == 0) {
        return;
    }

    Board_App_GetSensorSnapshot(&raw);
    Sensor_BuildSnapshot(&raw, snap);
}
