/* Motors - motors_stop.c
 *
 * Purpose: stop all motor movement immediately. Clears the command queue,
 * invalidates in-flight motion via the stop generation, and returns to READY.
 */
#include "motors_internal.h"

MotorResultCode motors_stop(void) {
    /* ERROR is unrecoverable — never transition out of it. */
    if (motors_status_is_error(motors_state.status)) {
        return MOTOR_ERR_HARDWARE_ERROR;
    }

    motors_stop_generation++;
    motors_queue_clear();

    portENTER_CRITICAL(&motors_state_lock);
    motors_state.status = MOTORS_STATUS_READY;
    portEXIT_CRITICAL(&motors_state_lock);

    return MOTOR_OK;
}
