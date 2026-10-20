/* Proto — proto_state.c — STATE: coherent snapshots per target.
 *
 * Routed by `target`: device, focuser, fan, environment, vibration, buttons,
 * operation. Real targets (device/focuser) read board/motors/tmc; the rest
 * read the mock modules.
 */
#include "proto_internal.h"

#include <string.h>

#include "esp_system.h"

#include "accelerometer.h"
#include "buttons.h"
#include "environment.h"
#include "fan.h"
#include "sht31.h"
#include "vibration.h"

static void get_session(const char *req, char *out, size_t cap) {
    proto_json_get_string(req, "session", out, cap);
}

static const char *reset_reason_str(void) {
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON: return "power_on";
        case ESP_RST_SW:      return "software";
        case ESP_RST_PANIC:   return "panic";
        case ESP_RST_BROWNOUT:return "brownout";
        case ESP_RST_WDT:
        case ESP_RST_INT_WDT:
        case ESP_RST_TASK_WDT:return "watchdog";
        case ESP_RST_USB:     return "usb";
        default:              return "unknown";
    }
}

/* ── state / device ────────────────────────────────────────── */

static void state_device(char *out, size_t cap, uint32_t id, const char *session) {
    MotorsState ms = motors_current_state();
    bool fault = motors_status_is_error(ms.status) || !tmc2209_is_initialized();
    bool power = board_has_external_power();
    bool sensor_fault = !accelerometer_is_present() || !sht31_is_present();

    size_t len = 0;
    out[0] = '\0';
    proto_env_begin(out, cap, &len, "state", id, session, true);
    proto_buf_append(out, cap, &len,
        ",\"data\":{\"power_external_present\":%s,\"health\":\"%s\","
        "\"focuser_busy\":%s,\"fan_mode\":\"%s\",\"sensor_health\":\"%s\","
        "\"reset_reason\":\"%s\",\"faults\":[",
        power ? "true" : "false",
        fault ? "fault" : "ok",
        ms.status == MOTORS_STATUS_MOVING ? "true" : "false",
        fan_mode_to_string(fan_get_mode()), sensor_fault ? "fault" : "ok",
        reset_reason_str());

    if (fault) {
        proto_buf_append(out, cap, &len,
            "{\"fault_id\":\"f1\",\"subsystem\":\"focuser\",\"code\":\"DRIVER_FAULT\","
            "\"message\":\"TMC2209 not initialised\",\"first_ms\":0,\"active\":true,"
            "\"latched\":true}");
    }
    proto_buf_append(out, cap, &len, "]}}");
}

/* ── state / focuser ───────────────────────────────────────── */

static void state_focuser(char *out, size_t cap, uint32_t id, const char *session) {
    MotorsState ms = motors_current_state();
    const ProtoOperation *op = proto_operation_get();
    bool moving = ms.status == MOTORS_STATUS_MOVING;
    bool fault = motors_status_is_error(ms.status);

    size_t len = 0;
    out[0] = '\0';
    proto_env_begin(out, cap, &len, "state", id, session, true);
    proto_buf_append(out, cap, &len,
        ",\"data\":{"
        "\"counter_steps\":%lld,\"counter_valid\":true,\"is_moving\":%s,"
        "\"source\":\"%s\",\"op_id\":\"%s\",\"op_status\":\"%s\","
        "\"start_counter\":%lld,\"target_counter\":%lld,\"executed_steps\":%lld,"
        "\"speed_steps_s\":0.0,\"enabled\":%s,\"enabled_command\":true,"
        "\"guard\":{\"enabled\":false,\"min_counter\":0,\"max_counter\":0},"
        "\"driver\":{\"uart_ok\":%s,\"last_read_ms\":0,\"overtemp_warning\":false,"
        "\"overtemp_shutdown\":false,\"short_a\":false,\"short_b\":false,"
        "\"open_load_a\":false,\"open_load_b\":false},"
        "\"error\":%s,\"last_error\":null}}",
        (long long) ms.position,
        moving ? "true" : "false",
        moving ? "alpaca" : "none",
        op->op_id, op->status,
        (long long) op->start_counter, (long long) op->target_counter,
        (long long) op->executed_steps,
        motors_is_enabled() ? "true" : "false",
        tmc2209_is_initialized() ? "true" : "false",
        fault ? "\"DRIVER_FAULT\"" : "null");
}

/* ── state / fan ───────────────────────────────────────────── */

static void state_fan(char *out, size_t cap, uint32_t id, const char *session) {
    const char *mode = fan_mode_to_string(fan_get_mode());
    bool power = fan_get_power();
    int speed = fan_get_effective_speed();
    int manual = fan_get_manual_speed();
    int rpm = fan_get_rpm();

    size_t len = 0;
    out[0] = '\0';
    proto_env_begin(out, cap, &len, "state", id, session, true);
    proto_buf_append(out, cap, &len,
        ",\"data\":{\"error\":null,\"mode\":\"%s\",\"power\":%s,\"speed\":%d,"
        "\"manual_speed\":%d,\"power_available\":%s,\"rpm\":",
        mode, power ? "true" : "false", speed, manual,
        board_has_external_power() ? "true" : "false");
    if (rpm >= 0) {
        proto_buf_append(out, cap, &len, "%d}}", rpm);
    } else {
        proto_buf_append(out, cap, &len, "null}}");
    }
}

/* ── state / environment ───────────────────────────────────── */

static void state_environment(char *out, size_t cap, uint32_t id, const char *session) {
    bool valid = environment_ambient_valid();
    float mirror = environment_get_mirror_c();

    size_t len = 0;
    out[0] = '\0';
    proto_env_begin(out, cap, &len, "state", id, session, true);

    if (!valid) {
        proto_buf_append(out, cap, &len,
            ",\"data\":{\"error\":\"SENSOR_ABSENT\",\"ambient_c\":null,"
            "\"humidity_pct\":null,\"mirror_c\":%.2f,\"dew_point_c\":null,"
            "\"mirror_ambient_delta_c\":null,\"mirror_dew_margin_c\":null}}",
            mirror);
        return;
    }

    float ambient = environment_get_ambient_c();
    float humidity = environment_get_humidity_pct();
    float dew = environment_get_dew_point_c();
    proto_buf_append(out, cap, &len,
        ",\"data\":{\"error\":null,\"ambient_c\":%.2f,\"humidity_pct\":%.2f,\"mirror_c\":%.2f,"
        "\"dew_point_c\":%.2f,\"mirror_ambient_delta_c\":%.2f,"
        "\"mirror_dew_margin_c\":%.2f}}",
        ambient, humidity, mirror, dew, mirror - ambient, mirror - dew);
}

/* ── state / vibration ─────────────────────────────────────── */

static void state_vibration(char *out, size_t cap, uint32_t id, const char *session) {
    bool enabled = vibration_get_enabled();
    bool valid = vibration_get_valid();

    size_t len = 0;
    out[0] = '\0';
    proto_env_begin(out, cap, &len, "state", id, session, true);

    if (!enabled) {
        proto_buf_append(out, cap, &len,
            ",\"data\":{\"enabled\":false,\"valid\":false,\"error\":\"SENSOR_ABSENT\","
            "\"rms_x\":null,\"rms_y\":null,\"rms_z\":null,\"rms_total\":null}}");
    } else {
        proto_buf_append(out, cap, &len,
            ",\"data\":{\"enabled\":true,\"valid\":%s,\"error\":null,"
            "\"sample_rate_hz\":5,"
            "\"rms_x\":%.3f,\"rms_y\":%.3f,\"rms_z\":%.3f,\"rms_total\":%.3f,"
            "\"processing\":\"per_second\"}}",
            valid ? "true" : "false",
            vibration_get_rms_x(), vibration_get_rms_y(), vibration_get_rms_z(),
            vibration_get_rms_total());
    }
}

/* ── state / buttons ───────────────────────────────────────── */

static void state_buttons(char *out, size_t cap, uint32_t id, const char *session) {
    MotorsState ms = motors_current_state();
    const char *led = "ready";
    if (motors_status_is_error(ms.status)) {
        led = "error";
    } else if (ms.status == MOTORS_STATUS_MOVING) {
        led = "moving";
    }

    size_t len = 0;
    out[0] = '\0';
    proto_env_begin(out, cap, &len, "state", id, session, true);
    proto_buf_append(out, cap, &len,
        ",\"data\":{\"focuser_enable_pressed\":%s,\"fan_mode_pressed\":%s,"
        "\"local_focus_active\":false,\"led_state\":\"%s\",\"last_input_change_ms\":0}}",
        buttons_focuser_enable_pressed() ? "true" : "false",
        buttons_fan_mode_pressed() ? "true" : "false",
        led);
}

/* ── state / operation ─────────────────────────────────────── */

static void state_operation(char *out, size_t cap, uint32_t id, const char *session) {
    const ProtoOperation *op = proto_operation_get();

    size_t len = 0;
    out[0] = '\0';
    proto_env_begin(out, cap, &len, "state", id, session, true);
    proto_buf_append(out, cap, &len,
        ",\"data\":{\"op_id\":\"%s\",\"status\":\"%s\",\"target\":\"focuser\","
        "\"started_ms\":0,\"ended_ms\":0,\"result\":\"%s\",\"error\":null}}",
        op->op_id, op->status, op->status);
}

/* ── dispatcher ────────────────────────────────────────────── */

void proto_handle_state(const char *req, uint32_t id, char *out, size_t cap) {
    char target[PROTO_MAX_TARGET_LEN] = {0};
    char session[PROTO_MAX_SESSION_LEN] = {0};
    proto_json_get_string(req, "target", target, sizeof(target));
    get_session(req, session, sizeof(session));

    if (strcmp(target, "device") == 0) {
        state_device(out, cap, id, session);
    } else if (strcmp(target, "focuser") == 0) {
        state_focuser(out, cap, id, session);
    } else if (strcmp(target, "fan") == 0) {
        state_fan(out, cap, id, session);
    } else if (strcmp(target, "environment") == 0) {
        state_environment(out, cap, id, session);
    } else if (strcmp(target, "vibration") == 0) {
        state_vibration(out, cap, id, session);
    } else if (strcmp(target, "buttons") == 0) {
        state_buttons(out, cap, id, session);
    } else if (strcmp(target, "operation") == 0) {
        state_operation(out, cap, id, session);
    } else {
        size_t len = 0;
        out[0] = '\0';
        proto_env_begin(out, cap, &len, "state", id, session, false);
        proto_env_error(out, cap, &len, "INVALID_ARGUMENT", "unknown target");
    }
}
