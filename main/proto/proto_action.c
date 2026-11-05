/* Proto — proto_action.c — ACTION: operations with a beginning and an end.
 *
 * Focuser move/stop are wired to the motors module; device restart reboots and
 * device bootloader reboots into ROM download mode for button-less flashing.
 * A retried request with the same id replays the original result instead of
 * re-executing (see proto_action_lookup/record).
 */
#include "proto_internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "usb_cdc.h"

#include "esp_system.h"
#include "esp_timer.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"

static void get_session(const char *req, char *out, size_t cap) {
    proto_json_get_string(req, "session", out, cap);
}

/* Map a motors result to a protocol error code string. */
static const char *motor_error_code(void) {
    return "DRIVER_FAULT";
}

/* Error code for a rejected move: a disabled focuser is distinct from a
 * hardware fault. */
static const char *move_error_code(MotorResultCode r) {
    if (r == MOTOR_ERR_DISABLED) {
        return "DISABLED";
    }
    return motor_error_code();
}

/* Delayed reboot: the handler returns and the response is flushed before this. */
static void restart_timer_cb(void *arg) {
    (void) arg;
    esp_restart();
}

/* Delayed reboot into ROM download mode (button-less flashing). */
static void bootloader_timer_cb(void *arg) {
    (void) arg;
    if (usb_cdc_prepare_download() != ESP_OK) {
        return;
    }
    REG_SET_BIT(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
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
    /* The motion task counts a single move in uint32 steps — reject a delta
     * whose magnitude does not fit. */
    if (delta < -(int64_t) UINT32_MAX || delta > (int64_t) UINT32_MAX) {
        emit_err(out, cap, "action", id, session, "INVALID_ARGUMENT",
                 "delta_steps out of range");
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
        emit_err(out, cap, "action", id, session, move_error_code(r),
                 "move rejected");
        return;
    }

    proto_operation_start(ms.position, ms.position + delta);
    ProtoOperation op = proto_operation_get();

    emit_ok(out, cap, "action", id, session,
            "\"op_id\":\"%s\",\"status\":\"running\",\"is_moving\":true,"
            "\"counter_steps\":%lld,\"target_counter\":%lld",
            op.op_id, (long long) ms.position, (long long) op.target_counter);
    return;
}

/* ── focuser stop ──────────────────────────────────────────── */

static void handle_stop(uint32_t id, const char *session, char *out, size_t cap) {
    MotorResultCode r = motors_stop();
    if (r != MOTOR_OK) {
        emit_err(out, cap, "action", id, session, "INTERNAL",
                 "stop did not settle");
        return;
    }

    proto_operation_cancel();

    MotorsState ms = motors_current_state();
    ProtoOperation op = proto_operation_get();

    emit_ok(out, cap, "action", id, session,
            "\"is_moving\":false,\"counter_steps\":%lld,\"cancelled_op_id\":\"%s\"",
            (long long) ms.position, op.op_id);
    return;
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
        return;
    } else {
        emit_err(out, cap, "action", id, session, "INTERNAL",
                 "restart unavailable");
        return;
    }
}

/* ── device bootloader ─────────────────────────────────────── */

static void handle_bootloader(uint32_t id, const char *session, char *out, size_t cap) {
    esp_timer_handle_t timer = NULL;
    esp_timer_create_args_t args = {
        .callback = bootloader_timer_cb,
        .name = "proto_bootloader",
    };
    esp_err_t err = esp_timer_create(&args, &timer);
    if (err == ESP_OK) {
        err = esp_timer_start_once(timer, 200 * 1000);
        if (err != ESP_OK) {
            esp_timer_delete(timer);
        }
    }
    if (err != ESP_OK) {
        emit_err(out, cap, "action", id, session, "INTERNAL",
                 "bootloader unavailable");
        return;
    }
    emit_ok(out, cap, "action", id, session,
            "\"op_id\":\"\",\"status\":\"accepted\"");
}

/* ── dispatcher ────────────────────────────────────────────── */

void proto_handle_action(const char *req, uint32_t id, char *out, size_t cap) {
    char target[PROTO_MAX_TARGET_LEN] = {0};
    char action[PROTO_MAX_ACTION_LEN] = {0};
    char session[PROTO_MAX_SESSION_LEN] = {0};

    proto_json_get_string(req, "target", target, sizeof(target));
    proto_json_get_string(req, "action", action, sizeof(action));
    get_session(req, session, sizeof(session));

    /* Idempotency: a retried action id replays its original response instead of
     * re-executing (a relative move would otherwise move the focuser again). */
    if (proto_action_lookup(id, out, cap)) {
        return;
    }

    if (strcmp(target, "focuser") == 0 && strcmp(action, "move") == 0) {
        handle_move(req, id, session, out, cap);
    } else if (strcmp(target, "focuser") == 0 && strcmp(action, "stop") == 0) {
        handle_stop(id, session, out, cap);
    } else if (strcmp(target, "device") == 0 && strcmp(action, "restart") == 0) {
        handle_restart(id, session, out, cap);
    } else if (strcmp(target, "device") == 0 && strcmp(action, "bootloader") == 0) {
        handle_bootloader(id, session, out, cap);
    } else {
        emit_err(out, cap, "action", id, session, "NOT_SUPPORTED",
                 "unknown action");
    }

    proto_action_record(id, out);
}
