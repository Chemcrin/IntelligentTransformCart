#include "sensors.h"
#include "imu_app.h"
#include "laser_app.h"

static SENSOR_Snapshot_t g_sensor_snapshot;

static void sensor_refresh_snapshot(void)
{
    SENSOR_Snapshot_t snap;
    const ATK_MS53L1M_Device_t *laser = Laser_App_GetDevice();

    snap.car_yaw_est_deg = car_yaw_est_deg;
    snap.car_pitch_deg = car_pitch_deg;
    snap.car_roll_deg = car_roll_deg;
    snap.gyro_yaw_dps = gyro_yaw_dps;
    snap.acc_forward_g = acc_forward_g;
    snap.acc_right_g = acc_right_g;
    snap.acc_down_g = acc_down_g;
    snap.temperature_c = temperature_c;
    snap.imu_valid = imu_valid;
    snap.imu_calibrated = imu_calibrated;
    snap.imu_state = g_imu_state;
    snap.imu_error = g_imu_last_error;

    snap.laser_raw_distance_mm = g_laser_raw_distance_mm;
    snap.laser_filtered_distance_mm = g_laser_distance_mm;
    snap.laser_last_valid_distance_mm = (laser != 0) ? laser->distance.last_valid_distance_mm : 0U;
    snap.laser_ok = laser_ok;
    snap.laser_warning = laser_warning;
    snap.laser_slow = laser_slow;
    snap.laser_too_close = laser_too_close;
    snap.laser_error = g_laser_status;

    snap.imu_last_ms = g_imu_last_tick_ms;
    snap.laser_last_ms = g_laser_last_tick_ms;
    snap.snapshot_ms = HAL_GetTick();

    g_sensor_snapshot = snap;
}

void sensor_update(void)
{
    IMU_App_Loop();
    Laser_App_Loop();
    sensor_refresh_snapshot();
}

void sensor_get_snapshot(SENSOR_Snapshot_t *snapshot)
{
    uint32_t primask;

    if (snapshot == 0)
    {
        return;
    }

    primask = __get_PRIMASK();
    __disable_irq();
    *snapshot = g_sensor_snapshot;
    if (primask == 0U)
    {
        __enable_irq();
    }
}
