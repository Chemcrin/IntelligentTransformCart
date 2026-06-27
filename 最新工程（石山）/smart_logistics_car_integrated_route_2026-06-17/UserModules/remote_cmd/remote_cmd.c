#include "remote_cmd.h"

#include "board_app.h"
#include "chassis_control.h"
#include "imu_app.h"
#include "laser_app.h"
#include "mecanum.h"
#include "sensor_bridge.h"
#include "straight_control.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define REMOTE_CMD_TX_TIMEOUT_MS           80U
#define REMOTE_CMD_EPSILON                 0.0001f

static UART_HandleTypeDef *g_remote_uart = NULL;
static uint8_t g_rx_byte = 0U;
static char g_rx_build[REMOTE_CMD_LINE_MAX];
static volatile uint8_t g_rx_build_len = 0U;
static char g_rx_ready[REMOTE_CMD_LINE_MAX];
static volatile uint8_t g_rx_line_ready = 0U;

static RemoteCmdStatus_t g_status;
static uint8_t g_laser_lock = 0U;
static uint8_t g_timeout_lock = 0U;

static uint8_t str_eq(const char *a, const char *b)
{
    return (strcmp(a, b) == 0) ? 1U : 0U;
}

static int32_t scale_to_i32(float value, float scale)
{
    float scaled = value * scale;

    if (scaled >= 0.0f) {
        return (int32_t)(scaled + 0.5f);
    }

    return (int32_t)(scaled - 0.5f);
}

static void milli_to_text(float value, char *out, size_t out_size)
{
    int32_t milli = scale_to_i32(value, 1000.0f);
    uint32_t abs_milli;
    const char *sign = "";

    if ((out == NULL) || (out_size == 0U)) {
        return;
    }

    if (milli < 0) {
        sign = "-";
        abs_milli = (uint32_t)(-milli);
    } else {
        abs_milli = (uint32_t)milli;
    }

    (void)snprintf(out,
                   out_size,
                   "%s%lu.%03lu",
                   sign,
                   (unsigned long)(abs_milli / 1000UL),
                   (unsigned long)(abs_milli % 1000UL));
}

static uint8_t parse_float_token(const char **cursor, float *out)
{
    const char *p;
    float sign = 1.0f;
    float value = 0.0f;
    float frac_base = 0.1f;
    uint8_t has_digit = 0U;

    if ((cursor == NULL) || (*cursor == NULL) || (out == NULL)) {
        return 0U;
    }

    p = *cursor;
    while ((*p == ' ') || (*p == '\t')) {
        p++;
    }

    if (*p == '-') {
        sign = -1.0f;
        p++;
    } else if (*p == '+') {
        p++;
    }

    while ((*p >= '0') && (*p <= '9')) {
        value = (value * 10.0f) + (float)(*p - '0');
        has_digit = 1U;
        p++;
    }

    if (*p == '.') {
        p++;
        while ((*p >= '0') && (*p <= '9')) {
            value += (float)(*p - '0') * frac_base;
            frac_base *= 0.1f;
            has_digit = 1U;
            p++;
        }
    }

    if (has_digit == 0U) {
        return 0U;
    }

    *out = value * sign;
    *cursor = p;
    return 1U;
}

static void remote_send_text(const char *text)
{
    if ((g_remote_uart == NULL) || (text == NULL)) {
        return;
    }

    (void)HAL_UART_Transmit(g_remote_uart,
                            (uint8_t *)text,
                            (uint16_t)strlen(text),
                            REMOTE_CMD_TX_TIMEOUT_MS);
}

static const char *state_to_text(MecanumState_t state)
{
    switch (state) {
    case MECANUM_STATE_IDLE:
        return "IDLE";
    case MECANUM_STATE_READY:
        return "READY";
    case MECANUM_STATE_RUNNING:
        return "RUN";
    case MECANUM_STATE_STOP:
        return "STOP";
    case MECANUM_STATE_FAULT:
        return "FAULT";
    case MECANUM_STATE_EMERGENCY_STOP:
        return "ESTOP";
    default:
        return "UNKNOWN";
    }
}

static void remote_send_status(const SensorSnapshot_t *snap, uint32_t now_ms)
{
    char tx[360];
    char yaw[18];
    char pitch[18];
    char roll[18];
    char gyro_z[18];
    char acc_f[18];
    char acc_r[18];
    char acc_d[18];
    int32_t vx_mmps;
    int32_t vy_mmps;
    int32_t wz_mradps;
    int len;

    g_status.chassis_state = Mecanum_GetState();
    g_status.target_velocity = Mecanum_GetTargetVelocity();
    g_status.last_status_ms = now_ms;

    vx_mmps = scale_to_i32(g_status.target_velocity.vx_mps, 1000.0f);
    vy_mmps = scale_to_i32(g_status.target_velocity.vy_mps, 1000.0f);
    wz_mradps = scale_to_i32(g_status.target_velocity.wz_radps, 1000.0f);

    milli_to_text(car_yaw_est_deg, yaw, sizeof(yaw));
    milli_to_text(car_pitch_deg, pitch, sizeof(pitch));
    milli_to_text(car_roll_deg, roll, sizeof(roll));
    milli_to_text(gyro_yaw_dps, gyro_z, sizeof(gyro_z));
    milli_to_text(acc_forward_g, acc_f, sizeof(acc_f));
    milli_to_text(acc_right_g, acc_r, sizeof(acc_r));
    milli_to_text(acc_down_g, acc_d, sizeof(acc_d));

    len = snprintf(tx,
                   sizeof(tx),
                   "STAT t=%lu link=%u hb_to=%u laser_estop=%u laser_fault=%u laser_ok=%u laser_warn=%u state=%s vx=%ld vy=%ld wz=%ld dist=%u raw=%u imu_valid=%u imu_cal=%u yaw=%s pitch=%s roll=%s gyro_z=%s acc_f=%s acc_r=%s acc_d=%s fault=0x%08lX sc=%u\r\n",
                   (unsigned long)now_ms,
                   (unsigned int)g_status.link_state,
                   (unsigned int)g_status.heartbeat_timeout,
                   (unsigned int)g_status.laser_emergency,
                   (unsigned int)g_status.laser_fault,
                   (unsigned int)((snap != NULL) ? snap->laser_valid : 0U),
                   (unsigned int)((snap != NULL) ? snap->laser_warning : 0U),
                   state_to_text(g_status.chassis_state),
                   (long)vx_mmps,
                   (long)vy_mmps,
                   (long)wz_mradps,
                   (unsigned int)((snap != NULL) ? snap->filtered_distance_mm : 0U),
                   (unsigned int)((snap != NULL) ? snap->raw_distance_mm : 0U),
                   (unsigned int)((snap != NULL) ? snap->imu_valid : 0U),
                   (unsigned int)((snap != NULL) ? snap->imu_calibrated : 0U),
                   yaw,
                   pitch,
                   roll,
                   gyro_z,
                   acc_f,
                   acc_r,
                   acc_d,
                   (unsigned long)((snap != NULL) ? snap->fault_flags : 0UL),
                   (unsigned int)Mecanum_IsStraightControlEnabled());

    if (len > 0) {
        remote_send_text(tx);
    }
}

static void remote_send_imu(uint32_t now_ms)
{
    char tx[360];
    char ax[18];
    char ay[18];
    char az[18];
    char gx[18];
    char gy[18];
    char gz[18];
    char roll[18];
    char pitch[18];
    char yaw[18];
    char car_yaw[18];
    char temp[18];
    int len;

    milli_to_text(g_imu_ax_g, ax, sizeof(ax));
    milli_to_text(g_imu_ay_g, ay, sizeof(ay));
    milli_to_text(g_imu_az_g, az, sizeof(az));
    milli_to_text(g_imu_gx_dps, gx, sizeof(gx));
    milli_to_text(g_imu_gy_dps, gy, sizeof(gy));
    milli_to_text(g_imu_gz_dps, gz, sizeof(gz));
    milli_to_text(g_imu_roll_deg, roll, sizeof(roll));
    milli_to_text(g_imu_pitch_deg, pitch, sizeof(pitch));
    milli_to_text(g_imu_yaw_deg, yaw, sizeof(yaw));
    milli_to_text(car_yaw_est_deg, car_yaw, sizeof(car_yaw));
    milli_to_text(g_imu_temp_c, temp, sizeof(temp));

    len = snprintf(tx,
                   sizeof(tx),
                   "IMU t=%lu valid=%u cal=%u ax=%s ay=%s az=%s gx=%s gy=%s gz=%s roll=%s pitch=%s yaw=%s car_yaw=%s temp=%s err=%u count=%lu\r\n",
                   (unsigned long)now_ms,
                   (unsigned int)imu_valid,
                   (unsigned int)imu_calibrated,
                   ax,
                   ay,
                   az,
                   gx,
                   gy,
                   gz,
                   roll,
                   pitch,
                   yaw,
                   car_yaw,
                   temp,
                   (unsigned int)g_imu_last_error,
                   (unsigned long)g_imu_update_count);

    if (len > 0) {
        remote_send_text(tx);
    }
}

static void remote_send_laser(uint32_t now_ms)
{
    SENSOR_Snapshot_t raw;
    const ATK_MS53L1M_Device_t *laser = Laser_App_GetDevice();
    char tx[260];
    int len;

    Board_App_GetSensorSnapshot(&raw);
    len = snprintf(tx,
                   sizeof(tx),
                   "LASER t=%lu ok=%u valid=%u raw=%u filt=%u last=%u warn=%u slow=%u stop=%u err=%u samples=%lu last_ms=%lu\r\n",
                   (unsigned long)now_ms,
                   (unsigned int)raw.laser_ok,
                   (unsigned int)((laser != NULL) ? laser->distance.valid : 0U),
                   (unsigned int)raw.laser_raw_distance_mm,
                   (unsigned int)raw.laser_filtered_distance_mm,
                   (unsigned int)raw.laser_last_valid_distance_mm,
                   (unsigned int)raw.laser_warning,
                   (unsigned int)raw.laser_slow,
                   (unsigned int)raw.laser_too_close,
                   (unsigned int)raw.laser_error,
                   (unsigned long)g_laser_sample_count,
                   (unsigned long)raw.laser_last_ms);

    if (len > 0) {
        remote_send_text(tx);
    }
}

static void remote_send_sensors(const SensorSnapshot_t *snap, uint32_t now_ms)
{
    char tx[360];
    char yaw[18];
    char gyro[18];
    char pitch[18];
    char roll[18];
    char ax[18];
    char ay[18];
    char az[18];
    int len;

    milli_to_text(car_yaw_est_deg, yaw, sizeof(yaw));
    milli_to_text(gyro_yaw_dps, gyro, sizeof(gyro));
    milli_to_text(car_pitch_deg, pitch, sizeof(pitch));
    milli_to_text(car_roll_deg, roll, sizeof(roll));
    milli_to_text(g_imu_ax_g, ax, sizeof(ax));
    milli_to_text(g_imu_ay_g, ay, sizeof(ay));
    milli_to_text(g_imu_az_g, az, sizeof(az));

    len = snprintf(tx,
                   sizeof(tx),
                   "SENS t=%lu imu_valid=%u imu_cal=%u yaw=%s gyro_z=%s pitch=%s roll=%s ax=%s ay=%s az=%s laser_ok=%u laser_warn=%u raw=%u filt=%u fault=0x%08lX\r\n",
                   (unsigned long)now_ms,
                   (unsigned int)((snap != NULL) ? snap->imu_valid : 0U),
                   (unsigned int)((snap != NULL) ? snap->imu_calibrated : 0U),
                   yaw,
                   gyro,
                   pitch,
                   roll,
                   ax,
                   ay,
                   az,
                   (unsigned int)((snap != NULL) ? snap->laser_valid : 0U),
                   (unsigned int)((snap != NULL) ? snap->laser_warning : 0U),
                   (unsigned int)((snap != NULL) ? snap->raw_distance_mm : 0U),
                   (unsigned int)((snap != NULL) ? snap->filtered_distance_mm : 0U),
                   (unsigned long)((snap != NULL) ? snap->fault_flags : 0UL));

    if (len > 0) {
        remote_send_text(tx);
    }
}

static void remote_send_straight_status(uint32_t now_ms)
{
    StraightControlStatus_t sc = StraightControl_GetStatus();
    char tx[260];
    char target_yaw[18];
    char err[18];
    char out[18];
    char left[18];
    char right[18];
    int len;

    milli_to_text(sc.target_yaw_rad, target_yaw, sizeof(target_yaw));
    milli_to_text(sc.yaw_error_rad, err, sizeof(err));
    milli_to_text(sc.output_wz_radps, out, sizeof(out));
    milli_to_text(sc.left_rpm_avg, left, sizeof(left));
    milli_to_text(sc.right_rpm_avg, right, sizeof(right));

    len = snprintf(tx,
                   sizeof(tx),
                   "SC t=%lu en=%u active=%u imu=%u fb=%u target_yaw_rad=%s yaw_err_rad=%s wz_corr=%s left_rpm=%s right_rpm=%s\r\n",
                   (unsigned long)now_ms,
                   (unsigned int)sc.enabled,
                   (unsigned int)sc.active,
                   (unsigned int)sc.using_imu,
                   (unsigned int)sc.using_motor_feedback,
                   target_yaw,
                   err,
                   out,
                   left,
                   right);

    if (len > 0) {
        remote_send_text(tx);
    }
}

static void mark_valid_command(uint32_t now_ms)
{
    g_status.last_rx_ms = now_ms;
    g_status.last_valid_cmd_ms = now_ms;
    g_status.link_state = REMOTE_CMD_LINK_ALIVE;
    g_status.heartbeat_timeout = 0U;
}

static void remote_apply_safety(const SensorSnapshot_t *snap, uint32_t now_ms)
{
    (void)snap;
    (void)now_ms;

    g_laser_lock = 0U;
    g_timeout_lock = 0U;
    g_status.laser_fault = 0U;
    g_status.laser_emergency = 0U;
    g_status.heartbeat_timeout = 0U;
    if (g_status.link_state == REMOTE_CMD_LINK_TIMEOUT) {
        g_status.link_state = REMOTE_CMD_LINK_ALIVE;
    }
    ChassisControl_SetExternalEmergency(0U);
}

static void remote_handle_line(char *line, const SensorSnapshot_t *snap, uint32_t now_ms)
{
    char *cmd;
    char *args;

    if (line == NULL) {
        return;
    }

    cmd = line;
    while ((*cmd == ' ') || (*cmd == '\t')) {
        cmd++;
    }
    if (*cmd == '\0') {
        return;
    }

    args = cmd;
    while ((*args != '\0') && (*args != ' ') && (*args != '\t')) {
        args++;
    }
    if (*args != '\0') {
        *args = '\0';
        args++;
        while ((*args == ' ') || (*args == '\t')) {
            args++;
        }
    } else {
        args = NULL;
    }

    if (str_eq(cmd, "HB") != 0U) {
        mark_valid_command(now_ms);
        remote_send_text("ACK HB\r\n");
        return;
    }

    if (str_eq(cmd, "STOP") != 0U) {
        mark_valid_command(now_ms);
        (void)Mecanum_Stop();
        remote_send_text("ACK STOP\r\n");
        return;
    }

    if (str_eq(cmd, "ESTOP") != 0U) {
        mark_valid_command(now_ms);
        g_timeout_lock = 0U;
        ChassisControl_SetExternalEmergency(0U);
        (void)Mecanum_Stop();
        remote_send_text("ACK ESTOP\r\n");
        return;
    }

    if ((str_eq(cmd, "CLR") != 0U) || (str_eq(cmd, "CLEAR") != 0U)) {
        mark_valid_command(now_ms);
        g_laser_lock = 0U;
        g_timeout_lock = 0U;
        g_status.laser_emergency = 0U;
        g_status.heartbeat_timeout = 0U;
        ChassisControl_SetExternalEmergency(0U);
        (void)Mecanum_ClearEmergencyStop();
        (void)Mecanum_ClearFault();
        (void)Mecanum_Stop();
        remote_send_text("ACK CLR\r\n");
        return;
    }

    if (str_eq(cmd, "ZERO") != 0U) {
        mark_valid_command(now_ms);
        (void)Mecanum_Stop();
        if (Board_App_ZeroYaw() == HAL_OK) {
            remote_send_text("ACK ZERO\r\n");
        } else {
            remote_send_text("ERR ZERO\r\n");
        }
        return;
    }

    if (str_eq(cmd, "GYROCAL") != 0U) {
        mark_valid_command(now_ms);
        (void)Mecanum_Stop();
        if (Board_App_StartImuGyroCalibration() == HAL_OK) {
            mark_valid_command(HAL_GetTick());
            remote_send_text("ACK GYROCAL\r\n");
        } else {
            remote_send_text("ERR GYROCAL\r\n");
        }
        return;
    }

    if (str_eq(cmd, "ACCCAL") != 0U) {
        mark_valid_command(now_ms);
        (void)Mecanum_Stop();
        if (Board_App_CalibrateImuAccelerometer() == HAL_OK) {
            mark_valid_command(HAL_GetTick());
            remote_send_text("ACK ACCCAL\r\n");
        } else {
            remote_send_text("ERR ACCCAL\r\n");
        }
        return;
    }

    if ((str_eq(cmd, "STAT?") != 0U) || (str_eq(cmd, "STATUS?") != 0U)) {
        mark_valid_command(now_ms);
        remote_send_status(snap, now_ms);
        return;
    }

    if (str_eq(cmd, "IMU?") != 0U) {
        mark_valid_command(now_ms);
        remote_send_imu(now_ms);
        return;
    }

    if (str_eq(cmd, "LASER?") != 0U) {
        mark_valid_command(now_ms);
        remote_send_laser(now_ms);
        return;
    }

    if (str_eq(cmd, "SENS?") != 0U) {
        mark_valid_command(now_ms);
        remote_send_sensors(snap, now_ms);
        return;
    }

    if (str_eq(cmd, "SENS_STREAM") != 0U) {
        mark_valid_command(now_ms);
        if ((args != NULL) && (str_eq(args, "ON") != 0U)) {
            g_status.sensor_stream_enable = 1U;
            remote_send_text("ACK SENS_STREAM ON\r\n");
        } else if ((args != NULL) && (str_eq(args, "OFF") != 0U)) {
            g_status.sensor_stream_enable = 0U;
            remote_send_text("ACK SENS_STREAM OFF\r\n");
        } else {
            remote_send_text("ERR SENS_STREAM FORMAT\r\n");
        }
        return;
    }

    if (str_eq(cmd, "SC") != 0U) {
        mark_valid_command(now_ms);
        if ((args != NULL) && (str_eq(args, "ON") != 0U)) {
            Mecanum_EnableStraightControl(1U);
            remote_send_text("ACK SC ON\r\n");
        } else if ((args != NULL) && (str_eq(args, "OFF") != 0U)) {
            Mecanum_EnableStraightControl(0U);
            remote_send_text("ACK SC OFF\r\n");
        } else if ((args != NULL) && (str_eq(args, "?") != 0U)) {
            remote_send_straight_status(now_ms);
        } else {
            remote_send_text("ERR SC FORMAT\r\n");
        }
        return;
    }

    if (str_eq(cmd, "VEL") != 0U) {
        const char *p = args;
        float vx;
        float vy;
        float wz;

        mark_valid_command(now_ms);
        if ((parse_float_token(&p, &vx) == 0U) ||
            (parse_float_token(&p, &vy) == 0U) ||
            (parse_float_token(&p, &wz) == 0U)) {
            g_status.parse_error_count++;
            remote_send_text("ERR VEL FORMAT\r\n");
            return;
        }

        if ((vx < REMOTE_CMD_EPSILON) && (vx > -REMOTE_CMD_EPSILON) &&
            (vy < REMOTE_CMD_EPSILON) && (vy > -REMOTE_CMD_EPSILON) &&
            (wz < REMOTE_CMD_EPSILON) && (wz > -REMOTE_CMD_EPSILON)) {
            (void)Mecanum_Stop();
        } else {
            (void)Mecanum_SetVelocity(vx, vy, wz);
        }
        remote_send_text("ACK VEL\r\n");
        return;
    }

    g_status.parse_error_count++;
    remote_send_text("ERR UNKNOWN\r\n");
}

void RemoteCmd_Init(UART_HandleTypeDef *huart)
{
    uint32_t now_ms = HAL_GetTick();

    g_remote_uart = huart;
    g_rx_build_len = 0U;
    g_rx_line_ready = 0U;
    g_laser_lock = 0U;
    g_timeout_lock = 0U;
    memset(&g_status, 0, sizeof(g_status));
    g_status.link_state = REMOTE_CMD_LINK_ALIVE;
    g_status.last_rx_ms = now_ms;
    g_status.last_valid_cmd_ms = now_ms;
    g_status.last_status_ms = now_ms;
    g_status.last_sensor_stream_ms = now_ms;

    if (g_remote_uart != NULL) {
        (void)HAL_UART_Receive_IT(g_remote_uart, &g_rx_byte, 1U);
    }
}

void RemoteCmd_Loop(void)
{
    SensorSnapshot_t snap;
    uint32_t now_ms = HAL_GetTick();

    Sensor_GetSnapshot(&snap);
    remote_apply_safety(&snap, now_ms);

    if (g_rx_line_ready != 0U) {
        char line[REMOTE_CMD_LINE_MAX];
        uint32_t primask = __get_PRIMASK();

        __disable_irq();
        strncpy(line, g_rx_ready, sizeof(line));
        line[sizeof(line) - 1U] = '\0';
        g_rx_line_ready = 0U;
        if (primask == 0U) {
            __enable_irq();
        }

        remote_handle_line(line, &snap, now_ms);
        remote_apply_safety(&snap, HAL_GetTick());
    }

    now_ms = HAL_GetTick();
#if (REMOTE_CMD_STATUS_PERIOD_MS > 0U)
    if ((now_ms - g_status.last_status_ms) >= REMOTE_CMD_STATUS_PERIOD_MS) {
        remote_send_status(&snap, now_ms);
    }
#endif

    if ((g_status.sensor_stream_enable != 0U) &&
        ((now_ms - g_status.last_sensor_stream_ms) >= REMOTE_CMD_SENSOR_STREAM_PERIOD_MS)) {
        g_status.last_sensor_stream_ms = now_ms;
        remote_send_sensors(&snap, now_ms);
    }
}

void RemoteCmd_UartRxCpltCallback(UART_HandleTypeDef *huart)
{
    uint8_t ch;

    if ((g_remote_uart == NULL) || (huart != g_remote_uart)) {
        return;
    }

    ch = g_rx_byte;
    if ((ch == '\n') || (ch == '\r')) {
        if (g_rx_build_len > 0U) {
            if (g_rx_line_ready == 0U) {
                uint8_t i;

                for (i = 0U; i < g_rx_build_len; i++) {
                    g_rx_ready[i] = g_rx_build[i];
                }
                g_rx_ready[g_rx_build_len] = '\0';
                g_rx_line_ready = 1U;
            } else {
                g_status.rx_overrun_count++;
            }
            g_rx_build_len = 0U;
        }
    } else if (g_rx_build_len < (REMOTE_CMD_LINE_MAX - 1U)) {
        g_rx_build[g_rx_build_len] = (char)ch;
        g_rx_build_len++;
    } else {
        g_rx_build_len = 0U;
        g_status.rx_overrun_count++;
    }

    (void)HAL_UART_Receive_IT(g_remote_uart, &g_rx_byte, 1U);
}

RemoteCmdStatus_t RemoteCmd_GetStatus(void)
{
    return g_status;
}





