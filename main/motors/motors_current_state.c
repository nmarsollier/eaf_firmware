/* Motors - motors_current_state.c
 *
 * Purpose: return a copy of the authoritative motors state, plus status
 * helpers for external consumers.
 */
#include "motors_internal.h"

/*
 * Return a snapshot copy of the motors module's authoritative state.
 * The copy is taken under motors_state_lock so a reader on another core
 * cannot observe a torn int64_t counter or a half-updated status.
 */
MotorsState motors_current_state(void) {
    MotorsState copy;
    portENTER_CRITICAL(&motors_state_lock);
    copy = motors_state;
    portEXIT_CRITICAL(&motors_state_lock);
    return copy;
}

const char *motors_status_to_string(MotorsStatus status) {
    switch (status) {
        case MOTORS_STATUS_READY:
            return "ready";
        case MOTORS_STATUS_MOVING:
            return "moving";
        case MOTORS_STATUS_ERROR:
            return "error";
        default:
            return "unknown";
    }
}

bool motors_status_is_error(MotorsStatus status) {
    return status == MOTORS_STATUS_ERROR;
}
