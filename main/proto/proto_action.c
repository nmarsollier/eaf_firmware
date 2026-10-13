/* Proto — proto_action.c — ACTION: operations with a beginning and an end.
 *
 * Focuser move/stop are wired to the motors module; device restart reboots.
 * A retried request with the same id replays the original result instead of
 * re-executing (see proto_action_lookup/record).
 */
#include "proto_internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "esp_system.h"
#include "esp_timer.h"

static void get_session(const char *req, char *out, size_t cap) {
    proto_json_get_string(req, "session", out, cap);
}

/* Map a motors result to a protocol error code string. */
static const char *motor_error_code(void) {
    if (!board_has_external_power()) {
        return "POWER_ABSENT";
    }
    return "DRIVER_FAULT";
}

/* Delayed reboot: the handler returns and the response is flushed before this. */
static void restart_timer_cb(void *arg) {
    (void) arg;
    esp_restart();
}

static void emit_ok(char *out, size_t cap, const char *type, uint32_t id,
                    const char *session, const char *data_fmt, ...)
    __attribute__((format(printf, 6, 7)));

static void emit_ok(char *out, size_t cap, const char *type, uint32_t id,
                    const char *session, const char *data_fmt, ...) {
    size_t len = 0;
    out[0] = '\0';
    proto_env_begin(out, cap, &len, type, id, session, true);
    proto_buf_append(out, cap, &len, ",\"data\":{");

    va_list ap;
    va_start(ap, data_fmt);
    char buf[256];   /* the data payload is small (op_id + counters) */
    vsnprintf(buf, sizeof(buf), data_fmt, ap);
    va_end(ap);
    proto_buf_append(out, cap, &len, "%s}}", buf);
}

static void emit_err(char *out, size_t cap, const char *type, uint32_t id,
                     const char *session, const char *code, const char *message) {
    size_t len = 0;
    out[0] = '\0';
    proto_env_begin(out, cap, &len, type, id, session, false);
    proto_env_error(out, cap, &len, code, message);
}

/* ── focuser move ──────────────────────────────────────────── */

static void handle_move(const char *req, uint32_t id, const char *session,
                        char *out, size_t cap) {
    char params[256];
    int64_t delta = 0;
    bool has_delta = false;
    int64_t expected = 0;
    bool has_expected = false;

    if (proto_json_get_object(req, "params", params, sizeof(params))) {
        has_delta = proto_json_get_int64(params, "delta_steps", &delta);
        has_expected = proto_json_get_int64(params, "expected_counter", &expected);
    }

    if (!has_delta) {
        emit_err(out, cap, "action", id, session, "INVALID_ARGUMENT",
                 "move requires params.delta_steps");
        return;
    }

    MotorsState ms = motors_current_state();

    if (motors_status_is_error(ms.status)) {
        emit_err(out, cap, "action", id, session, motor_error_code(),
                 "focuser in error state");
        return;
    }
    if (ms.status == MOTORS_STATUS_MOVING) {
        emit_err(out, cap, "action", id, session, "BUSY",
                 "focuser already moving");
        return;
    }
    if (has_expected && expected != ms.position) {
        emit_err(out, cap, "action", id, session, "COUNTER_CHANGED",
                 "counter changed since the move was computed");
        return;
    }

    if (delta == 0) {
        emit_ok(out, cap, "action", id, session,
                "\"op_id\":\"\",\"status\":\"idle\",\"is_moving\":false,"
                "\"counter_steps\":%lld,\"target_counter\":%lld",
                (long long) ms.position, (long long) ms.position);
        return;
    }

    MotorResultCode r = motors_move_by(delta);
    if (r != MOTOR_OK) {
        emit_err(out, cap, "action", id, session, motor_error_code(),
                 "move rejected");
        return;
    }

    proto_operation_start(ms.position, ms.position + delta);
    const ProtoOperation *op = proto_operation_get();

    emit_ok(out, cap, "action", id, session,
            "\"op_id\":\"%s\",\"status\":\"running\",\"is_moving\":true,"
            "\"counter_steps\":%lld,\"target_counter\":%lld",
            op->op_id, (long long) ms.position, (long long) op->target_counter);
}

/* ── focuser stop ──────────────────────────────────────────── */

static void handle_stop(uint32_t id, const char *session, char *out, size_t cap) {
    motors_stop();
    proto_operation_cancel();

    MotorsState ms = motors_current_state();
    const ProtoOperation *op = proto_operation_get();

    emit_ok(out, cap, "action", id, session,
            "\"is_moving\":false,\"counter_steps\":%lld,\"cancelled_op_id\":\"%s\"",
            (long long) ms.position, op->op_id);
}

/* ── device restart ────────────────────────────────────────── */

static void handle_restart(uint32_t id, const char *session, char *out, size_t cap) {
    esp_timer_handle_t timer = NULL;
    esp_timer_create_args_t args = {
        .callback = restart_timer_cb,
        .name = "proto_restart",
    };
    if (esp_timer_create(&args, &timer) == ESP_OK) {
        esp_timer_start_once(timer, 200 * 1000);
        emit_ok(out, cap, "action", id, session,
                "\"op_id\":\"\",\"status\":\"accepted\"");
    } else {
        emit_err(out, cap, "action", id, session, "INTERNAL",
                 "restart unavailable");
    }
}

/* ── dispatcher ────────────────────────────────────────────── */

void proto_handle_action(const char *req, uint32_t id, char *out, size_t cap) {
    char target[PROTO_MAX_TARGET_LEN] = {0};
    char action[PROTO_MAX_ACTION_LEN] = {0};
    char session[PROTO_MAX_SESSION_LEN] = {0};

    proto_json_get_string(req, "target", target, sizeof(target));
    proto_json_get_string(req, "action", action, sizeof(action));
    get_session(req, session, sizeof(session));

    if (strcmp(target, "focuser") == 0 && strcmp(action, "move") == 0) {
        handle_move(req, id, session, out, cap);
    } else if (strcmp(target, "focuser") == 0 && strcmp(action, "stop") == 0) {
        handle_stop(id, session, out, cap);
    } else if (strcmp(target, "device") == 0 && strcmp(action, "restart") == 0) {
        handle_restart(id, session, out, cap);
    } else {
        emit_err(out, cap, "action", id, session, "NOT_SUPPORTED",
                 "unknown action");
    }
}
