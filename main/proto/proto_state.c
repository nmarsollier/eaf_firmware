/* Proto — proto_state.c — STATE: coherent snapshots per target.
 *
 * Routed by `target`: device, focuser, buttons, operation. device/focuser read
 * board/motors/tmc; buttons/operation read the focuser's local inputs and the
 * last operation status.
 */
#include "proto_internal.h"

#include <string.h>

#include "esp_system.h"

#include "buttons.h"

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

    size_t len = 0;
    out[0] = '\0';
    proto_env_begin(out, cap, &len, "state", id, session, true);
    proto_buf_append(out, cap, &len,
        ",\"data\":{\"power_external_present\":%s,\"health\":\"%s\","
        "\"focuser_busy\":%s,\"reset_reason\":\"%s\",\"faults\":[",
        power ? "true" : "false",
        fault ? "fault" : "ok",
        ms.status == MOTORS_STATUS_MOVING ? "true" : "false",
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
        ",\"data\":{\"focuser_enable_pressed\":%s,"
        "\"local_focus_active\":false,\"led_state\":\"%s\",\"last_input_change_ms\":0}}",
        buttons_focuser_enable_pressed() ? "true" : "false",
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
