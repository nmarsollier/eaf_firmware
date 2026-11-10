/* Proto — proto_operation.c — operation tracking (op_id / status / counters).
 *
 * A finite operation (a focuser move) gets an op_id unique per boot, and its
 * status advances running → completed (or cancelled on stop, failed on a
 * hardware fault). The focuser state handler reports it so the host can
 * reconcile after a disconnect.  s_op is shared between the request (RX) task
 * and the runtime loop, so every access is serialised by s_op_lock.
 */
#include "proto_internal.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"

static ProtoOperation s_op = {
    .op_id = "",
    .status = "idle",
};
static uint32_t s_op_seq;
static portMUX_TYPE s_op_lock = portMUX_INITIALIZER_UNLOCKED;

static void op_id_for(uint32_t seq, char *out, size_t cap) {
    char boot[9];
    proto_boot_id_str(boot, sizeof(boot));
    snprintf(out, cap, "%s:op%lu", boot, (unsigned long) seq);
}

void proto_operation_start(int64_t start_counter, int64_t target_counter) {
    portENTER_CRITICAL(&s_op_lock);
    s_op_seq++;
    op_id_for(s_op_seq, s_op.op_id, sizeof(s_op.op_id));
    snprintf(s_op.status, sizeof(s_op.status), "running");
    s_op.start_counter = start_counter;
    s_op.target_counter = target_counter;
    s_op.executed_steps = 0;
    portEXIT_CRITICAL(&s_op_lock);
}

void proto_operation_cancel(void) {
    portENTER_CRITICAL(&s_op_lock);
    if (strcmp(s_op.status, "running") != 0) {
        portEXIT_CRITICAL(&s_op_lock);
        return;
    }
    MotorsState ms = motors_current_state();
    s_op.executed_steps = ms.position - s_op.start_counter;
    snprintf(s_op.status, sizeof(s_op.status), "cancelled");
    portEXIT_CRITICAL(&s_op_lock);
}

/* Advance a running operation to its terminal state once the motor stops:
 * completed only if it reached the target, failed on a hardware fault, and
 * cancelled otherwise (e.g. the focuser was disabled mid-move). */
void proto_operation_poll(void) {
    portENTER_CRITICAL(&s_op_lock);
    if (strcmp(s_op.status, "running") != 0) {
        portEXIT_CRITICAL(&s_op_lock);
        return;
    }
    MotorsState ms = motors_current_state();
    s_op.executed_steps = ms.position - s_op.start_counter;
    if (motors_status_is_busy(ms.status)) {
        portEXIT_CRITICAL(&s_op_lock);
        return;
    }
    if (motors_status_is_error(ms.status)) {
        snprintf(s_op.status, sizeof(s_op.status), "failed");
    } else if (ms.position == s_op.target_counter) {
        snprintf(s_op.status, sizeof(s_op.status), "completed");
    } else {
        snprintf(s_op.status, sizeof(s_op.status), "cancelled");
    }
    portEXIT_CRITICAL(&s_op_lock);
}

ProtoOperation proto_operation_get(void) {
    ProtoOperation op;
    portENTER_CRITICAL(&s_op_lock);
    op = s_op;
    portEXIT_CRITICAL(&s_op_lock);
    return op;
}

void proto_poll(void) {
    proto_operation_poll();
}
