/* Motors - motors_transition.c
 *
 * Purpose: the single owner of motors_state.status. Every command source
 * (move, stop, enable/disable, power, error) routes its status change through
 * one of these validated transitions, so the machine rules live in one place.
 */
#include "motors_internal.h"

MotorResultCode motors_transition_reserve(void) {
    portENTER_CRITICAL(&motors_state_lock);

    MotorResultCode result;
    if (motors_status_is_error(motors_state.status)) {
        result = MOTOR_ERR_HARDWARE_ERROR;
    } else if (motors_state.status == MOTORS_STATUS_DISABLED) {
        result = MOTOR_ERR_DISABLED;
    } else if (motors_state.status != MOTORS_STATUS_READY) {
        result = MOTOR_ERR_BUSY;
    } else {
        motors_state.status = MOTORS_STATUS_QUEUED;
        result = MOTOR_OK;
    }

    portEXIT_CRITICAL(&motors_state_lock);
    return result;
}

void motors_transition_moving(void) {
    /* Trusted: called only from run_move, after a successful reserve. */
    portENTER_CRITICAL(&motors_state_lock);
    motors_state.status = MOTORS_STATUS_MOVING;
    portEXIT_CRITICAL(&motors_state_lock);
}

void motors_transition_settle(void) {
    portENTER_CRITICAL(&motors_state_lock);
    if (motors_state.status == MOTORS_STATUS_MOVING ||
        motors_state.status == MOTORS_STATUS_QUEUED) {
        motors_state.status = MOTORS_STATUS_READY;
    }
    portEXIT_CRITICAL(&motors_state_lock);
}

void motors_transition_disable(void) {
    portENTER_CRITICAL(&motors_state_lock);
    if (motors_state.status == MOTORS_STATUS_READY) {
        motors_state.status = MOTORS_STATUS_DISABLED;
        /* Torque released — the position reference is lost. */
        motors_state.counter_valid = false;
    }
    portEXIT_CRITICAL(&motors_state_lock);
}

void motors_transition_enable(void) {
    portENTER_CRITICAL(&motors_state_lock);
    if (motors_state.status == MOTORS_STATUS_DISABLED) {
        motors_state.status = MOTORS_STATUS_READY;
    }
    portEXIT_CRITICAL(&motors_state_lock);
}

void motors_transition_error(void) {
    portENTER_CRITICAL(&motors_state_lock);
    motors_state.status = MOTORS_STATUS_ERROR;
    /* A fault (or the power loss that caused it) makes the executed-step count
     * uncertain, so the position reference is no longer trustworthy. */
    motors_state.counter_valid = false;
    portEXIT_CRITICAL(&motors_state_lock);
}

void motors_transition_recover(bool enabled) {
    portENTER_CRITICAL(&motors_state_lock);
    if (motors_state.status == MOTORS_STATUS_ERROR) {
        motors_state.status = enabled ? MOTORS_STATUS_READY : MOTORS_STATUS_DISABLED;
    }
    portEXIT_CRITICAL(&motors_state_lock);
}
