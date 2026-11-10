/* Motors - motors_move.c
 *
 * Purpose: enqueue move commands — absolute (move_to) and relative
 * (move_by). Both are serialized through the motion command queue, and both
 * atomically reserve the axis so a second move is rejected while one is
 * queued, running, or disabled.
 */
#include "motors_internal.h"

static MotorResultCode motors_move_enqueue(int64_t target_steps, bool relative) {
    MotionCommand cmd = {
        .target_steps = target_steps,
        .relative = relative,
    };

    /* Reserve the axis atomically: only READY accepts a new command, so a
     * second move is rejected while one is queued, running, or disabled. */
    MotorResultCode result = motors_transition_reserve();
    if (result != MOTOR_OK) {
        return result;
    }

    if (!motors_queue_put(&cmd)) {
        /* Enqueue failed — release the reservation. */
        motors_transition_settle();
        return MOTOR_ERR_BUSY;
    }
    return MOTOR_OK;
}

MotorResultCode motors_move_to(int64_t position) {
    return motors_move_enqueue(position, false);
}

MotorResultCode motors_move_by(int64_t steps) {
    return motors_move_enqueue(steps, true);
}
