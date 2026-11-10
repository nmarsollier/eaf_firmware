/* Motors - motors_set_position.c
 *
 * Purpose: set the absolute position counter and mark it valid. This is the
 * calibration entry point: it declares what number represents the focuser's
 * current physical position, without moving the motor.
 */
#include "motors_internal.h"

MotorResultCode motors_set_position(int64_t position) {
    portENTER_CRITICAL(&motors_state_lock);

    MotorResultCode result;
    if (motors_status_is_error(motors_state.status)) {
        result = MOTOR_ERR_HARDWARE_ERROR;
    } else if (motors_status_is_busy(motors_state.status)) {
        /* A queued or in-flight move would race with the counter write. */
        result = MOTOR_ERR_BUSY;
    } else {
        /* READY or DISABLED: the axis is stopped, so the reference can be
         * declared. Setting a position while disabled is the normal
         * calibration flow (disable → turn by hand → set position). */
        motors_state.position = position;
        motors_state.counter_valid = true;
        result = MOTOR_OK;
    }

    portEXIT_CRITICAL(&motors_state_lock);
    return result;
}
