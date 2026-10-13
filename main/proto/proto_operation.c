/* Proto — proto_operation.c — operation tracking (op_id / status / counters).
 *
 * A finite operation (a focuser move) gets an op_id unique per boot, and its
 * status advances running → completed (or cancelled on stop). The focuser state
 * handler reports it so the host can reconcile after a disconnect.
 */
#include "proto_internal.h"

#include <stdio.h>
#include <string.h>

static ProtoOperation s_op = {
    .op_id = "",
    .status = "idle",
};
static uint32_t s_op_seq;

static void op_id_for(uint32_t seq, char *out, size_t cap) {
    char boot[9];
    proto_boot_id_str(boot, sizeof(boot));
    snprintf(out, cap, "%s:op%lu", boot, (unsigned long) seq);
}

void proto_operation_start(int64_t start_counter, int64_t target_counter) {
    s_op_seq++;
    op_id_for(s_op_seq, s_op.op_id, sizeof(s_op.op_id));
    snprintf(s_op.status, sizeof(s_op.status), "running");
    s_op.start_counter = start_counter;
    s_op.target_counter = target_counter;
    s_op.executed_steps = 0;
}

void proto_operation_cancel(void) {
    if (strcmp(s_op.status, "running") != 0) {
        return;
    }
    MotorsState ms = motors_current_state();
    s_op.executed_steps = ms.position - s_op.start_counter;
    snprintf(s_op.status, sizeof(s_op.status), "cancelled");
}

/* Advance a running operation to completed once the motor stops. Also keeps
 * executed_steps fresh while it runs. */
void proto_operation_poll(void) {
    if (strcmp(s_op.status, "running") != 0) {
        return;
    }
    MotorsState ms = motors_current_state();
    s_op.executed_steps = ms.position - s_op.start_counter;
    if (ms.status != MOTORS_STATUS_MOVING) {
        snprintf(s_op.status, sizeof(s_op.status), "completed");
    }
}

const ProtoOperation *proto_operation_get(void) {
    return &s_op;
}

void proto_poll(void) {
    proto_operation_poll();
}
