/* Motors - motors_move.c
 *
 * Purpose: enqueue move commands — absolute (move_to) and relative
 * (move_by). Both are serialized through the motion command queue.
 */
#include "motors_internal.h"

MotorResultCode motors_move_to(int64_t position) {
    if (motors_status_is_error(motors_state.status)) {
        return MOTOR_ERR_HARDWARE_ERROR;
    }

    MotionCommand cmd = {
        .target_steps = position,
        .relative = false,
    };
    if (!motors_queue_put(&cmd)) {
        return MOTOR_ERR_BUSY;
    }
    return MOTOR_OK;
}

MotorResultCode motors_move_by(int64_t steps) {
    if (motors_status_is_error(motors_state.status)) {
        return MOTOR_ERR_HARDWARE_ERROR;
    }

    MotionCommand cmd = {
        .target_steps = steps,
        .relative = true,
    };
    if (!motors_queue_put(&cmd)) {
        return MOTOR_ERR_BUSY;
    }
    return MOTOR_OK;
}
